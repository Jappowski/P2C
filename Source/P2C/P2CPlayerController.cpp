// Copyright Epic Games, Inc. All Rights Reserved.


#include "P2CPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "P2C.h"
#include "Lobby/P2CLobbyGameMode.h"
#include "Player/P2CPlayerState.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Lobby/P2CLobbyGameState.h"
#include "UI/Lobby/P2CLobbyWidget.h"


void AP2CPlayerController::BeginPlay()
{
	Super::BeginPlay();
	TryCreateLobbyWidget();
	
	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogP2C, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AP2CPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	TryCreateLobbyWidget();
	if (IsValid(LobbyWidget))
	{
		LobbyWidget->RefreshFromCurrentState();
	}
}

void AP2CPlayerController::PreClientTravel(const FString& PendingURL, const ETravelType TravelType, const bool bIsSeamlessTravel)
{
	RemoveLobbyWidget();

	Super::PreClientTravel(
		PendingURL,
		TravelType,
		bIsSeamlessTravel
	);
}

void AP2CPlayerController::TryCreateLobbyWidget()
{
	if (!IsLocalController() || IsValid(LobbyWidget))
	{
		return;
	}

	const AP2CLobbyGameState* LobbyGameState = GetWorld()
			? GetWorld()->GetGameState<AP2CLobbyGameState>()
			: nullptr;

	if (!IsValid(LobbyGameState))
	{
		return;
	}

	if (!LobbyWidgetClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("LobbyWidgetClass is not configured.")
		);

		return;
	}

	LobbyWidget = CreateWidget<UP2CLobbyWidget>(this, LobbyWidgetClass);

	if (!IsValid(LobbyWidget))
	{
		return;
	}

	LobbyWidget->AddToPlayerScreen();

	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(LobbyWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);
}

void AP2CPlayerController::RemoveLobbyWidget()
{
	if (IsValid(LobbyWidget))
	{
		LobbyWidget->RemoveFromParent();
		LobbyWidget = nullptr;
	}

	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AP2CPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AP2CPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AP2CPlayerController::SetLobbyReady(const bool bReady)
{
	if (!IsLocalController())
	{
		return;
	}

	ServerSetLobbyReady(bReady);
}

void AP2CPlayerController::ServerSetLobbyReady_Implementation(const bool bReady)
{
	AP2CPlayerState* P2CPlayerState = GetPlayerState<AP2CPlayerState>();

	if (!IsValid(P2CPlayerState))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Cannot set ready state: invalid P2CPlayerState.")
		);

		return;
	}

	P2CPlayerState->SetReady(bReady);
}

void AP2CPlayerController::RequestStartMatch()
{
	if (!IsLocalController())
	{
		return;
	}

	ServerRequestStartMatch();
}

void AP2CPlayerController::ServerRequestStartMatch_Implementation()
{
	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	AP2CLobbyGameMode* LobbyGameMode =World->GetAuthGameMode<AP2CLobbyGameMode>();

	if (!IsValid(LobbyGameMode))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Start match rejected: LobbyGameMode is unavailable.")
		);

		return;
	}

	LobbyGameMode->TryStartMatch(this);
}
