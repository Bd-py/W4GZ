#include "SpawnPattern.h"

// Bu, Blueprint'te override edilmezse varsayýlan olarak çalýþacak koddur.
TArray<FVector> USpawnPattern::GetSpawnLocations_Implementation(AActor* Spawner, AActor* PlayerTarget, int32 SpawnCount, float SpawnRadius) const
{
    TArray<FVector> Locations;
    if (PlayerTarget)
    {
        const FVector PlayerLocation = PlayerTarget->GetActorLocation();
        for (int32 i = 0; i < SpawnCount; ++i)
        {
            // Varsayýlan olarak, oyuncunun etrafýnda rastgele bir konumda spawn et.
            const FVector Direction = FVector(FMath::FRand() * 2.f - 1.f, FMath::FRand() * 2.f - 1.f, 0.f).GetSafeNormal();
            Locations.Add(PlayerLocation + Direction * SpawnRadius);
        }
    }
    return Locations;
}