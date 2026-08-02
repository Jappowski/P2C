#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P2CMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UMultiplayerSessionSubsystem;
class UP2CSessionEntryWidget;

UCLASS()
class UP2CMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleHostGameClicked();
	
	UFUNCTION()
	void HandleRefreshClicked();
	
	UFUNCTION()
	void HandleQuitClicked();
	
	void HandleJoinRequested(int32 CachedResultIndex);
	
	UFUNCTION()
	void HandleCreateSessionCompleted(bool bWasSuccessful);
	
	UFUNCTION()
	void HandleFindSessionsCompleted(bool bWasSuccessful, int32 ResultCount);
	
	UFUNCTION()
	void HandleJoinSessionCompleted(bool bWasSuccessful);
	
	void BindWidgetEvents();
	void UnbindWidgetEvents();
	void SetSessionControlsEnabled(bool bEnabled);
	bool CheckSessionValidityAndUpdateText();
	void SetSessionStatusText(const FString& Text);
	void RebuildSessionList();

	UPROPERTY(EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "P2C|Sessions",
		meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 NumPublicConnections = 4;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Sessions",
	meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxSearchResults = 100;
	
	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionSubsystem> SessionSubsystem;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Sessions",
	meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UP2CSessionEntryWidget> SessionEntryWidgetClass;
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> HostGameButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> RefreshButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> SessionContainer;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;
};
