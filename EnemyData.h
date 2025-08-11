#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyProjectile.h"
#include "GameplayEffect.h"
#include "EnemyData.generated.h"



class USpawnPattern;

// Saldýrý tipini belirlemek için Enum
UENUM(BlueprintType)
enum class E_AttackType : uint8
{
    Melee,
    Ranged
};

// YENÝ: Spawn tiplerini belirlemek için Enum
UENUM(BlueprintType)
enum class E_SpawnType : uint8
{
    Standard, // Mevcut rastgele spawn
    Elite,    // Oyuncuya daha yakýn rastgele spawn
    Scripted  // Belirli bir SpawnPattern'e göre
};

// Düþmanýn düþürebileceði tek bir ganimeti tanýmlayan Struct
USTRUCT(BlueprintType)
struct FST_LootDrop : public FTableRowBase
{
    GENERATED_BODY()

    // Düþecek olan toplanabilirin DT_Collectibles'taki satýr adý
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CollectibleID;

    // Bu ganimetin düþme þansý (0.0 ile 1.0 arasýnda)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance = 1.0f;

    // Düþecek minimum miktar/deðer
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MinValue = 1;

    // Düþecek maksimum miktar/deðer
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxValue = 1;
};


// Bir düþman tipinin tüm özelliklerini barýndýran ana Struct
USTRUCT(BlueprintType)
struct FEnemyData : public FTableRowBase
{
    GENERATED_BODY()

public:
    // --- TEMEL STAT'LAR ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Stats")
    float Health = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Stats")
    float MoveSpeed = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Stats")
    TObjectPtr<UStaticMesh> EnemyStaticMesh;

    // --- SALDIRI STAT'LARI ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Stats")
    float AttackDamage = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Stats")
    TSubclassOf<UGameplayEffect> AttackEffect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Stats")
    float AttackRange = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Stats")
    float AttackCooldown = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Stats")
    E_AttackType AttackType = E_AttackType::Melee;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Stats", meta = (EditCondition = "SpawnType == E_AttackType::Ranged"))
    TSubclassOf<AEnemyProjectile> ProjectileClass;

    // --- ÖDÜL STAT'LARI ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
    float ExperienceReward = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
    TArray<FST_LootDrop> LootDrops;
};

USTRUCT(BlueprintType)
struct FST_EnemyGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EnemyID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SpawnCount;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpawnRadius;

    // GÜNCELLENDÝ: Spawn tipini ve desenini belirleyen yeni alanlar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    E_SpawnType SpawnType = E_SpawnType::Standard;

    // Sadece 'Scripted' seçiliyse kullanýlacak olan Spawn Pattern'i
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (EditCondition = "SpawnType == E_SpawnType::Scripted"))
    TSubclassOf<USpawnPattern> SpawnPattern;
};
