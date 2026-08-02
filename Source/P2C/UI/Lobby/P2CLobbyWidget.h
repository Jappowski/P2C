#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P2CLobbyWidget.generated.h"

class AP2CLobbyGameState;
class AP2CPlayerController;
class UButton;
class UTextBlock;
class UVerticalBox;
class UP2CLobbyPlayerEntryWidget;

UCLASS(Abstract, Blueprintable)
class P2C_API UP2CLobbyWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void RefreshFromCurrentState();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> PlayerListBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReadyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReadyButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartMatchButton;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Lobby"
	)
	TSubclassOf<UP2CLobbyPlayerEntryWidget> PlayerEntryWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<AP2CLobbyGameState> CachedLobbyGameState;

	UPROPERTY()
	TObjectPtr<AP2CPlayerController> CachedPlayerController;

	UFUNCTION()
	void HandleReadyButtonClicked();

	UFUNCTION()
	void HandleStartMatchButtonClicked();

	UFUNCTION()
	void HandleLobbyPlayersChanged();

	UFUNCTION()
	void HandleLobbyStateChanged();

	void BindLobbyState();
	void UnbindLobbyState();

	void RefreshPlayerList();
	void RefreshControls();

	bool IsLocalPlayerHost() const;
};