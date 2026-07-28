
#include "P2CMainMenuPlayerController.h"

#include "P2C.h"
#include "Blueprint/UserWidget.h"
#include "UI/MainMenu/P2CMainMenuWidget.h"

void AP2CMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsLocalController())
	{
		return;
	}
	
	CreateMainMenu();
}

void AP2CMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveMainMenu();
	Super::EndPlay(EndPlayReason);
}

void AP2CMainMenuPlayerController::CreateMainMenu()
{
	if (MainMenuWidget || !MainMenuWidgetClass)
	{
		return;
	}
	
	MainMenuWidget = CreateWidget<UP2CMainMenuWidget>(this, MainMenuWidgetClass);
	if (!MainMenuWidget)
	{
		UE_LOG(LogP2C, Error,  TEXT("Failed to create Main Menu widget in %s."),
			*GetNameSafe(this));
		return;
	}
	
	MainMenuWidget->AddToViewport();
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	
	SetInputMode(InputMode);
}

void AP2CMainMenuPlayerController::RemoveMainMenu()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}
	
	bShowMouseCursor = false;
}
