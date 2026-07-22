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

void AP2CPlayerController::BeginPlay()
{
	Super::BeginPlay();

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
	// are we on a mobile platform? Should we force touch?
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

void AP2CPlayerController::LobbySetReady(const bool bReady)
{
	SetLobbyReady(bReady);
}

void AP2CPlayerController::LobbyStartMatch()
{
	RequestStartMatch();
}
