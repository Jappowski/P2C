// Copyright Epic Games, Inc. All Rights Reserved.

#include "P2CGameMode.h"

#include "P2CPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Player/P2CPlayerState.h"

AP2CGameMode::AP2CGameMode()
{
	PlayerControllerClass = AP2CPlayerController::StaticClass();
	PlayerStateClass = AP2CPlayerState::StaticClass();
}

void AP2CGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	if (!IsValid(GameState))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Arena seamless travel completed, but GameState is invalid.")
		);

		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"Arena seamless travel completed. Players: %d"
		),
		GameState->PlayerArray.Num()
	);

	for (APlayerState* BasePlayerState : GameState->PlayerArray)
	{
		const AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(BasePlayerState);

		if (!IsValid(P2CPlayerState))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Unexpected PlayerState class after travel.")
			);

			continue;
		}

		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"Arena player: %s, Ready: %s, PlayerState: %s"
			),
			*P2CPlayerState->GetPlayerName(),
			P2CPlayerState->IsReady()
				? TEXT("true")
				: TEXT("false"),
			*GetNameSafe(P2CPlayerState)
		);
	}
}
