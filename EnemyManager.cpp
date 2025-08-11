#include "EnemyManager.h"
#include "EnemyData.h"
#include "ECS_Components.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "W4GZ_Character.h"
#include "EnemyProjectile.h"
#include "TimerManager.h"
#include "SpawnPattern.h"
#include "LootManager.h"
#include "Misc/ScopeLock.h"
#include <type_traits>
#include "GenericPlatform/GenericPlatformMisc.h"
#include <new>

// ===== Throttling params =====
static constexpr int32 kTransformStrideNear = 1;            // yakın düşman: her frame
static constexpr int32 kTransformStrideFar = 4;            // uzak düşman: 4 frame’de bir
static constexpr float kFarDistanceSq = 3000.f * 3000.f;

using flecs::query; // explicit destructor çağrılarında kolaylık

// Helper: get grid coords
FIntVector AEnemyManager::GetGridCoords(const FVector& Location) const
{
    return FIntVector(
        FMath::FloorToInt(Location.X / GridCellSize),
        FMath::FloorToInt(Location.Y / GridCellSize),
        0
    );
}

AEnemyManager::AEnemyManager()
    : CachedPositionQuery()
    , CachedConstPositionQuery()
    , CachedMovementQuery()
    , CachedMeleeQuery()
    , CachedRangedQuery()
    , CachedInstanceTransformQuery()
    , CachedAvoidanceQuery()
    , CachedDestructionQuery()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot")));
    GridCellSize = 150;
}

