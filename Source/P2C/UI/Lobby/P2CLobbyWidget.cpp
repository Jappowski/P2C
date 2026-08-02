#include "UI/Lobby/P2CLobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Lobby/P2CLobbyGameMode.h"
#include "Lobby/P2CLobbyGameState.h"
#include "P2CPlayerController.h"
#include "Player/P2CPlayerState.h"
#include "UI/Lobby/P2CLobbyPlayerEntryWidget.h"

void UP2CLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachedPlayerController = Cast<AP2CPlayerController>(GetOwningPlayer());

	CachedLobbyGameState = GetWorld()
			? GetWorld()->GetGameState<AP2CLobbyGameState>()
			: nullptr;

	if (IsValid(ReadyButton))
	{
		ReadyButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleReadyButtonClicked
		);

		ReadyButton->OnClicked.AddDynamic(
			this,
			&ThisClass::HandleReadyButtonClicked
		);
	}

	if (IsValid(StartMatchButton))
	{
		StartMatchButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleStartMatchButtonClicked
		);

		StartMatchButton->OnClicked.AddDynamic(
			this,
			&ThisClass::HandleStartMatchButtonClicked
		);
	}

	BindLobbyState();

	RefreshPlayerList();
	RefreshControls();
}

void UP2CLobbyWidget::NativeDestruct()
{
	UnbindLobbyState();

	if (IsValid(ReadyButton))
	{
		ReadyButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleReadyButtonClicked
		);
	}

	if (IsValid(StartMatchButton))
	{
		StartMatchButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleStartMatchButtonClicked
		);
	}

	Super::NativeDestruct();
}

void UP2CLobbyWidget::RefreshFromCurrentState()
{
	CachedPlayerController =
		Cast<AP2CPlayerController>(GetOwningPlayer());

	AP2CLobbyGameState* CurrentLobbyGameState =
		GetWorld()
			? GetWorld()->GetGameState<AP2CLobbyGameState>()
			: nullptr;

	if (CachedLobbyGameState != CurrentLobbyGameState)
	{
		UnbindLobbyState();

		CachedLobbyGameState = CurrentLobbyGameState;

		BindLobbyState();
	}

	RefreshPlayerList();
	RefreshControls();
}

void UP2CLobbyWidget::HandleReadyButtonClicked()
{
	if (!IsValid(CachedPlayerController))
	{
		return;
	}

	const AP2CPlayerState* LocalPlayerState = CachedPlayerController->GetPlayerState<AP2CPlayerState>();

	if (!IsValid(LocalPlayerState))
	{
		return;
	}

	CachedPlayerController->SetLobbyReady(!LocalPlayerState->IsReady());
}

void UP2CLobbyWidget::HandleStartMatchButtonClicked()
{
	if (!IsValid(CachedPlayerController))
	{
		return;
	}

	CachedPlayerController->RequestStartMatch();
}

void UP2CLobbyWidget::HandleLobbyPlayersChanged()
{
	RefreshPlayerList();
	RefreshControls();
}

void UP2CLobbyWidget::HandleLobbyStateChanged()
{
	RefreshControls();
}

void UP2CLobbyWidget::BindLobbyState()
{
	if (!IsValid(CachedLobbyGameState))
	{
		return;
	}

	CachedLobbyGameState->OnLobbyPlayersChanged.RemoveDynamic(
		this,
		&ThisClass::HandleLobbyPlayersChanged
	);

	CachedLobbyGameState->OnLobbyPlayersChanged.AddDynamic(
		this,
		&ThisClass::HandleLobbyPlayersChanged
	);

	CachedLobbyGameState->OnLobbyStateChanged.RemoveDynamic(
		this,
		&ThisClass::HandleLobbyStateChanged
	);

	CachedLobbyGameState->OnLobbyStateChanged.AddDynamic(
		this,
		&ThisClass::HandleLobbyStateChanged
	);
}

void UP2CLobbyWidget::UnbindLobbyState()
{
	if (!IsValid(CachedLobbyGameState))
	{
		return;
	}

	CachedLobbyGameState->OnLobbyPlayersChanged.RemoveDynamic(
		this,
		&ThisClass::HandleLobbyPlayersChanged
	);

	CachedLobbyGameState->OnLobbyStateChanged.RemoveDynamic(
		this,
		&ThisClass::HandleLobbyStateChanged
	);

	CachedLobbyGameState = nullptr;
}

void UP2CLobbyWidget::RefreshPlayerList()
{
	if (!IsValid(PlayerListBox) || !IsValid(CachedLobbyGameState))
	{
		return;
	}

	PlayerListBox->ClearChildren();

	if (!PlayerEntryWidgetClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Lobby PlayerEntryWidgetClass is not configured.")
		);

		return;
	}

	for (APlayerState* BasePlayerState : CachedLobbyGameState->PlayerArray)
	{
		AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(BasePlayerState);

		if (!IsValid(P2CPlayerState))
		{
			continue;
		}

		UP2CLobbyPlayerEntryWidget* EntryWidget = CreateWidget<UP2CLobbyPlayerEntryWidget>(
			GetOwningPlayer(),
			PlayerEntryWidgetClass
			);

		if (!IsValid(EntryWidget))
		{
			continue;
		}

		EntryWidget->InitializeFromPlayerState(P2CPlayerState);
		PlayerListBox->AddChild(EntryWidget);
	}
}

void UP2CLobbyWidget::RefreshControls()
{
	if (!IsValid(CachedPlayerController))
	{
		return;
	}

	const AP2CPlayerState* LocalPlayerState = CachedPlayerController->GetPlayerState<AP2CPlayerState>();

	const bool bHasLocalPlayerState = IsValid(LocalPlayerState);

	if (IsValid(ReadyButton))
	{
		ReadyButton->SetIsEnabled(bHasLocalPlayerState);
	}

	if (IsValid(ReadyButtonText))
	{
		const bool bCurrentlyReady = bHasLocalPlayerState && LocalPlayerState->IsReady();

		ReadyButtonText->SetText(bCurrentlyReady
				? FText::FromString(TEXT("Not Ready"))
				: FText::FromString(TEXT("Ready"))
		);
	}

	if (!IsValid(StartMatchButton))
	{
		return;
	}

	const bool bIsHost = IsLocalPlayerHost();

	StartMatchButton->SetVisibility(bIsHost
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed
	);

	if (!bIsHost)
	{
		return;
	}

	const AP2CLobbyGameMode* LobbyGameMode = GetWorld()
			? GetWorld()->GetAuthGameMode<AP2CLobbyGameMode>()
			: nullptr;

	StartMatchButton->SetIsEnabled(IsValid(LobbyGameMode) && LobbyGameMode->CanStartMatch()
	);
}

bool UP2CLobbyWidget::IsLocalPlayerHost() const
{
	return IsValid(CachedPlayerController)
		&& CachedPlayerController->IsLocalController()
		&& CachedPlayerController->HasAuthority();
}