#pragma once

#include "CoreMinimal.h"
#include "P2CGameMode.h"
#include "P2CArenaGameMode.generated.h"

class AP2CArenaGameState;
class APlayerController;

UCLASS()
class P2C_API AP2CArenaGameMode : public AP2CGameMode
{
	GENERATED_BODY()

public:
	AP2CArenaGameMode();
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

protected:
	virtual void BeginPlay() override;

private:
	void InitializeArena();
	void RefreshAlivePlayerCount();

	AP2CArenaGameState* GetP2CArenaGameState() const;
};