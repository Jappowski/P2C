#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "P2CLobbyGameMode.generated.h"

class APlayerController;

UCLASS()
class P2C_API AP2CLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AP2CLobbyGameMode();

	/**
	 * Called only on the server through the requesting PlayerController.
	*/
	bool TryStartMatch(APlayerController* RequestingController);
	
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool CanStartMatch() const;

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool AreAllPlayersReady() const;

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* ExitingController) override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Lobby",
		meta = (ClampMin = "1")
	)
	int32 MinimumPlayersToStart = 2;

private:
	bool IsHostController(
		const APlayerController* RequestingController
	) const;
	
	bool bMatchTravelStarted = false;
};