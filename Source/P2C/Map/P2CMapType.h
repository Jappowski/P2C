#pragma once

#include "CoreMinimal.h"
#include "P2CMapType.generated.h"

UENUM(BlueprintType)
enum class EP2CMapType : uint8
{
	MainMenu,
	Lobby,
	Arena
};