#include "P2CMainMenuWidget.h"

#include "P2CSessionEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Multiplayer/MultiplayerSessionSubsystem.h"

void UP2CMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	BindWidgetEvents();
	
	UGameInstance* GameInstance = GetGameInstance();
	SessionSubsystem = IsValid(GameInstance)
	? GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>()
	: nullptr;
	
	if (!CheckSessionValidityAndUpdateText())
	{
		SetSessionControlsEnabled(false);
		return;
	}
	
	SessionSubsystem->OnCreateSessionCompleted.AddUniqueDynamic(
		this,
		&ThisClass::HandleCreateSessionCompleted);
	
	SessionSubsystem->OnFindSessionsCompleted.AddUniqueDynamic(
		this,
		&ThisClass::HandleFindSessionsCompleted);
	
	SessionSubsystem->OnJoinSessionCompleted.AddUniqueDynamic(
		this,
		&ThisClass::HandleJoinSessionCompleted);
}

void UP2CMainMenuWidget::NativeDestruct()
{
	if (IsValid(SessionSubsystem))
	{
		SessionSubsystem->OnCreateSessionCompleted.RemoveDynamic(
			this,
			&ThisClass::HandleCreateSessionCompleted);
		
		SessionSubsystem->OnFindSessionsCompleted.RemoveDynamic(
			this,
			&ThisClass::HandleFindSessionsCompleted);
		
		SessionSubsystem->OnJoinSessionCompleted.RemoveDynamic(
			this,
			&ThisClass::HandleJoinSessionCompleted);
	}
	SessionSubsystem = nullptr;
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

void UP2CMainMenuWidget::SetSessionControlsEnabled(bool bEnabled)
{
	if (IsValid(HostGameButton))
	{
		HostGameButton->SetIsEnabled(bEnabled);
	}
	
	if (IsValid(RefreshButton))
	{
		RefreshButton->SetIsEnabled(bEnabled);
	}
	
	if (IsValid(SessionContainer))
	{
		SessionContainer->SetIsEnabled(bEnabled);
	}
}

bool UP2CMainMenuWidget::CheckSessionValidityAndUpdateText()
{
	if (!IsValid(SessionSubsystem))
	{
		if (IsValid(StatusText))
		{
			SetSessionStatusText("Session service unavailable.");
		}
		
		return false;
	}
	
	return true;
}

void UP2CMainMenuWidget::SetSessionStatusText(const FString& Text)
{
	StatusText->SetText(FText::FromString(Text));
}

void UP2CMainMenuWidget::RebuildSessionList()
{
	if (!IsValid(SessionContainer))
	{
		return;
	}
	
	SessionContainer->ClearChildren();
	
	if (!IsValid(SessionSubsystem) || !SessionEntryWidgetClass)
	{
		return;
	}
	
	
	const TArray<FP2CSessionSearchResult> SessionResults =
		SessionSubsystem->GetCachedSessionResults();
	
	APlayerController* OwningPlayer = GetOwningPlayer();

	if (!IsValid(OwningPlayer))
	{
		return;
	}
	
	for (int32 CachedResultIndex = 0; CachedResultIndex < SessionResults.Num(); ++CachedResultIndex)
	{
		const FP2CSessionSearchResult& SessionResult = SessionResults[CachedResultIndex];
		
		UP2CSessionEntryWidget* EntryWidget = CreateWidget<UP2CSessionEntryWidget>(
			OwningPlayer,
			SessionEntryWidgetClass);
		
		if (!IsValid(EntryWidget))
		{
			continue;
		}
		
		EntryWidget->Setup(SessionResult, CachedResultIndex);
		EntryWidget->OnJoinRequested.AddUObject(this,&ThisClass::HandleJoinRequested);

		UVerticalBoxSlot* EntrySlot = SessionContainer->AddChildToVerticalBox(EntryWidget);
		
		if (IsValid(EntrySlot))
		{
			EntrySlot->SetHorizontalAlignment(HAlign_Fill);
			EntrySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
	}
}

void UP2CMainMenuWidget::HandleHostGameClicked()
{
	if (!CheckSessionValidityAndUpdateText())
	{
		return;
	}
	
	SetSessionStatusText("Creating session...");
	SetSessionControlsEnabled(false);
	SessionSubsystem->CreateSession(NumPublicConnections);
}

void UP2CMainMenuWidget::HandleRefreshClicked()
{
	if (!CheckSessionValidityAndUpdateText())
	{
		return;
	}
	
	SetSessionStatusText("Looking for sessions...");
	SetSessionControlsEnabled(false);
	SessionSubsystem->FindSessions(MaxSearchResults);
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

void UP2CMainMenuWidget::HandleJoinRequested(int32 CachedResultIndex)
{
	if (!IsValid(SessionSubsystem) || CachedResultIndex == INDEX_NONE)
	{
		if (IsValid(StatusText))
		{
			SetSessionStatusText("Cannot join selected session.");
		}

		return;
	}
	
	SetSessionControlsEnabled(false);
	
	if (IsValid(StatusText))
	{
		SetSessionStatusText("Joining...");
	}

	SessionSubsystem->JoinSession(CachedResultIndex);
}

void UP2CMainMenuWidget::HandleCreateSessionCompleted(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (IsValid(StatusText))
		{
			SetSessionStatusText("Session created. Loading lobby...");
			return;
		}
	}
	
	if (IsValid(StatusText))
	{
		SetSessionStatusText("Failed to create session.");
	}

	SetSessionControlsEnabled(true);
}

void UP2CMainMenuWidget::HandleFindSessionsCompleted(bool bWasSuccessful, int32 ResultCount)
{
	SetSessionControlsEnabled(true);
	
	if (!IsValid(StatusText))
	{
		return;
	}
	
	if (!bWasSuccessful)
	{
		SetSessionStatusText("Failed to find sessions.");
		return;
	}
	
	if (ResultCount <= 0)
	{
		if (IsValid(StatusText))
		{
			SetSessionStatusText("No sessions found.");
		}
		
		if (IsValid(SessionContainer))
		{
			SessionContainer->ClearChildren();
		}
		
		return;
	}
	
	SetSessionStatusText(FText::Format(
			NSLOCTEXT(
				"P2CMainMenu",
				"SessionsFound",
				"Found sessions: {0}"),
			FText::AsNumber(ResultCount)).ToString());

	RebuildSessionList();
}

void UP2CMainMenuWidget::HandleJoinSessionCompleted(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (IsValid(StatusText))
		{
			SetSessionStatusText("Joined. Connecting...");
		}

		return;
	}

	if (IsValid(StatusText))
	{
		SetSessionStatusText("Failed to join session.");
	}

	SetSessionControlsEnabled(true);
}

