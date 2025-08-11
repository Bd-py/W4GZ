#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SpawnPattern.generated.h"

UCLASS(Blueprintable, Abstract, EditInlineNew)
class WORKFORGENZ_API USpawnPattern : public UObject
{
    GENERATED_BODY()

public:
    // Bu fonksiyon, bir desenin tüm spawn konumlarýný hesaplar ve bir dizi olarak döndürür.
    // Blueprint'te override edilecek.
    UFUNCTION(BlueprintNativeEvent, Category = "Spawning")
    TArray<FVector> GetSpawnLocations(AActor* Spawner, AActor* PlayerTarget, int32 SpawnCount, float SpawnRadius) const;
};