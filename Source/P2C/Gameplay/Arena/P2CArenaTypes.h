#pragma once

#include "CoreMinimal.h"
#include "P2CArenaTypes.generated.h"

UENUM(BlueprintType)
enum class EP2CArenaPhase : uint8
{
	Preparing UMETA(DisplayName = "Preparing"),
	BombActive UMETA(DisplayName = "Bomb Active"),
	ResolvingExplosion UMETA(DisplayName = "Resolving Explosion"),
	RoundEnded UMETA(DisplayName = "Round Ended")
};