#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "W4GZ_AttributeSet.generated.h"

// Attributes'a C++ ve Blueprint'ten eriþim için makro
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class WORKFORGENZ_API UW4GZAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UW4GZAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // --- DEFANSÝF STAT'LAR ---

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Defensive", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Defensive", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, MaxHealth);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Defensive", ReplicatedUsing = OnRep_HealthRegen)
    FGameplayAttributeData HealthRegen; // Saniye baþýna can yenileme
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, HealthRegen);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Defensive", ReplicatedUsing = OnRep_Armor)
    FGameplayAttributeData Armor; // Alýnan hasarý azaltýr
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, Armor);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Defensive", ReplicatedUsing = OnRep_Evasion)
    FGameplayAttributeData Evasion; // Kaçýnma þansý (0-1 arasý, %0-100)
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, Evasion);

    // --- OFANSÝF STAT'LAR (Sadece Silah) ---

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Offensive", ReplicatedUsing = OnRep_WeaponDamage)
    FGameplayAttributeData WeaponDamage; // Yüzdelik hasar artýþý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, WeaponDamage);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Offensive", ReplicatedUsing = OnRep_WeaponRange)
    FGameplayAttributeData WeaponRange; // Yüzdelik menzil artýþý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, WeaponRange);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Offensive", ReplicatedUsing = OnRep_WeaponArea)
    FGameplayAttributeData WeaponArea; // Yüzdelik alan artýþý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, WeaponArea);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Offensive", ReplicatedUsing = OnRep_WeaponAttackSpeed)
    FGameplayAttributeData WeaponAttackSpeed; // Yüzdelik saldýrý hýzý artýþý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, WeaponAttackSpeed);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Offensive", ReplicatedUsing = OnRep_WeaponPiercing)
    FGameplayAttributeData WeaponPiercing; // Merminin deleceði düþman sayýsý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, WeaponPiercing);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Offensive", ReplicatedUsing = OnRep_WeaponMultistrike)
    FGameplayAttributeData WeaponMultistrike; // Ekstra saldýrý/mermi sayýsý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, WeaponMultistrike);

    // --- KRÝTÝK VURUÞ STAT'LARI ---

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Critical", ReplicatedUsing = OnRep_CriticalHitChance)
    FGameplayAttributeData CriticalHitChance; // Kritik vuruþ þansý (0-1 arasý, %0-100)
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, CriticalHitChance);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Critical", ReplicatedUsing = OnRep_CriticalDamageBonus)
    FGameplayAttributeData CriticalDamageBonus; // Ekstra kritik hasar (Yüzdelik)
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, CriticalDamageBonus);

    // --- DASH STAT'LARI ---

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Dash", ReplicatedUsing = OnRep_DashCharge)
    FGameplayAttributeData DashCharge;
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, DashCharge);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Dash", ReplicatedUsing = OnRep_MaxDashCharge)
    FGameplayAttributeData MaxDashCharge;
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, MaxDashCharge);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Dash", ReplicatedUsing = OnRep_DashCooldown)
    FGameplayAttributeData DashCooldown; // Dash bekleme süresi (Yüzdelik azaltma)
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, DashCooldown);

    // --- UTILITY STAT'LARI ---

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Utility", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed; // Yüzdelik hareket hýzý artýþý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, MoveSpeed);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Utility", ReplicatedUsing = OnRep_AttackMoveSpeed)
    FGameplayAttributeData AttackMoveSpeed; // Saldýrý anýndaki hareket hýzý (Yüzdelik)
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, AttackMoveSpeed);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Utility", ReplicatedUsing = OnRep_ExperienceGain)
    FGameplayAttributeData ExperienceGain; // Yüzdelik EXP artýþý
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, ExperienceGain);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Utility", ReplicatedUsing = OnRep_PickupRange)
    FGameplayAttributeData PickupRange; // Toplama menzili (Yüzdelik)
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, PickupRange);

    // --- SEVÝYE STAT'LARI ---

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Leveling", ReplicatedUsing = OnRep_Level)
    FGameplayAttributeData Level; // Karakterin mevcut seviyesi
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, Level);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Leveling", ReplicatedUsing = OnRep_Experience)
    FGameplayAttributeData Experience; // Mevcut tecrübe puaný
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, Experience);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Leveling", ReplicatedUsing = OnRep_MaxExperience)
    FGameplayAttributeData MaxExperience; // Bir sonraki seviye için gereken tecrübe puaný
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, MaxExperience);

    // --- TUR ÝÇÝ STAT'LAR ---

    UPROPERTY(BlueprintReadOnly, Category = "Attributes | Session", ReplicatedUsing = OnRep_Currency)
    FGameplayAttributeData Currency; // Tur içinde toplanan para
    ATTRIBUTE_ACCESSORS(UW4GZAttributeSet, Currency);


protected:
    // Replikasyon bildirim fonksiyonlarý
    UFUNCTION() virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
    UFUNCTION() virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
    UFUNCTION() virtual void OnRep_HealthRegen(const FGameplayAttributeData& OldHealthRegen);
    UFUNCTION() virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);
    UFUNCTION() virtual void OnRep_Evasion(const FGameplayAttributeData& OldEvasion);
    UFUNCTION() virtual void OnRep_WeaponDamage(const FGameplayAttributeData& OldWeaponDamage);
    UFUNCTION() virtual void OnRep_WeaponRange(const FGameplayAttributeData& OldWeaponRange);
    UFUNCTION() virtual void OnRep_WeaponArea(const FGameplayAttributeData& OldWeaponArea);
    UFUNCTION() virtual void OnRep_WeaponAttackSpeed(const FGameplayAttributeData& OldWeaponAttackSpeed);
    UFUNCTION() virtual void OnRep_WeaponPiercing(const FGameplayAttributeData& OldWeaponPiercing);
    UFUNCTION() virtual void OnRep_WeaponMultistrike(const FGameplayAttributeData& OldWeaponMultistrike);
    UFUNCTION() virtual void OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance);
    UFUNCTION() virtual void OnRep_CriticalDamageBonus(const FGameplayAttributeData& OldCriticalDamageBonus);
    UFUNCTION() virtual void OnRep_DashCharge(const FGameplayAttributeData& OldDashCharge);
    UFUNCTION() virtual void OnRep_MaxDashCharge(const FGameplayAttributeData& OldMaxDashCharge);
    UFUNCTION() virtual void OnRep_DashCooldown(const FGameplayAttributeData& OldDashCooldown);
    UFUNCTION() virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
    UFUNCTION() virtual void OnRep_AttackMoveSpeed(const FGameplayAttributeData& OldAttackMoveSpeed);
    UFUNCTION() virtual void OnRep_ExperienceGain(const FGameplayAttributeData& OldExperienceGain);
    UFUNCTION() virtual void OnRep_PickupRange(const FGameplayAttributeData& OldPickupRange);
    UFUNCTION() virtual void OnRep_Level(const FGameplayAttributeData& OldLevel);
    UFUNCTION() virtual void OnRep_Experience(const FGameplayAttributeData& OldExperience);
    UFUNCTION() virtual void OnRep_MaxExperience(const FGameplayAttributeData& OldMaxExperience);
    UFUNCTION() virtual void OnRep_Currency(const FGameplayAttributeData& OldCurrency);
};