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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FP2CRoundWinnerChangedSignature,
	const FString&,
	NewWinnerName
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
	
	UFUNCTION(BlueprintPure, Category = "P2C|Arena")
	FString GetRoundWinnerName() const;
	
	UPROPERTY(BlueprintAssignable, Category = "P2C|Arena")
	FP2CArenaPhaseChangedSignature OnArenaPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "P2C|Arena")
	FP2CAlivePlayerCountChangedSignature OnAlivePlayerCountChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "P2C|Arena")
	FP2CRoundWinnerChangedSignature OnRoundWinnerChanged;
	
	/**
	 * Server-only.
	 */
	void SetArenaPhase(EP2CArenaPhase NewPhase);

	/**
	 * Temporary server-side setter.
	 * Later this value will be calculated from AP2CPlayerState::bIsAlive.
	 */
	void SetAlivePlayerCount(int32 NewAlivePlayerCount);
	
	/**
	 * Server-only.
	*/
	void SetRoundWinnerName(const FString& NewWinnerName);
	
private:
	UFUNCTION()
	void OnRep_ArenaPhase();
	
	UFUNCTION()
	void OnRep_AlivePlayerCount();
	
	UFUNCTION()
	void OnRep_RoundWinnerName();
	
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
	
	UPROPERTY(
	ReplicatedUsing = OnRep_RoundWinnerName,
	VisibleInstanceOnly,
	Category = "P2C|Arena"
	)
	FString RoundWinnerName;
};

