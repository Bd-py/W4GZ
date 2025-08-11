#include "LootManager.h"
#include "CollectibleData.h"
#include "ECS_Component_Loot.h" // Ganimetlere özel component'ler
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "W4GZ_Character.h" // Oyuncu karakterimiz
#include "AbilitySystemComponent.h"
#include "W4GZ_AttributeSet.h" 

ALootManager::ALootManager()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot")));
}

void ALootManager::BeginPlay()
{
	Super::BeginPlay();
	LootWorld = MakeUnique<flecs::world>();
	LootWorld->set_threads(1);

	// Ganimet Component'lerini Kaydet
	LootWorld->component<FLoot_Position>();
	LootWorld->component<FLoot_Velocity>();
	LootWorld->component<FLoot_InstanceLink>();
	LootWorld->component<FLoot_MagnetTarget>();
	LootWorld->component<FLoot_Data>();

	// --- HISM Component'lerini DT_Collectibles'a Göre Oluştur ---
	if (CollectiblesDataTable)
	{
		const TArray<FName> RowNames = CollectiblesDataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FCollectibleData* Data = CollectiblesDataTable->FindRow<FCollectibleData>(RowName, TEXT(""));
			if (Data && Data->StaticMesh)
			{
				UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, RowName);
				if (HISM)
				{
					HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					HISM->RegisterComponent();
					HISM->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
					HISM->SetStaticMesh(Data->StaticMesh);
					CollectibleMeshInstances.Add(RowName, HISM);
				}
			}
		}
	}
}

void ALootManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!LootWorld) return;

	LootUpdateTimer += DeltaTime;
	if (LootUpdateTimer >= LootUpdateInterval)
	{
		LootUpdateTimer = 0.f;
		UpdateInstanceTransforms(); // sadece belli aralıklarla çağır
	}

	LootWorld->progress(DeltaTime);

	AW4GZCharacter* PlayerCharacter = Cast<AW4GZCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter) return;

	const FVector PlayerLocation = PlayerCharacter->GetActorLocation();

	// --- YENİ DİNAMİK YARIÇAP MANTIĞI ---
	float MagnetRadius = 500.f; // Varsayılan değer
	if (UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
	{
		if (ASC->HasAttributeSetForAttribute(UW4GZAttributeSet::GetPickupRangeAttribute()))
		{
			MagnetRadius = ASC->GetNumericAttribute(UW4GZAttributeSet::GetPickupRangeAttribute());
		}
	}

	const float MagnetRadiusSq = FMath::Square(MagnetRadius);
	const float PickupRadiusSq = FMath::Square(50.f);

	// Faz 1: Mıknatıs ve Toplama Tespiti
	LootWorld->query<FLoot_Position, FLoot_MagnetTarget, const FLoot_Data, const FLoot_InstanceLink>().each(
		[this, PlayerCharacter, PlayerLocation, MagnetRadiusSq, PickupRadiusSq](flecs::entity e, FLoot_Position& p, FLoot_MagnetTarget& mt, const FLoot_Data& d, const FLoot_InstanceLink& l)
		{
			const float DistSq = FVector::DistSquared(p.Value, PlayerLocation);
			if (DistSq < PickupRadiusSq)
			{
				this->CollectionCommandBuffer.Add({ e.id(), d.CollectibleID, d.Value, l.Value, PlayerCharacter });
				return;
			}
			if (DistSq < MagnetRadiusSq)
			{ 
				mt.Target = PlayerCharacter;

				if (UHierarchicalInstancedStaticMeshComponent* HISM = CollectibleMeshInstances.FindRef(d.CollectibleID))
				{
					// Eğer loot uzak pozisyondaysa, gerçek pozisyona taşı
					const FTransform* LastTransform = LastKnownTransforms.Find(e.id());
					if (LastTransform && LastTransform->GetLocation().Z < -9999.f)
					{
						HISM->UpdateInstanceTransform(l.Value, *LastTransform, true, false, false);
					}
				}
			}

			else 
			{ 
				mt.Target = nullptr;
			}
		});

	// Faz 2: Toplanan Ganimet Emirlerini Güvenle İşleme
	if (CollectionCommandBuffer.Num() > 0)
	{
		for (const FLootCollectedCommand& Command : CollectionCommandBuffer)
		{
			this->OnCollectiblePickedUp(Command.Player.Get(), Command.CollectibleID, Command.Value);

			if (UHierarchicalInstancedStaticMeshComponent* HISM = this->CollectibleMeshInstances.FindRef(Command.CollectibleID))
			{
				const FVector HiddenPosition(0.f, 0.f, -100000.f);
				HISM->UpdateInstanceTransform(Command.InstanceLink, FTransform(HiddenPosition), true, false, false);

				this->AvailableInstancesMap.FindOrAdd(Command.CollectibleID).Add(Command.InstanceLink);
			}

			flecs::entity e = LootWorld->entity(Command.EntityToDestroyID);
			if (e.is_alive())
			{
				LastKnownTransforms.Remove(e.id()); // Transform kaydını da temizle
				e.destruct();
			}
		}
		CollectionCommandBuffer.Empty();
	}

	// Faz 3: Hareket Sistemi
	LootWorld->query<FLoot_Position, FLoot_Velocity, const FLoot_MagnetTarget>().each([DeltaTime](FLoot_Position& p, FLoot_Velocity& v, const FLoot_MagnetTarget& mt)
		{
			if (mt.Target.IsValid())
			{
				// Hedef geçerliyse, hedefe doğru bir hız belirle
				const FVector TargetLocation = mt.Target->GetActorLocation();
				const FVector Direction = (TargetLocation - p.Value).GetSafeNormal();
				const float Speed = 1500.f; // Çekilme hızı
				v.Value = Direction * Speed;
			}
			else
			{
				v.Value += FVector(0, 0, -980.f) * DeltaTime; // Yer çekimi
				v.Value *= 0.98f; // Hava sürtünmesi
			}

			// Pozisyonu, hesaplanan yeni hıza ve geçen zamana göre güncelle
			p.Value += v.Value * DeltaTime;
			// Yere düşmesini sağla (basit zemin kontrolü)
			if (p.Value.Z < 0.f) { p.Value.Z = 0.f; v.Value.Z = 0.f; }
		});

	// FAZ 4: GÜNCELLENMİŞ OPTİMİZE RENDERING SİSTEMİ
	UpdateInstanceTransforms();
}



void ALootManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (LootWorld) { LootWorld.Reset(); }
}

void ALootManager::CreateLootDrop(const TArray<FST_LootDrop>& LootTable, FVector Location)
{
	if (!LootWorld) return;

	for (const FST_LootDrop& LootDrop : LootTable)
	{
		if (FMath::FRand() <= LootDrop.DropChance)
		{
			const int32 Quantity = FMath::RandRange(LootDrop.MinValue, LootDrop.MaxValue);
			if (Quantity > 0)
			{
				UHierarchicalInstancedStaticMeshComponent* HISM = CollectibleMeshInstances.FindRef(LootDrop.CollectibleID);
				if (!HISM) continue;

				int32 InstanceIndex = INDEX_NONE;
				TArray<int32>& AvailableInstances = AvailableInstancesMap.FindOrAdd(LootDrop.CollectibleID);

				if (AvailableInstances.Num() > 0)
				{
					InstanceIndex = AvailableInstances.Pop();
					const FVector SpawnLocation = Location + FVector(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), 0.f);
					HISM->UpdateInstanceTransform(InstanceIndex, FTransform(SpawnLocation), true, false, false);
				}
				else
				{
					const FVector SpawnLocation = Location + FVector(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), 0.f);
					InstanceIndex = HISM->AddInstance(FTransform(SpawnLocation));
				}

				// ECS entity oluştur
				LootWorld->entity()
					.set<FLoot_Position>({ Location })
					.set<FLoot_Velocity>({ FVector::ZeroVector })
					.set<FLoot_InstanceLink>({ InstanceIndex })
					.set<FLoot_Data>({ LootDrop.CollectibleID, Quantity })
					.add<FLoot_MagnetTarget>();
			}
		}
	}
}


void ALootManager::UpdateInstanceTransforms()
{
	if (!LootWorld) return;

	TMap<UHierarchicalInstancedStaticMeshComponent*, TArray<TPair<int32, FTransform>>> UpdateBatchMap;
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager) return;

	const FVector CameraLocation = CameraManager->GetCameraLocation();
	const float VisibilityDistanceSq = FMath::Square(3000.f); // 30m görünürlük mesafesi

	LootWorld->query<const FLoot_Position, const FLoot_InstanceLink, const FLoot_Data>().each(
		[this, &UpdateBatchMap, &CameraLocation, VisibilityDistanceSq](flecs::entity e, const FLoot_Position& Position, const FLoot_InstanceLink& Link, const FLoot_Data& Data)
		{
			UHierarchicalInstancedStaticMeshComponent* HISM = CollectibleMeshInstances.FindRef(Data.CollectibleID);
			if (!HISM) return;

			const int32 InstanceIndex = Link.Value;
			const FTransform NewTransform(Position.Value);
			const FTransform* LastTransform = LastKnownTransforms.Find(e.id());

			// Kamera ile loot arası mesafe kontrolü
			const float DistSq = FVector::DistSquared(Position.Value, CameraLocation);
			const bool bShouldBeVisible = DistSq < VisibilityDistanceSq;

			// Görünür olmalıysa transform güncelle
			if (bShouldBeVisible)
			{
				TArray<TPair<int32, FTransform>>& List = UpdateBatchMap.FindOrAdd(HISM);
				List.Add({ InstanceIndex, NewTransform });
			}
			else
			{
				// Görünür olmaması gereken instance’ı uzaklaştır
				TArray<TPair<int32, FTransform>>& List = UpdateBatchMap.FindOrAdd(HISM);
				List.Add({ InstanceIndex, FTransform(FVector(0.f, 0.f, -100000.f)) });
			}

			LastKnownTransforms.Add(e.id(), NewTransform);
		});

	for (auto& Elem : UpdateBatchMap)
	{
		UHierarchicalInstancedStaticMeshComponent* HISM = Elem.Key;
		const TArray<TPair<int32, FTransform>>& Instances = Elem.Value;

		for (const TPair<int32, FTransform>& Pair : Instances)
		{
			HISM->UpdateInstanceTransform(Pair.Key, Pair.Value, true, false, false);
		}
		HISM->MarkRenderStateDirty();
	}
}



