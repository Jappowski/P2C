#include "P2CMainMenuWidget.h"

#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

void UP2CMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	BindWidgetEvents();
}

void UP2CMainMenuWidget::NativeDestruct()
{
	UnbindWidgetEvents();
	
	Super::NativeDestruct();
}

void UP2CMainMenuWidget::BindWidgetEvents()
{
	if (IsValid(HostGameButton))
	{
		HostGameButton->OnClicked.AddDynamic(this,
			&ThisClass::HandleHostGameClicked);
	}
	
	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleRefreshClicked);
	}

	if (IsValid(QuitButton))
	{
		QuitButton->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleQuitClicked);
	}
}

void UP2CMainMenuWidget::UnbindWidgetEvents()
{
	if (IsValid(HostGameButton))
	{
		HostGameButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleHostGameClicked);
	}

	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleRefreshClicked);
	}

	if (IsValid(QuitButton))
	{
		QuitButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleQuitClicked);
	}
}

void UP2CMainMenuWidget::HandleHostGameClicked()
{
}

void UP2CMainMenuWidget::HandleRefreshClicked()
{
}

void UP2CMainMenuWidget::HandleQuitClicked()
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!IsValid(OwningPlayer))
	{
		return;
	}
	
	UKismetSystemLibrary::QuitGame(this, OwningPlayer, EQuitPreference::Quit, false);
}

