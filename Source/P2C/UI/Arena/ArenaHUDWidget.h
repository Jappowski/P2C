#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Arena/P2CArenaTypes.h"

#include "ArenaHUDWidget.generated.h"

class AP2CArenaGameState;
class AP2CPlayerState;
class UP2CPlayerStatsComponent;
class UProgressBar;
class UTextBlock;

UCLASS(Abstract)
class P2C_API UP2CArenaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "P2C|Arena HUD")
	void BindToGameplaySources();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
private:
	void UnbindFromGameplaySources();
	
	void RefreshStamina();
	void RefreshMatchPoints();
	void RefreshAlivePlayerCount();
	void RefreshRoundSummary();
	
	UFUNCTION()
	void HandleStaminaChanged(float CurrentStamina, float MaxStamina);
	
	UFUNCTION()
	void HandleMatchPointsChanged(int32 NewMatchPoints);
	
	UFUNCTION()
	void HandleAlivePlayerCountChanged(int32 NewAlivePlayerCount);
	
	UFUNCTION()
	void HandleArenaPhaseChanged(EP2CArenaPhase NewPhase);

	UFUNCTION()
	void HandleRoundWinnerChanged(const FString& WinnerName);
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> StaminaBar;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StaminaText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchPointsText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AlivePlayerCountText;
	
	UPROPERTY(Transient)
	TObjectPtr<UP2CPlayerStatsComponent> BoundStatsComponent;
	
	UPROPERTY(Transient)
	TObjectPtr<AP2CPlayerState> BoundPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<AP2CArenaGameState> BoundArenaGameState;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> RoundSummaryPanel;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WinnerText;
};