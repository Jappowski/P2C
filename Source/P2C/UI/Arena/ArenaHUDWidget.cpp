#include "ArenaHUDWidget.h"

#include "P2CCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Gameplay/Arena/P2CArenaGameState.h"
#include "Player/P2CPlayerState.h"
#include "Player/Components/P2CPlayerStatsComponent.h"

void UP2CArenaHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	BindToGameplaySources();
}

void UP2CArenaHUDWidget::NativeDestruct()
{
	UnbindFromGameplaySources();
	Super::NativeDestruct();
}

void UP2CArenaHUDWidget::BindToGameplaySources()
{
	UnbindFromGameplaySources();
	
	APlayerController* PlayerController = GetOwningPlayer();
	
	if (!IsValid(PlayerController))
	{
		return;
	}
	
	AP2CCharacter* Character = Cast<AP2CCharacter>(PlayerController->GetPawn());
	
	if (IsValid(Character))
	{
		BoundStatsComponent = Character->GetPlayerStatsComponent();

		if (IsValid(BoundStatsComponent))
		{
			BoundStatsComponent->OnStaminaChanged.AddDynamic(
				this,
				&ThisClass::HandleStaminaChanged
			);
		}
	}
	
	BoundPlayerState  = PlayerController->GetPlayerState<AP2CPlayerState>();
	
	if (IsValid(BoundPlayerState))
	{
		BoundPlayerState->OnMatchPointsChanged.AddDynamic(
			this,
			&ThisClass::HandleMatchPointsChanged
		);
	}

	if (UWorld* World = GetWorld())
	{
		BoundArenaGameState = World->GetGameState<AP2CArenaGameState>();
	}

	if (IsValid(BoundArenaGameState))
	{
		BoundArenaGameState->OnAlivePlayerCountChanged.AddDynamic(
			this,
			&ThisClass::HandleAlivePlayerCountChanged
		);

		BoundArenaGameState->OnArenaPhaseChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleArenaPhaseChanged
		);

		BoundArenaGameState->OnRoundWinnerChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleRoundWinnerChanged
		);
		
		BoundPlayerState->OnAliveStateChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleAliveStateChanged
		);
	}
	
	RefreshStamina();
	RefreshMatchPoints();
	RefreshAlivePlayerCount();
	RefreshStaminaVisibility();
}

void UP2CArenaHUDWidget::UnbindFromGameplaySources()
{
	if (IsValid(BoundStatsComponent))
	{
		BoundStatsComponent->OnStaminaChanged.RemoveDynamic(
			this,
			&ThisClass::HandleStaminaChanged
		);
	}
	
	if (IsValid(BoundPlayerState))
	{
		BoundPlayerState->OnMatchPointsChanged.RemoveDynamic(
			this,
			&ThisClass::HandleMatchPointsChanged
		);
		
		BoundPlayerState->OnAliveStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleAliveStateChanged
		);
	}
	
	if (IsValid(BoundArenaGameState))
	{
		BoundArenaGameState->OnAlivePlayerCountChanged.RemoveDynamic(
				this,
				&ThisClass::HandleAlivePlayerCountChanged
			);
	}
	
	BoundStatsComponent = nullptr;
	BoundPlayerState = nullptr;
	BoundArenaGameState = nullptr;
}

void UP2CArenaHUDWidget::RefreshStamina()
{
	if (!IsValid(BoundStatsComponent))
	{
		HandleStaminaChanged(0.0f, 0.0f);
		return;
	}
	
	HandleStaminaChanged(
		BoundStatsComponent->GetCurrentStamina(),
		BoundStatsComponent->GetMaxStamina()
		);
}

void UP2CArenaHUDWidget::RefreshMatchPoints()
{
	const int32 MatchPoints = IsValid(BoundPlayerState)
		? BoundPlayerState->GetMatchPoints()
		: 0;
	
	HandleMatchPointsChanged(MatchPoints);
}

void UP2CArenaHUDWidget::RefreshAlivePlayerCount()
{
	const int32 AlivePlayerCount =
		IsValid(BoundArenaGameState)
			? BoundArenaGameState->GetAlivePlayerCount()
			: 0;

	HandleAlivePlayerCountChanged(AlivePlayerCount);
}

