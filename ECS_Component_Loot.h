#pragma once

#include "CoreMinimal.h"

// Ortak component'leri tekrar kullanabiliriz ama temizlik için ayýralým
struct FLoot_Position { FVector Value; };
struct FLoot_Velocity { FVector Value; };
struct FLoot_InstanceLink { int32 Value; };

// Ganimetin kime doðru çekileceðini tutar
struct FLoot_MagnetTarget { TWeakObjectPtr<AActor> Target; };

// Ganimetin ne olduðu ve ne kadar olduðu
struct FLoot_Data
{
    FName CollectibleID;
    int32 Value;
};

struct FLoot_WasCollected {};