void AEnemyManager::BeginPlay()
{
    Super::BeginPlay();

    // Create flecs world
    ECSWorld = new flecs::world();
    int32 NumThreads = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
    if (NumThreads < 1) NumThreads = 1;
    ECSWorld->set_threads((int)NumThreads);
    UE_LOG(LogTemp, Log, TEXT("Flecs threads set to %d"), NumThreads);

    // Register components
    ECSWorld->component<FPosition>();
    ECSWorld->component<FVelocity>();
    ECSWorld->component<FHealth>();
    ECSWorld->component<FInstanceLink>();
    ECSWorld->component<FTargetActor>();
    ECSWorld->component<FEnemyArchetype>();
    ECSWorld->component<FAttackTimer>();

    ECSWorld->component<FNeedsDestruction>();
    ECSWorld->component<FDamageRequest>();
    ECSWorld->component<FKnockbackRequest>();
    ECSWorld->component<FRequestMeleeAttack>();
    ECSWorld->component<FRequestRangedAttack>();
    ECSWorld->component<FAvoidance>();

    // Create HISM pools
    if (EnemiesDataTable)
    {
        const TArray<FName> RowNames = EnemiesDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            FEnemyData* EnemyData = EnemiesDataTable->FindRow<FEnemyData>(RowName, TEXT(""));
            if (EnemyData && EnemyData->EnemyStaticMesh)
            {
                UHierarchicalInstancedStaticMeshComponent* HISM_Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, RowName);
                if (HISM_Component)
                {
                    HISM_Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                    HISM_Component->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
                    HISM_Component->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
                    HISM_Component->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
                    HISM_Component->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);

                    HISM_Component->RegisterComponent();
                    HISM_Component->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
                    HISM_Component->SetStaticMesh(EnemyData->EnemyStaticMesh);

                    HISM_Component->SetMobility(EComponentMobility::Movable);
                    HISM_Component->SetCastShadow(true);
                    HISM_Component->bCastDynamicShadow = true;
                    HISM_Component->bAffectDistanceFieldLighting = true;
                    HISM_Component->bAffectDynamicIndirectLighting = true;
                    HISM_Component->SetCullDistances(0, 0);
                    HISM_Component->SetCanEverAffectNavigation(false);
                    HISM_Component->SetVisibility(true);
                    HISM_Component->bHiddenInGame = false;
                    HISM_Component->SetBoundsScale(100.f);

                    EnemyMeshInstances.Add(RowName, HISM_Component);
                }
            }
        }
    }

    // --- cache queries (member queries) via placement-new ---
    new (&CachedPositionQuery) flecs::query<FPosition>(ECSWorld->query_builder<FPosition>().build());
    new (&CachedConstPositionQuery) flecs::query<const FPosition>(ECSWorld->query_builder<const FPosition>().build());
    new (&CachedMovementQuery) flecs::query<FVelocity, const FPosition, const FTargetActor, const FEnemyArchetype, const FAttackTimer>(
        ECSWorld->query_builder<FVelocity, const FPosition, const FTargetActor, const FEnemyArchetype, const FAttackTimer>().build());
    new (&CachedMeleeQuery) flecs::query<const FPosition, const FEnemyArchetype, const FRequestMeleeAttack>(
        ECSWorld->query_builder<const FPosition, const FEnemyArchetype, const FRequestMeleeAttack>().build());
    new (&CachedRangedQuery) flecs::query<const FPosition, const FTargetActor, const FEnemyArchetype, const FRequestRangedAttack>(
        ECSWorld->query_builder<const FPosition, const FTargetActor, const FEnemyArchetype, const FRequestRangedAttack>().build());
    new (&CachedInstanceTransformQuery) flecs::query<FPosition, const FInstanceLink, const FEnemyArchetype>(
        ECSWorld->query_builder<FPosition, const FInstanceLink, const FEnemyArchetype>().build());
    new (&CachedAvoidanceQuery) flecs::query<FPosition, const FAvoidance>(
        ECSWorld->query_builder<FPosition, const FAvoidance>().build());
    new (&CachedDestructionQuery) flecs::query<const FInstanceLink, const FEnemyArchetype, const FNeedsDestruction>(
        ECSWorld->query_builder<const FInstanceLink, const FEnemyArchetype, const FNeedsDestruction>().build());

    // Systems
    ECSWorld->system<FHealth, const FDamageRequest>()
        .each([](flecs::entity e, FHealth& h, const FDamageRequest& req)
            {
                h.CurrentHealth -= req.Amount;
                e.remove<FDamageRequest>();
            });

    ECSWorld->system<FVelocity, const FKnockbackRequest>()
        .each([](flecs::entity e, FVelocity& v, const FKnockbackRequest& req)
            {
                v.Value += req.Force;
                v.Value = v.Value.GetClampedToMaxSize(2000.f);
                e.remove<FKnockbackRequest>();
            });

    ECSWorld->system<const FHealth, const FPosition, const FEnemyArchetype>("DeathSystem")
        .each([this](flecs::entity e, const FHealth& h, const FPosition& p, const FEnemyArchetype& a)
            {
                if (h.CurrentHealth <= 0.0f)
                {
                    if (ALootManager* LootManager = Cast<ALootManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ALootManager::StaticClass())))
                    {
                        FEnemyData* EnemyData = this->EnemiesDataTable->FindRow<FEnemyData>(a.Value, TEXT(""));
                        if (EnemyData)
                        {
                            TArray<FST_LootDrop> FinalLootDrops = EnemyData->LootDrops;

                            if (EnemyData->ExperienceReward > 0)
                            {
                                FName ExpIdToDrop = TEXT("EXP_Small");
                                if (EnemyData->ExperienceReward >= 50) ExpIdToDrop = TEXT("EXP_Large");
                                else if (EnemyData->ExperienceReward >= 20) ExpIdToDrop = TEXT("EXP_Medium");

                                FST_LootDrop ExpDrop;
                                ExpDrop.CollectibleID = ExpIdToDrop;
                                ExpDrop.DropChance = 1.0f;
                                ExpDrop.MinValue = (int32)EnemyData->ExperienceReward;
                                ExpDrop.MaxValue = (int32)EnemyData->ExperienceReward;
                                FinalLootDrops.Add(ExpDrop);
                            }

                            LootManager->CreateLootDrop(FinalLootDrops, p.Value);
                        }
                    }

                    e.add<FNeedsDestruction>();
                }
            });

    ECSWorld->system<FAttackTimer, const FPosition, const FTargetActor, const FEnemyArchetype, FVelocity>()
        .each([this](flecs::entity e, FAttackTimer& cd, const FPosition& p, const FTargetActor& t, const FEnemyArchetype& a, FVelocity& v)
            {
                cd.TimeRemaining -= e.world().delta_time();
                if (cd.TimeRemaining > 0) return;

                if (IsValid(t.Target))
                {
                    FEnemyData* EnemyData = this->EnemiesDataTable->FindRow<FEnemyData>(a.Value, TEXT("AttackSystem"));
                    if (!EnemyData) return;

                    const float DistSq = FVector::DistSquared(p.Value, t.Target->GetActorLocation());
                    if (DistSq <= FMath::Square(EnemyData->AttackRange))
                    {
                        if (EnemyData->AttackType == E_AttackType::Melee) e.add<FRequestMeleeAttack>();
                        else e.add<FRequestRangedAttack>();

                        cd.TimeRemaining = EnemyData->AttackCooldown;
                    }
                }
            });
}

