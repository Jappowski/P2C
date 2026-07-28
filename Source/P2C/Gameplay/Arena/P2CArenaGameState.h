#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Gameplay/Arena/P2CArenaTypes.h"
#include "P2CArenaGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FP2CArenaPhaseChangedSignature,
	EP2CArenaPhase,
	NewPhase
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FP2CAlivePlayerCountChangedSignature,
	int32,
	NewAlivePlayerCount
);

UCLASS()
class P2C_API AP2CArenaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AP2CArenaGameState();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintPure, Category = "P2C|Arena")
	EP2CArenaPhase GetArenaPhase();
	
	UFUNCTION(BlueprintPure, Category = "P2C|Arena")
	int32 GetAlivePlayerCount();
	
	UPROPERTY(BlueprintAssignable, Category = "P2C|Arena")
	FP2CArenaPhaseChangedSignature OnArenaPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "P2C|Arena")
	FP2CAlivePlayerCountChangedSignature OnAlivePlayerCountChanged;
	
	/**
	 * Server-only.
	 */
	void SetArenaPhase(EP2CArenaPhase NewPhase);

	/**
	 * Temporary server-side setter.
	 * Later this value will be calculated from AP2CPlayerState::bIsAlive.
	 */
	void SetAlivePlayerCount(int32 NewAlivePlayerCount);
	
private:
	UFUNCTION()
	void OnRep_ArenaPhase();
	
	UFUNCTION()
	void OnRep_AlivePlayerCount();
	
	UPROPERTY(
	   ReplicatedUsing = OnRep_ArenaPhase,
	   VisibleInstanceOnly,
	   Category = "P2C|Arena"
   )
	EP2CArenaPhase ArenaPhase = EP2CArenaPhase::Preparing;
	
	UPROPERTY(
		ReplicatedUsing = OnRep_AlivePlayerCount,
		VisibleInstanceOnly,
		Category = "P2C|Arena"
	)
	int32 AlivePlayerCount = 0;
};

