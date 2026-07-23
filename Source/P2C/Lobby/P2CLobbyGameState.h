#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "P2CLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyStateChanged);

UCLASS()
class P2C_API AP2CLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyPlayersChanged OnLobbyPlayersChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyStateChanged OnLobbyStateChanged;

private:
	UFUNCTION()
	void HandlePlayerReadyStateChanged(bool bIsReady);
};