static void DestroyAndResetQueries(AEnemyManager* Manager)
{
    // Queries MUST be destroyed before deleting world. Use explicit destructor + re-default placement new.
    Manager->CachedPositionQuery.~query<>();
    new (&Manager->CachedPositionQuery) flecs::query<FPosition>();

    Manager->CachedConstPositionQuery.~query<>();
    new (&Manager->CachedConstPositionQuery) flecs::query<const FPosition>();

    Manager->CachedMovementQuery.~query<>();
    new (&Manager->CachedMovementQuery) flecs::query<FVelocity, const FPosition, const FTargetActor, const FEnemyArchetype, const FAttackTimer>();

    Manager->CachedMeleeQuery.~query<>();
    new (&Manager->CachedMeleeQuery) flecs::query<const FPosition, const FEnemyArchetype, const FRequestMeleeAttack>();

    Manager->CachedRangedQuery.~query<>();
    new (&Manager->CachedRangedQuery) flecs::query<const FPosition, const FTargetActor, const FEnemyArchetype, const FRequestRangedAttack>();

    Manager->CachedInstanceTransformQuery.~query<>();
    new (&Manager->CachedInstanceTransformQuery) flecs::query<FPosition, const FInstanceLink, const FEnemyArchetype>();

    Manager->CachedAvoidanceQuery.~query<>();
    new (&Manager->CachedAvoidanceQuery) flecs::query<FPosition, const FAvoidance>();

    Manager->CachedDestructionQuery.~query<>();
    new (&Manager->CachedDestructionQuery) flecs::query<const FInstanceLink, const FEnemyArchetype, const FNeedsDestruction>();
}

void AEnemyManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!ECSWorld) return;

    ++FrameCounter;

    // 1) Apply buffered damage commands
    for (const FDamageCommand& Cmd : DamageCommandBuffer)
    {
        if (Cmd.TargetEntity.is_alive())
        {
            Cmd.TargetEntity.set<FDamageRequest>({ Cmd.DamageAmount });
            Cmd.TargetEntity.set<FKnockbackRequest>({ Cmd.KnockbackForce });
        }
    }
    DamageCommandBuffer.Empty();

    // 2) Melee & Ranged (defer + each)
    ECSWorld->defer([this]()
        {
            // Melee
            CachedMeleeQuery.each([this](flecs::entity e, const FPosition& p, const FEnemyArchetype& a, const FRequestMeleeAttack&)
                {
                    FEnemyData* EnemyData = this->EnemiesDataTable->FindRow<FEnemyData>(a.Value, TEXT(""));
                    if (!EnemyData || !EnemyData->AttackEffect)
                    {
                        e.remove<FRequestMeleeAttack>();
                        return;
                    }

                    TArray<AActor*> OverlappedActors;
                    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
                    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

                    UKismetSystemLibrary::SphereOverlapActors(
                        GetWorld(),
                        p.Value,
                        EnemyData->AttackRange,
                        ObjectTypes,
                        AW4GZCharacter::StaticClass(),
                        TArray<AActor*>(),
                        OverlappedActors
                    );

                    for (AActor* Actor : OverlappedActors)
                    {
                        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor))
                        {
                            if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
                            {
                                FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
                                ContextHandle.AddInstigator(this, this);

                                FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EnemyData->AttackEffect, 1, ContextHandle);
                                if (SpecHandle.IsValid())
                                {
                                    SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), EnemyData->AttackDamage);
                                    TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                                }
                            }
                        }
                    }

                    e.world().defer([e]() { e.remove<FRequestMeleeAttack>(); });
                });

            // Ranged
            CachedRangedQuery.each([this](flecs::entity e, const FPosition& p, const FTargetActor& t, const FEnemyArchetype& a, const FRequestRangedAttack&)
                {
                    if (!IsValid(t.Target))
                    {
                        e.world().defer([e]() { e.remove<FRequestRangedAttack>(); });
                        return;
                    }

                    FEnemyData* EnemyData = this->EnemiesDataTable->FindRow<FEnemyData>(a.Value, TEXT(""));
                    if (!EnemyData || !EnemyData->ProjectileClass)
                    {
                        e.remove<FRequestRangedAttack>();
                        return;
                    }

                    UWorld* World = GetWorld();
                    if (World)
                    {
                        const FVector TargetLocation = t.Target->GetActorLocation();
                        const FVector SpawnLocation = p.Value;
                        const FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();
                        const FRotator Rotation = Direction.Rotation();

                        AEnemyProjectile* SpawnedProjectile = World->SpawnActor<AEnemyProjectile>(EnemyData->ProjectileClass, SpawnLocation, Rotation);
                        (void)SpawnedProjectile;
                    }

                    e.world().defer([e]() { e.remove<FRequestRangedAttack>(); });
                });
        });

    // 3) Movement (each)
    CachedMovementQuery.each([this, DeltaTime](flecs::entity e, FVelocity& Velocity, const FPosition& Position, const FTargetActor& Target, const FEnemyArchetype& Archetype, const FAttackTimer& AttackTimer)
        {
            if (Target.Target && !e.has<FKnockbackRequest>())
            {
                FEnemyData* EnemyData = this->EnemiesDataTable->FindRow<FEnemyData>(Archetype.Value, TEXT(""));
                if (!EnemyData) return;

                const FVector TargetLocation = Target.Target->GetActorLocation();
                const float DistanceToTargetSq = FVector::DistSquared(Position.Value, TargetLocation);

                if (DistanceToTargetSq <= FMath::Square(EnemyData->AttackRange) && AttackTimer.TimeRemaining <= 0.f)
                {
                    Velocity.Value = FVector::ZeroVector;
                }
                else
                {
                    const float Speed = EnemyData->MoveSpeed;
                    const FVector HomingForce = (TargetLocation - Position.Value).GetSafeNormal() * Speed;
                    Velocity.Value = HomingForce;
                }
            }
        });

    // 4) Integrate positions (each)
    CachedPositionQuery.each([DeltaTime](flecs::entity e, FPosition& p)
        {
            FVector DeltaMove = FVector::ZeroVector;
            if (e.has<FVelocity>())
            {
                FVelocity& v = e.get_mut<FVelocity>();
                DeltaMove = v.Value * DeltaTime;
            }

            if (!DeltaMove.IsNearlyZero() && DeltaMove.Size() < 1000.f)
            {
                p.Value += DeltaMove;
            }
        });

    // 5) Heavy updates throttled every 2 frames (collisions + transforms)
    const bool bDoHeavy = (FrameCounter % 2 == 0);
    if (bDoHeavy)
    {
        ResolveCollisions();
    }

    ECSWorld->progress(DeltaTime);

    // 6) deferred: destruction handling
    ECSWorld->defer([this]()
        {
            CachedDestructionQuery.each([this](flecs::entity e, const FInstanceLink& l, const FEnemyArchetype& a, const FNeedsDestruction&)
                {
                    if (EnemyMeshInstances.Contains(a.Value))
                    {
                        if (UHierarchicalInstancedStaticMeshComponent* TargetHISM = EnemyMeshInstances[a.Value])
                        {
                            if (TargetHISM->IsValidInstance(l.Value))
                            {
                                FTransform OffscreenTransform;
                                OffscreenTransform.SetLocation(FVector(0, 0, -200000.f));
                                OffscreenTransform.SetScale3D(FVector::ZeroVector);

                                TargetHISM->UpdateInstanceTransform(l.Value, OffscreenTransform, true, false, true);
                                AvailableInstanceIndices.FindOrAdd(a.Value).Add(l.Value);
                            }
                        }
                    }

                    LastKnownTransforms.Remove(e.id());
                    e.destruct();
                });
        });

    ProcessSpawnQueue();

    if (bDoHeavy)
    {
        UpdateInstanceTransforms(); // per-entity throttle + single MarkRenderStateDirty per HISM
    }
}

void AEnemyManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (ECSWorld)
    {
        DestroyAndResetQueries(this); // destroy queries BEFORE deleting world

        ECSWorld->each([](flecs::entity e) { e.destruct(); });
        delete ECSWorld;
        ECSWorld = nullptr;
    }

    for (auto& Elem : EnemyMeshInstances)
    {
        if (Elem.Value)
        {
            Elem.Value->DestroyComponent();
        }
    }

    EnemyMeshInstances.Empty();
    LastKnownTransforms.Empty();
    PendingSpawns.Empty();
    DamageCommandBuffer.Empty();
    SpatialGrid.Empty();
    AvailableInstanceIndices.Empty();
}

void AEnemyManager::SpawnSingleEnemy(const FName& EnemyID, FVector SpawnLocation, AActor* PlayerTarget)
{
    if (!ECSWorld || !EnemiesDataTable) return;

    FEnemyData* EnemyData = EnemiesDataTable->FindRow<FEnemyData>(EnemyID, TEXT("Spawn"));
    if (!EnemyData)
    {
        UE_LOG(LogTemp, Error, TEXT("HATA: EnemyData '%s' bulunamadi!"), *EnemyID.ToString());
        return;
    }

    UHierarchicalInstancedStaticMeshComponent* TargetHISM = nullptr;

    if (!EnemyMeshInstances.Contains(EnemyID))
    {
        TargetHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, EnemyID);
        if (TargetHISM)
        {
            TargetHISM->SetStaticMesh(EnemyData->EnemyStaticMesh);
            TargetHISM->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

            TargetHISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            TargetHISM->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
            TargetHISM->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
            TargetHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
            TargetHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
            TargetHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);

            TargetHISM->SetMobility(EComponentMobility::Movable);
            TargetHISM->SetCastShadow(true);
            TargetHISM->bCastDynamicShadow = true;
            TargetHISM->bAffectDistanceFieldLighting = true;
            TargetHISM->bAffectDynamicIndirectLighting = true;
            TargetHISM->SetCullDistances(0, 0);
            TargetHISM->SetCanEverAffectNavigation(false);
            TargetHISM->SetVisibility(true);
            TargetHISM->bHiddenInGame = false;

            TargetHISM->RegisterComponent();
            EnemyMeshInstances.Add(EnemyID, TargetHISM);
            TargetHISM->SetBoundsScale(100.f);
        }
    }
    else
    {
        TargetHISM = EnemyMeshInstances[EnemyID].Get();
    }

    if (!TargetHISM) return;

    int32 NewInstanceIndex = INDEX_NONE;
    TArray<int32>& AvailableIndices = AvailableInstanceIndices.FindOrAdd(EnemyID);

    if (AvailableIndices.Num() > 0)
    {
        NewInstanceIndex = AvailableIndices.Pop();
        TargetHISM->UpdateInstanceTransform(NewInstanceIndex, FTransform(SpawnLocation), true, false, true);
        UE_LOG(LogTemp, Log, TEXT("Reusable instance used. Index: %d for EnemyID: %s"), NewInstanceIndex, *EnemyID.ToString());
    }
    else
    {
        NewInstanceIndex = TargetHISM->AddInstance(FTransform(SpawnLocation), true);
        UE_LOG(LogTemp, Log, TEXT("New instance created. Index: %d for EnemyID: %s"), NewInstanceIndex, *EnemyID.ToString());
    }

    if (NewInstanceIndex == INDEX_NONE) return;

    flecs::entity newEntity = ECSWorld->entity()
        .set<FPosition>({ SpawnLocation })
        .set<FVelocity>({ FVector::ZeroVector })
        .set<FHealth>({ EnemyData->Health, EnemyData->Health })
        .set<FTargetActor>({ PlayerTarget })
        .set<FInstanceLink>({ NewInstanceIndex })
        .set<FEnemyArchetype>({ EnemyID })
        .set<FAttackTimer>({ 0.0f })
        .set<FAvoidance>({ 60.f });
    UE_LOG(LogTemp, Warning, TEXT("Entity %llu spawn oldu, InstanceIndex: %d, Pozisyon: %s"), newEntity.id(), NewInstanceIndex, *SpawnLocation.ToString());

    if (!newEntity.has<FHealth>()) UE_LOG(LogTemp, Error, TEXT("HATA: Entity %llu health component olmadan spawn oldu!"), newEntity.id());
}

