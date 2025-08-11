#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "W4GZ_Character.generated.h"

class UW4GZAbilitySystemComponent;
class UW4GZAttributeSet;

UCLASS()
class WORKFORGENZ_API AW4GZCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AW4GZCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Character|Leveling")
    void OnLevelUp();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    TObjectPtr<UW4GZAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    TObjectPtr<UW4GZAttributeSet> AttributeSet;


    // BeginPlay fonksiyonunu override ediyoruz
    virtual void BeginPlay() override;

    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;

private:
    void InitAbilityActorInfo();
};