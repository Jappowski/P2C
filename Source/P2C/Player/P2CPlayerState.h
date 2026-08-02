#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "P2CPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnLobbyReadyStateChanged,
	bool,
	bIsReady
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDataChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FP2CMatchPointsChangedSignature,
	int32,
	NewMatchPoints
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FP2CAliveStateChangedSignature,
	bool,
	bNewIsAlive
);

UCLASS()
class P2C_API AP2CPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AP2CPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsReady() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Arena")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Arena")
	int32 GetMatchPoints() const;


	void SetReady(bool bNewReady);
	void AddMatchPoints(int32 Points);
	void SetIsAlive(bool bNewIsAlive);
	void ResetArenaState();

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyReadyStateChanged OnReadyStateChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnPlayerDataChanged OnPlayerDataChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "P2C|Arena")
	FP2CMatchPointsChangedSignature OnMatchPointsChanged;

	UPROPERTY(BlueprintAssignable, Category = "P2C|Arena")
	FP2CAliveStateChangedSignature OnAliveStateChanged;

protected:
	UPROPERTY(
		ReplicatedUsing = OnRep_IsReady,
		BlueprintReadOnly,
		Category = "Lobby"
	)
	bool bIsReady = false;
	
	UPROPERTY(ReplicatedUsing = OnRep_MatchPoints,
		VisibleInstanceOnly,
		Category = "P2C|Arena")
	int32 MatchPoints = 0;
	
	UPROPERTY(ReplicatedUsing = OnRep_IsAlive,
		VisibleInstanceOnly,
		Category = "P2C|Arena")
	bool bIsAlive = true;

	UFUNCTION()
	void OnRep_IsReady();

	UFUNCTION()
	void OnRep_MatchPoints();

	UFUNCTION()
	void OnRep_IsAlive();
	
	virtual void OnRep_PlayerName() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
};