void AEnemyManager::ApplyPushBackToPlayer(ACharacter* Player)
{
    if (!Player || !ECSWorld) return;

    const FVector PlayerLocation = Player->GetActorLocation();
    const float PushRadiusSq = FMath::Square(150.f);
    const float PushStrength = 300.f;

    FVector TotalPush = FVector::ZeroVector;
    int32 PushCount = 0;

    CachedConstPositionQuery.each([&](flecs::entity e, const FPosition& p)
        {
            float DistSq = FVector::DistSquared(p.Value, PlayerLocation);
            if (DistSq < PushRadiusSq && DistSq > 1.0f)
            {
                FVector PushDir = (PlayerLocation - p.Value).GetSafeNormal();
                TotalPush += PushDir;
                PushCount++;
            }
        });

    if (PushCount > 0)
    {
        FVector AveragePush = (TotalPush / PushCount).GetSafeNormal() * PushStrength * ECSWorld->delta_time();
        FHitResult Hit;
        Player->AddActorWorldOffset(AveragePush, true, &Hit);
    }
}

void AEnemyManager::UpdateInstanceTransforms()
{
    if (!ECSWorld) return;

    // Player location for near/far throttling
    FVector PlayerLoc = FVector::ZeroVector;
    if (ACharacter* PC = UGameplayStatics::GetPlayerCharacter(this, 0))
    {
        PlayerLoc = PC->GetActorLocation();
    }

    // Geçici batch
    TMap<UHierarchicalInstancedStaticMeshComponent*, TArray<TPair<int32, FTransform>>> UpdateBatchMap;

    CachedInstanceTransformQuery.each([this, &UpdateBatchMap, PlayerLoc](flecs::entity e, FPosition& Position, const FInstanceLink& Link, const FEnemyArchetype& Archetype)
        {
            if (e.has<FNeedsDestruction>()) return;
            if (!EnemyMeshInstances.Contains(Archetype.Value)) return;

            // Per-entity throttle: far entities less frequent, hashed phase
            const float DistSq = FVector::DistSquared(Position.Value, PlayerLoc);
            const int32 Stride = (DistSq > kFarDistanceSq) ? kTransformStrideFar : kTransformStrideNear;
            const uint64 Hash = e.id();
            if (((FrameCounter + int32(Hash & 3)) % Stride) != 0)
            {
                return;
            }

            const int32 InstanceIndex = Link.Value;
            FTransform NewTransform;
            NewTransform.SetLocation(Position.Value);
            NewTransform.SetRotation(FQuat::Identity);
            NewTransform.SetScale3D(FVector(1.0f));

            const FVector Loc = NewTransform.GetLocation();
            const FVector Scale = NewTransform.GetScale3D();
            const FQuat Rot = NewTransform.GetRotation();

            bool bValidLoc = FMath::IsFinite(Loc.X) && FMath::IsFinite(Loc.Y) && FMath::IsFinite(Loc.Z);
            bool bValidScale = FMath::IsFinite(Scale.X) && FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z);
            bool bValidRotation = Rot.IsNormalized();
            bool bNonZeroScale = !Scale.IsNearlyZero();

            if (!bValidLoc || !bValidScale || !bValidRotation || !bNonZeroScale)
            {
                UE_LOG(LogTemp, Error, TEXT("⛔ HATALI TRANSFORM! Entity: %llu | Pos=%s | Scale=%s | Rot=%s"),
                    e.id(), *Loc.ToString(), *Scale.ToString(), *Rot.ToString());
                return;
            }

            const FTransform* LastTransform = LastKnownTransforms.Find(e.id());
            if (LastTransform && LastTransform->Equals(NewTransform, 0.001f)) return;

            UHierarchicalInstancedStaticMeshComponent* HISM = EnemyMeshInstances[Archetype.Value];
            TArray<TPair<int32, FTransform>>& List = UpdateBatchMap.FindOrAdd(HISM);
            List.Add({ InstanceIndex, NewTransform });

            LastKnownTransforms.Add(e.id(), NewTransform);
        });

    // Batch apply + single MarkRenderStateDirty per HISM
    for (auto& Elem : UpdateBatchMap)
    {
        UHierarchicalInstancedStaticMeshComponent* HISM = Elem.Key;
        const TArray<TPair<int32, FTransform>>& Instances = Elem.Value;

        for (const TPair<int32, FTransform>& Pair : Instances)
        {
            // bMarkRenderStateDirty = false → senkronizasyon sayısını düşür
            HISM->UpdateInstanceTransform(Pair.Key, Pair.Value, /*bWorldSpace*/true, /*bMarkRenderStateDirty*/false, /*bTeleport*/true);
        }

        // tek seferde render state kirlet
        HISM->MarkRenderStateDirty();
    }
}

