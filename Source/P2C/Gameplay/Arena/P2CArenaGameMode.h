#pragma once

#include "CoreMinimal.h"
#include "P2CGameMode.h"
#include "P2CArenaGameMode.generated.h"

class AP2CCharacter;
class AP2CBomb;
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "P2C|Arena", meta = (ClampMin = "0.0"))
	float PreparationDuration = 3.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "P2C|Bomb")
	TSubclassOf<AP2CBomb> BombClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<AP2CBomb> ActiveBomb;
	
	bool bPreparationStarted = false;
	FTimerHandle PreparationTimerHandle;
	
	void TryStartPreparation();
	void StartPreparation(AP2CCharacter* InitialHolder);
	void ActiveBombRound();
	void SetAllPlayerMovementEnabled(bool bEnabled);
	void SetCharacterMovementEnabled(AP2CCharacter* Character, bool bEnabled);
	AP2CCharacter* ChooseRandomAliveCharacter();
	
	void InitializeArena();
	void RefreshAlivePlayerCount();

	AP2CArenaGameState* GetP2CArenaGameState() const;
};