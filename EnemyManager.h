#pragma once

#include "CoreMinimal.h"
#include "EnemyData.h"
#include "GameFramework/Actor.h"
#include "flecs.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ECS_Components.h"
#include "EnemyManager.generated.h"



USTRUCT(BlueprintType)
struct FActiveSpawnGroup
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FST_EnemyGroup GroupData;

    UPROPERTY(BlueprintReadWrite)
    AActor* PlayerTarget;
};

USTRUCT()
struct FDamageCommand
{
    GENERATED_BODY()

    flecs::entity TargetEntity;
    float DamageAmount;
    FVector KnockbackForce;
};

UCLASS()
class WORKFORGENZ_API AEnemyManager : public AActor
{
    GENERATED_BODY()

public:
    AEnemyManager();
    ~AEnemyManager();
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "ECS")
    void AddNewEnemyGroupToSpawner(FST_EnemyGroup EnemyGroup);

    UFUNCTION(BlueprintCallable)
    void ApplyDamageToClosestEnemy(FVector Origin, float DamageAmount, const AActor* DamageCauser);

    void ApplyPushBackToPlayer(ACharacter* Player);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ECS | Queries")
    bool FindClosestEnemyToLocation(FVector SearchLocation, float SearchRadius, FVector& Out_EnemyLocation);

    UFUNCTION(BlueprintCallable, Category = "ECS | Combat")
    void ApplyRadialDamageToECS(FVector Origin, float Radius, float DamageAmount, const AActor* DamageCauser);

    flecs::query<FPosition> CachedPositionQuery;
    flecs::query<const FPosition> CachedConstPositionQuery;
    flecs::query<FVelocity, const FPosition, const FTargetActor, const FEnemyArchetype, const FAttackTimer> CachedMovementQuery;
    flecs::query<const FPosition, const FEnemyArchetype, const FRequestMeleeAttack> CachedMeleeQuery;
    flecs::query<const FPosition, const FTargetActor, const FEnemyArchetype, const FRequestRangedAttack> CachedRangedQuery;
    flecs::query<FPosition, const FInstanceLink, const FEnemyArchetype> CachedInstanceTransformQuery;
    flecs::query<FPosition, const FAvoidance> CachedAvoidanceQuery;
    flecs::query<const FInstanceLink, const FEnemyArchetype, const FNeedsDestruction> CachedDestructionQuery;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // === ECS ===
    flecs::world* ECSWorld = nullptr;

    // Cached Flecs queries (declared as members so we don't recreate per-frame)


    // === Static Mesh Instance Yönetimi ===
    UPROPERTY(VisibleAnywhere)
    TMap<FName, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> EnemyMeshInstances;

    // Performans için: her entity'nin son transform'u saklanıyor
    TMap<flecs::entity_t, FTransform> LastKnownTransforms;

    // === Veriler ===
    UPROPERTY(EditAnywhere, Category = "ECS | Data")
    TObjectPtr<UDataTable> EnemiesDataTable;

    // === Spawn Kontrol ===
    TArray<FActiveSpawnGroup> PendingSpawns;

    TArray<FDamageCommand> DamageCommandBuffer;

    TMap<FIntVector, TArray<flecs::entity>> SpatialGrid;
    int32 GridCellSize = 200;

    TMap<FName, TArray<int32>> AvailableInstanceIndices;

    void UpdateSpatialGrid();
    void ResolveCollisions();
    FIntVector GetGridCoords(const FVector& Location) const;

    // Yardımcı Fonksiyonlar
    void SpawnSingleEnemy(const FName& EnemyID, FVector SpawnLocation, AActor* PlayerTarget);
    void ProcessSpawnQueue();

    void UpdateInstanceTransforms();

    int32 FrameCounter = 0;
};