void AEnemyManager::ProcessSpawnQueue()
{
    if (PendingSpawns.IsEmpty()) return;

    for (const FActiveSpawnGroup& ActiveGroup : PendingSpawns)
    {
        const FST_EnemyGroup& GroupData = ActiveGroup.GroupData;
        AActor* PlayerTarget = ActiveGroup.PlayerTarget;
        if (!PlayerTarget) continue;

        TArray<FVector> SpawnLocations;
        float SpawnRadius = GroupData.SpawnRadius;

        switch (GroupData.SpawnType)
        {
        case E_SpawnType::Elite:
            SpawnRadius *= 0.5f;
        case E_SpawnType::Standard:
        {
            const USpawnPattern* DefaultPattern = GetDefault<USpawnPattern>();
            SpawnLocations = DefaultPattern->GetSpawnLocations(this, PlayerTarget, GroupData.SpawnCount, SpawnRadius);
            break;
        }
        case E_SpawnType::Scripted:
        {
            if (GroupData.SpawnPattern)
            {
                USpawnPattern* Pattern = NewObject<USpawnPattern>(this, GroupData.SpawnPattern);
                SpawnLocations = Pattern->GetSpawnLocations(this, PlayerTarget, GroupData.SpawnCount, GroupData.SpawnRadius);
            }
            break;
        }
        }

        for (const FVector& Location : SpawnLocations)
        {
            SpawnSingleEnemy(GroupData.EnemyID, Location, PlayerTarget);
        }
    }

    PendingSpawns.Empty();
}

bool AEnemyManager::FindClosestEnemyToLocation(FVector SearchLocation, float SearchRadius, FVector& Out_EnemyLocation)
{
    if (!ECSWorld) return false;
    const float SearchRadiusSq = SearchRadius * SearchRadius;
    flecs::entity closest_entity;
    float min_dist_sq = SearchRadiusSq;
    FPosition closest_pos;

    CachedConstPositionQuery.each([&](flecs::entity e, const FPosition& p)
        {
            float dist_sq = FVector::DistSquared(SearchLocation, p.Value);
            if (dist_sq < min_dist_sq)
            {
                min_dist_sq = dist_sq;
                closest_entity = e;
                closest_pos = p;
            }
        });

    if (closest_entity.is_alive())
    {
        Out_EnemyLocation = closest_pos.Value;
        return true;
    }

    return false;
}

void AEnemyManager::ApplyRadialDamageToECS(FVector Origin, float Radius, float DamageAmount, const AActor* DamageCauser)
{
    if (!ECSWorld || !DamageCauser) return;
    const float RadiusSq = Radius * Radius;
    const FVector CauserLocation = DamageCauser->GetActorLocation();

    CachedConstPositionQuery.each([&](flecs::entity e, const FPosition& p)
        {
            if (FVector::DistSquared(Origin, p.Value) <= RadiusSq)
            {
                const FVector Direction = (p.Value - CauserLocation).GetSafeNormal();
                const float KnockbackStrength = 8000.f;
                const FVector KnockbackForce = Direction * KnockbackStrength;

                FDamageCommand NewCommand;
                NewCommand.TargetEntity = e;
                NewCommand.DamageAmount = DamageAmount;
                NewCommand.KnockbackForce = KnockbackForce;

                DamageCommandBuffer.Add(NewCommand);
            }
        });
}

