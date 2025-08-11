#include "W4GZ_AttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "W4GZ_Character.h"
#include "GameplayEffectExtension.h"

UW4GZAttributeSet::UW4GZAttributeSet() {}

void UW4GZAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Tüm yeni stat'larý replikasyon için ekliyoruz
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, Armor, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, Evasion, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, WeaponDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, WeaponRange, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, WeaponArea, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, WeaponAttackSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, WeaponPiercing, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, WeaponMultistrike, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, CriticalDamageBonus, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, DashCharge, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, MaxDashCharge, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, DashCooldown, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, AttackMoveSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, ExperienceGain, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, PickupRange, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, Level, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, Experience, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, MaxExperience, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UW4GZAttributeSet, Currency, COND_None, REPNOTIFY_Always);
}

void UW4GZAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
    }
    else if (Data.EvaluatedData.Attribute == GetDashChargeAttribute())
    {
        SetDashCharge(FMath::Clamp(GetDashCharge(), 0.0f, GetMaxDashCharge()));
    }
    else if (Data.EvaluatedData.Attribute == GetEvasionAttribute())
    {
        // Kaçýnma þansýný %0 ile %80 arasýnda sýnýrla (0.0 - 0.8)
        SetEvasion(FMath::Clamp(GetEvasion(), 0.0f, 0.8f));
    }
    else if (Data.EvaluatedData.Attribute == GetCriticalHitChanceAttribute())
    {
        // Kritik vuruþ þansýný %0 ile %100 arasýnda sýnýrla (0.0 - 1.0)
        SetCriticalHitChance(FMath::Clamp(GetCriticalHitChance(), 0.0f, 1.0f));
    }
    else if (Data.EvaluatedData.Attribute == GetCriticalDamageBonusAttribute())
    {
        // Kritik hasar bonusunu %0 ile %300 arasýnda sýnýrla (0.0 - 3.0)
        SetCriticalDamageBonus(FMath::Clamp(GetCriticalDamageBonus(), 0.0f, 3.0f));
    }

    else if (Data.EvaluatedData.Attribute == GetExperienceAttribute())
    {
        // Mevcut EXP, gereken EXP'den fazla veya eþit olduðu sürece döngüye gir. (Çoklu seviye atlama için)
        while (GetExperience() >= GetMaxExperience())
        {
            const float LeftoverExp = GetExperience() - GetMaxExperience(); // Artan EXP'yi sakla

            SetLevel(GetLevel() + 1.0f); // Seviyeyi 1 artýr

            // Bir sonraki seviye için gereken EXP'yi artýr (örn: %20 daha fazla)
            const float NewMaxExperience = GetMaxExperience() * 1.2f;
            SetMaxExperience(NewMaxExperience);

            SetExperience(LeftoverExp); // EXP'yi artan miktara ayarla

            // Seviye atlama olayýný tetiklemek için karakteri al
            if (Data.Target.GetAvatarActor())
            {
                AW4GZCharacter* TargetCharacter = Cast<AW4GZCharacter>(Data.Target.GetAvatarActor());
                if (TargetCharacter)
                {
                    TargetCharacter->OnLevelUp(); // Blueprint'te oluþturacaðýmýz OnLevelUp eventini çaðýr
                }
            }
        }
    }
}

// Tüm OnRep fonksiyonlarýnýn implementasyonu
void UW4GZAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, Health, OldHealth); }
void UW4GZAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, MaxHealth, OldMaxHealth); }
void UW4GZAttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldHealthRegen) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, HealthRegen, OldHealthRegen); }
void UW4GZAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, Armor, OldArmor); }
void UW4GZAttributeSet::OnRep_Evasion(const FGameplayAttributeData& OldEvasion) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, Evasion, OldEvasion); }
void UW4GZAttributeSet::OnRep_WeaponDamage(const FGameplayAttributeData& OldWeaponDamage) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, WeaponDamage, OldWeaponDamage); }
void UW4GZAttributeSet::OnRep_WeaponRange(const FGameplayAttributeData& OldWeaponRange) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, WeaponRange, OldWeaponRange); }
void UW4GZAttributeSet::OnRep_WeaponArea(const FGameplayAttributeData& OldWeaponArea) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, WeaponArea, OldWeaponArea); }
void UW4GZAttributeSet::OnRep_WeaponAttackSpeed(const FGameplayAttributeData& OldWeaponAttackSpeed) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, WeaponAttackSpeed, OldWeaponAttackSpeed); }
void UW4GZAttributeSet::OnRep_WeaponPiercing(const FGameplayAttributeData& OldWeaponPiercing) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, WeaponPiercing, OldWeaponPiercing); }
void UW4GZAttributeSet::OnRep_WeaponMultistrike(const FGameplayAttributeData& OldWeaponMultistrike) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, WeaponMultistrike, OldWeaponMultistrike); }
void UW4GZAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, CriticalHitChance, OldCriticalHitChance); }
void UW4GZAttributeSet::OnRep_CriticalDamageBonus(const FGameplayAttributeData& OldCriticalDamageBonus) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, CriticalDamageBonus, OldCriticalDamageBonus); }
void UW4GZAttributeSet::OnRep_DashCharge(const FGameplayAttributeData& OldDashCharge) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, DashCharge, OldDashCharge); }
void UW4GZAttributeSet::OnRep_MaxDashCharge(const FGameplayAttributeData& OldMaxDashCharge) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, MaxDashCharge, OldMaxDashCharge); }
void UW4GZAttributeSet::OnRep_DashCooldown(const FGameplayAttributeData& OldDashCooldown) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, DashCooldown, OldDashCooldown); }
void UW4GZAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, MoveSpeed, OldMoveSpeed); }
void UW4GZAttributeSet::OnRep_AttackMoveSpeed(const FGameplayAttributeData& OldAttackMoveSpeed) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, AttackMoveSpeed, OldAttackMoveSpeed); }
void UW4GZAttributeSet::OnRep_ExperienceGain(const FGameplayAttributeData& OldExperienceGain) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, ExperienceGain, OldExperienceGain); }
void UW4GZAttributeSet::OnRep_PickupRange(const FGameplayAttributeData& OldPickupRange) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, PickupRange, OldPickupRange); }
void UW4GZAttributeSet::OnRep_Level(const FGameplayAttributeData& OldLevel) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, Level, OldLevel); }
void UW4GZAttributeSet::OnRep_Experience(const FGameplayAttributeData& OldExperience) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, Experience, OldExperience); }
void UW4GZAttributeSet::OnRep_MaxExperience(const FGameplayAttributeData& OldMaxExperience) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, MaxExperience, OldMaxExperience); }
void UW4GZAttributeSet::OnRep_Currency(const FGameplayAttributeData& OldCurrency) { GAMEPLAYATTRIBUTE_REPNOTIFY(UW4GZAttributeSet, Currency, OldCurrency); }