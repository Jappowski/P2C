// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "P2CPlayerController.generated.h"

class UP2CArenaHUDWidget;
class UInputMappingContext;
class UUserWidget;
class UP2CLobbyWidget;

/**
 * Basic PlayerController class for a third person game.
 * Manages input mappings and lobby requests.
 */
UCLASS()
class P2C_API AP2CPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLobbyReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestStartMatch();

protected:
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Called when PlayerState is replicated to the owning client. */
	virtual void OnRep_PlayerState() override;

	/** Cleans up lobby UI before travelling to another map. */
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts excluded on mobile */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, UMG touch controls are used outside mobile platforms. */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI"
	)
	TSubclassOf<UP2CLobbyWidget> LobbyWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UP2CArenaHUDWidget> ArenaHUDWidgetClass;

	/** Returns true if the player should use UMG touch controls. */
	bool ShouldUseTouchControls() const;

	UFUNCTION(Server, Reliable)
	void ServerSetLobbyReady(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerRequestStartMatch();
	
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;

private:
	UPROPERTY()
	TObjectPtr<UP2CLobbyWidget> LobbyWidget;
	
	UPROPERTY()
	TObjectPtr<UP2CArenaHUDWidget> ArenaHUDWidget;

	void TryCreateLobbyWidget();
	void RemoveLobbyWidget();
	
	void TryCreateArenaHUDWidget();
	void RemoveArenaHUDWidget();
};