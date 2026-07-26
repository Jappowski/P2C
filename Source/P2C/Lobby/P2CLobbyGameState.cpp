#include "Lobby/P2CLobbyGameState.h"
#include "Player/P2CPlayerState.h"


void AP2CLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(PlayerState);

	if (IsValid(P2CPlayerState))
	{
		P2CPlayerState->OnReadyStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandlePlayerReadyStateChanged
		);

		P2CPlayerState->OnReadyStateChanged.AddDynamic(
			this,
			&ThisClass::HandlePlayerReadyStateChanged
		);
	}

	OnLobbyPlayersChanged.Broadcast();
	OnLobbyStateChanged.Broadcast();
}

void AP2CLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(PlayerState);

	if (IsValid(P2CPlayerState))
	{
		P2CPlayerState->OnReadyStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandlePlayerReadyStateChanged
		);
	}

	Super::RemovePlayerState(PlayerState);

	OnLobbyPlayersChanged.Broadcast();
	OnLobbyStateChanged.Broadcast();
}

void AP2CLobbyGameState::HandlePlayerReadyStateChanged(const bool bIsReady)
{
	OnLobbyStateChanged.Broadcast();
}