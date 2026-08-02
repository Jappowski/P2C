#pragma once

#include "CoreMinimal.h"
#include "P2CGameMode.h"
#include "P2CPlayerController.h"
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
	bool TryThrowBomb(AP2CPlayerController* RequestingController) const;
	bool TryPassBombToCharacter(AP2CBomb* Bomb,AP2CCharacter* TargetCharacter) const;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "P2C|Arena", meta = (ClampMin = "0.0"))
	float PreparationDuration = 3.0f;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Bomb",
	meta = (ClampMin = "0.1")
	)
	float MinimumBombFuseDuration = 10.0f;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Bomb",
	meta = (ClampMin = "0.1")
	)
	float MaximumBombFuseDuration = 30.0f;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Arena",
	meta = (ClampMin = "0.0")
	)
	float ExplosionResolutionDuration = 2.0f;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Arena",
	meta = (ClampMin = "0.0")
	)
	float RoundEndDuration = 3.0f;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Arena|Stamina",
	meta = (ClampMin = "1.0")
	)
	float BombThrowStaminaCost = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "P2C|Bomb")
	TSubclassOf<AP2CBomb> BombClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<AP2CBomb> ActiveBomb;
	
	bool bPreparationStarted = false;
	FTimerHandle PreparationTimerHandle;
	FTimerHandle BombFuseTimerHandle;
	FTimerHandle NextBombCycleTimerHandle;
	FTimerHandle ReturnToLobbyTimerHandle;
	
	TWeakObjectPtr<AP2CPlayerController> PendingOverviewPlayerController;
	
	void TryStartPreparation();
	void StartPreparation(AP2CCharacter* InitialHolder);
	void ActiveBombRound();
	void SetAllPlayerMovementEnabled(bool bEnabled);
	void SetCharacterMovementEnabled(AP2CCharacter* Character, bool bEnabled);
	AP2CCharacter* ChooseRandomAliveCharacter();
	
	void InitializeArena();
	void RefreshAlivePlayerCount();

	AP2CArenaGameState* GetP2CArenaGameState() const;

	void StartBombFuse();
	void HandleBombFuseExpired();
	void EliminateCharacter(AP2CCharacter* Character);
	
	void ResolveExplosion();
	void StartNextBombCycle();
	void ResetAlivePlayersForNextCycle();
	void EndArenaRound();
	void ReturnToLobby();
	
	void GatherAliveCharacters(TArray<AP2CCharacter*>& OutCharacters) const;
	void GatherPlayerStarts(TArray<APlayerStart*>& OutPlayerStarts) const;
	void ShufflePlayerStarts(TArray<APlayerStart*>& PlayerStarts) const;
	void ResetAliveCharacter(AP2CCharacter* Character, APlayerStart* PlayerStart);
	void CompletedExplosionResolution();
};