void UP2CArenaHUDWidget::RefreshRoundSummary()
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(RoundSummaryPanel) ||!IsValid(WinnerText))
	{
		return;
	}

	 AP2CArenaGameState* ArenaGameState = World->GetGameState<AP2CArenaGameState>();

	if (!IsValid(ArenaGameState))
	{
		RoundSummaryPanel->SetVisibility(ESlateVisibility::Collapsed);

		return;
	}

	const bool bRoundEnded = ArenaGameState->GetArenaPhase() ==EP2CArenaPhase::RoundEnded;

	RoundSummaryPanel->SetVisibility(
		bRoundEnded
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed
	);

	if (!bRoundEnded)
	{
		return;
	}

	const FString WinnerName = ArenaGameState->GetRoundWinnerName();

	WinnerText->SetText(
		FText::Format(
			NSLOCTEXT(
				"P2C",
				"RoundWinnerFormat",
				"Winner: {0}"
			),
			FText::FromString(
				WinnerName.IsEmpty()
					? TEXT("Unknown")
					: WinnerName
			)
		)
	);
}

void UP2CArenaHUDWidget::RefreshStaminaVisibility()
{
	if (!IsValid(StaminaPanel))
	{
		return;
	}

	const bool bShouldShowStamina = IsValid(BoundPlayerState) && BoundPlayerState->IsAlive();

	StaminaPanel->SetVisibility(
		bShouldShowStamina
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed
	);
}

void UP2CArenaHUDWidget::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
	const float StaminaPercent = MaxStamina > 0.0f
		? CurrentStamina / MaxStamina
		: 0.0f;
	
	if (IsValid(StaminaBar))
	{
		StaminaBar->SetPercent(FMath::Clamp(StaminaPercent, 0.0f, 1.0f));
	}
	
	if (IsValid(StaminaText))
	{
		StaminaText->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("%d / %d"),
					FMath::RoundToInt(CurrentStamina),
					FMath::RoundToInt(MaxStamina)
				)
			)
		);
	}
}

void UP2CArenaHUDWidget::HandleMatchPointsChanged(int32 NewMatchPoints)
{
	if (IsValid(MatchPointsText))
	{
		MatchPointsText->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("Points: %d"),
					NewMatchPoints
				)
			)
		);
	}
}

void UP2CArenaHUDWidget::HandleAlivePlayerCountChanged(int32 NewAlivePlayerCount)
{
	if (!IsValid(AlivePlayerCountText))
	{
		return;
	}
	
	AlivePlayerCountText->SetText(
		FText::FromString(
			FString::Printf(
				TEXT("Players alive: %d"),
				NewAlivePlayerCount
			)
		)
	);
}

void UP2CArenaHUDWidget::HandleArenaPhaseChanged(EP2CArenaPhase NewPhase)
{
	RefreshRoundSummary();
}

void UP2CArenaHUDWidget::HandleRoundWinnerChanged(const FString& WinnerName)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(RoundSummaryPanel) || !IsValid(WinnerText))
	{
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = World->GetGameState<AP2CArenaGameState>();
	if (!IsValid(ArenaGameState))
	{
		RoundSummaryPanel->SetVisibility(ESlateVisibility::Collapsed);

		return;
	}

	const bool bRoundEnded = ArenaGameState->GetArenaPhase() ==EP2CArenaPhase::RoundEnded;

	RoundSummaryPanel->SetVisibility(bRoundEnded
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed
	);
	
	if (!bRoundEnded)
	{
		return;
	}

	WinnerText->SetText(
		FText::Format(
			NSLOCTEXT(
				"P2C",
				"RoundWinnerFormat",
				"Winner: {0}"
			),
			FText::FromString(
				WinnerName.IsEmpty()
					? TEXT("Unknown")
					: WinnerName
			)
		)
	);
}

void UP2CArenaHUDWidget::HandleAliveStateChanged(bool bNewIsAlive)
{
	RefreshStaminaVisibility();
}
