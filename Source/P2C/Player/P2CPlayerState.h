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

UCLASS()
class P2C_API AP2CPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AP2CPlayerState();

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsReady() const;

	void SetReady(bool bNewReady);

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyReadyStateChanged OnReadyStateChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnPlayerDataChanged OnPlayerDataChanged;

protected:
	UPROPERTY(
		ReplicatedUsing = OnRep_IsReady,
		BlueprintReadOnly,
		Category = "Lobby"
	)
	bool bIsReady = false;

	UFUNCTION()
	void OnRep_IsReady();
	virtual void OnRep_PlayerName() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
};