#include "UI/Lobby/P2CLobbyPlayerEntryWidget.h"

#include "Components/TextBlock.h"
#include "Player/P2CPlayerState.h"

void UP2CLobbyPlayerEntryWidget::InitializeFromPlayerState(AP2CPlayerState* InPlayerState)
{
	if (CachedPlayerState == InPlayerState)
	{
		RefreshView();
		return;
	}

	UnbindPlayerState();

	CachedPlayerState = InPlayerState;

	if (IsValid(CachedPlayerState))
	{
		CachedPlayerState->OnPlayerDataChanged.RemoveDynamic(
			this,
			&ThisClass::HandlePlayerDataChanged
);

		CachedPlayerState->OnPlayerDataChanged.AddDynamic(
			this,
			&ThisClass::HandlePlayerDataChanged
		);
	}

	RefreshView();
}

void UP2CLobbyPlayerEntryWidget::NativeDestruct()
{
	UnbindPlayerState();

	Super::NativeDestruct();
}

void UP2CLobbyPlayerEntryWidget::HandlePlayerDataChanged()
{
	RefreshView();
}

void UP2CLobbyPlayerEntryWidget::RefreshView()
{
	if (!IsValid(CachedPlayerState))
	{
		return;
	}

	if (IsValid(PlayerNameText))
	{
		PlayerNameText->SetText(FText::FromString(CachedPlayerState->GetPlayerName())
		);
	}

	if (IsValid(ReadyStateText))
	{
		ReadyStateText->SetText(
			CachedPlayerState->IsReady()
				? FText::FromString(TEXT("Ready"))
				: FText::FromString(TEXT("Not Ready"))
		);
	}
}

void UP2CLobbyPlayerEntryWidget::UnbindPlayerState()
{
	if (!IsValid(CachedPlayerState))
	{
		return;
	}

	CachedPlayerState->OnPlayerDataChanged.RemoveDynamic(
		this,
		&ThisClass::HandlePlayerDataChanged
	);

	CachedPlayerState = nullptr;
}