#include "W4GZ_Character.h"
#include "W4GZ_AbilitySystemComponent.h"
#include "W4GZ_AttributeSet.h"
#include "GameFramework/PlayerState.h"

AW4GZCharacter::AW4GZCharacter()
{
    AbilitySystemComponent = CreateDefaultSubobject<UW4GZAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    AttributeSet = CreateDefaultSubobject<UW4GZAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AW4GZCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AW4GZCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Bu kontrol, AI karakterlerinin ASC'sinin BeginPlay'de güvenli bir þekilde baþlatýlmasýný saðlar.
    // Bu sayede Blueprint'teki BeginPlay'de ASC'yi sorunsuzca kullanabiliriz.
    if (GetLocalRole() == ROLE_Authority && AbilitySystemComponent && !AbilitySystemComponent->GetAvatarActor())
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

void AW4GZCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    InitAbilityActorInfo();
}

void AW4GZCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitAbilityActorInfo();
}

void AW4GZCharacter::InitAbilityActorInfo()
{
    if (!AbilitySystemComponent) { return; }

    // Zaten baþlatýldýysa tekrar baþlatma
    if (AbilitySystemComponent->GetAvatarActor()) { return; }

    APlayerState* PS = GetPlayerState();
    if (PS)
    {
        AbilitySystemComponent->InitAbilityActorInfo(PS, this);
    }
    else
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}