void AEnemyManager::UpdateSpatialGrid()
{
    SpatialGrid.Empty();
    CachedConstPositionQuery.each([this](flecs::entity e, const FPosition& p)
        {
            FIntVector GridCoords = GetGridCoords(p.Value);
            SpatialGrid.FindOrAdd(GridCoords).Add(e);
        });
}

void AEnemyManager::ResolveCollisions()
{
    UpdateSpatialGrid();

    CachedAvoidanceQuery.each([this](flecs::entity e, FPosition& p, const FAvoidance& a)
        {
            const FIntVector GridPos = GetGridCoords(p.Value);

            for (int32 x = -1; x <= 1; ++x)
            {
                for (int32 y = -1; y <= 1; ++y)
                {
                    const FIntVector CurrentGridPos = GridPos + FIntVector(x, y, 0);
                    if (const TArray<flecs::entity>* Cell = SpatialGrid.Find(CurrentGridPos))
                    {
                        for (const flecs::entity& OtherEntity : *Cell)
                        {
                            if (e == OtherEntity) continue;

                            if (OtherEntity.is_alive() && OtherEntity.has<FPosition>() && OtherEntity.has<FAvoidance>())
                            {
                                FPosition& OtherPos = OtherEntity.get_mut<FPosition>();
                                const FAvoidance& OtherAvoidance = OtherEntity.get<FAvoidance>();

                                const FVector& PosA = p.Value;
                                FVector& PosB = OtherPos.Value;
                                const float OtherRadius = OtherAvoidance.Radius;
                                const float CombinedRadius = a.Radius + OtherRadius;

                                const FVector Delta = PosB - PosA;
                                const float DistanceSq = Delta.SizeSquared();

                                if (DistanceSq > 0 && DistanceSq < FMath::Square(CombinedRadius))
                                {
                                    const float Distance = FMath::Sqrt(DistanceSq);
                                    const float Overlap = 0.5f * (CombinedRadius - Distance);
                                    const FVector Correction = Delta.GetSafeNormal() * Overlap;

                                    p.Value -= Correction;
                                    PosB += Correction;
                                }
                            }
                        }
                    }
                }
            }
        });
}

void AEnemyManager::ApplyDamageToClosestEnemy(FVector WorldLocation, float DamageAmount, const AActor* DamageCauser)
{
    if (!ECSWorld || !DamageCauser) return;

    const float SearchRadiusSq = 3000.f * 3000.f;
    flecs::entity closest_entity;
    FPosition closest_pos;
    float min_dist_sq = SearchRadiusSq;

    CachedConstPositionQuery.each([&](flecs::entity e, const FPosition& p)
        {
            float dist_sq = FVector::DistSquared(WorldLocation, p.Value);
            if (dist_sq < min_dist_sq)
            {
                min_dist_sq = dist_sq;
                closest_entity = e;
                closest_pos = p;
            }
        });

    if (closest_entity.is_alive())
    {
        const FVector CauserLocation = DamageCauser->GetActorLocation();
        const FVector Direction = (closest_pos.Value - CauserLocation).GetSafeNormal();
        if (!Direction.IsNormalized())
        {
            UE_LOG(LogTemp, Error, TEXT("Knockback hatalı yön: Causer=%s, Target=%s"), *CauserLocation.ToString(), *closest_pos.Value.ToString());
            return;
        }
        const float KnockbackStrength = 8000.f;

        DamageCommandBuffer.Add({
            closest_entity,
            DamageAmount,
            Direction * KnockbackStrength
            });
    }
}

void AEnemyManager::AddNewEnemyGroupToSpawner(FST_EnemyGroup EnemyGroup)
{
    ACharacter* PlayerTarget = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (PlayerTarget)
    {
        PendingSpawns.Add({ EnemyGroup, PlayerTarget });
    }
}

AEnemyManager::~AEnemyManager()
{
    if (ECSWorld)
    {
        DestroyAndResetQueries(this);
        ECSWorld->each([](flecs::entity e) { e.destruct(); });
        delete ECSWorld;
        ECSWorld = nullptr;
    }
}
