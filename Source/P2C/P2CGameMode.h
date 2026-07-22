// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "P2CGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AP2CGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AP2CGameMode();
	virtual void PostSeamlessTravel() override;
};



