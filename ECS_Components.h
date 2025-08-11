#pragma once

#include "CoreMinimal.h"

// --- TEMEL COMPONENT'LER ---
struct FPosition { FVector Value; };
struct FVelocity { FVector Value; };
struct FHealth { float CurrentHealth; float MaxHealth; };
struct FInstanceLink { int32 Value; };
struct FTargetActor { TObjectPtr<AActor> Target; };
struct FEnemyArchetype { FName Value; };
struct FAttackTimer { float TimeRemaining; };

// --- EMÝR (COMMAND) COMPONENT'LERÝ ---
struct FNeedsDestruction {};
struct FDamageRequest { float Amount; };
struct FKnockbackRequest { FVector Force; };
struct FRequestMeleeAttack {};
struct FRequestRangedAttack {};
struct FAvoidance { float Radius = 50.f;; };
