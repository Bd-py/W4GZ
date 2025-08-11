#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "CollectibleData.generated.h"

USTRUCT(BlueprintType)
struct FCollectibleData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag GrantOnPickupTag; // Toplandýðýnda verilecek etiket (örn: Data.Gain.Experience)
};