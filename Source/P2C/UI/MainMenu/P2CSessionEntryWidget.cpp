#include "UI/MainMenu/P2CSessionEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"

void UP2CSessionEntryWidget::Setup(const FP2CSessionSearchResult& InSessionResult, int32 InCachedResultIndex)
{
	SessionResult = InSessionResult;
	CachedResultIndex = InCachedResultIndex;
	RefreshDisplayedData();
}

void UP2CSessionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleJoinButtonClicked);
	}
}

void UP2CSessionEntryWidget::NativeDestruct()
{
	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.RemoveDynamic(
			this,
			&ThisClass::HandleJoinButtonClicked);
	}

	Super::NativeDestruct();
}

void UP2CSessionEntryWidget::HandleJoinButtonClicked()
{
	if (CachedResultIndex == INDEX_NONE)
	{
		return;
	}

	OnJoinRequested.Broadcast(CachedResultIndex);
}

void UP2CSessionEntryWidget::RefreshDisplayedData()
{
	if (IsValid(HostNameText))
	{
		HostNameText->SetText(FText::FromString(SessionResult.HostName));
	}

	if (IsValid(PlayersText))
	{
		PlayersText->SetText(
			FText::Format(
				NSLOCTEXT(
					"P2CSessionEntry",
					"PlayerCount",
					"{0}/{1}"),
				FText::AsNumber(SessionResult.CurrentPlayers),
				FText::AsNumber(SessionResult.MaxPlayers)));
	}

	if (IsValid(PingText))
	{
		PingText->SetText(
			FText::Format(
				NSLOCTEXT(
					"P2CSessionEntry",
					"Ping",
					"{0} ms"),
				FText::AsNumber(SessionResult.PingInMs)));
	}
}
