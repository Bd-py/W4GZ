#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "flecs.h"
#include "EnemyData.h" // FST_LootDrop'u içerir
#include "LootManager.generated.h"

class UDataTable;
class UHierarchicalInstancedStaticMeshComponent;

USTRUCT()
struct FLootCollectedCommand
{
	GENERATED_BODY()

	uint64 EntityToDestroyID;
	FName CollectibleID;
	int32 Value;
	int32 InstanceLink;
	TWeakObjectPtr<AActor> Player;
};

UCLASS()
class WORKFORGENZ_API ALootManager : public AActor
{
	GENERATED_BODY()

public:
	ALootManager();
	virtual void Tick(float DeltaTime) override;

	// EnemyManager tarafýndan çaðrýlacak olan ana fonksiyon
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void CreateLootDrop(const TArray<FST_LootDrop>& LootTable, FVector Location);

protected:

	float LootUpdateTimer = 0.f;
	float LootUpdateInterval = 0.2f;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Ganimetleri toplayan oyuncuya ödülü vermek için Blueprint'te oluþturulacak event.
	UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
	void OnCollectiblePickedUp(AActor* Player, FName CollectibleID, int32 Value);

private:
	TUniquePtr<flecs::world> LootWorld;

	// Her bir toplanabilir tipi için ayrý bir HISM tutacak Map
	UPROPERTY()
	TMap<FName, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> CollectibleMeshInstances;

	UPROPERTY(EditAnywhere, Category = "Loot | Data")
	TObjectPtr<UDataTable> CollectiblesDataTable; // DT_Collectibles

	TArray<FLootCollectedCommand> CollectionCommandBuffer;

	TMap<flecs::entity_t, FTransform> LastKnownTransforms;

	TMap<FName, TArray<int32>> AvailableInstancesMap;

	// Hangi instance'larýn gizli olduðunu takip eder (her CollectibleID için)
	TMap<FName, TSet<int32>> HiddenInstancesMap;

	// YENÝ EKLENEN FONKSÝYON TANIMI:
	// Ganimet görsellerini optimize bir þekilde güncelleyecek fonksiyon.
	void UpdateInstanceTransforms();

};