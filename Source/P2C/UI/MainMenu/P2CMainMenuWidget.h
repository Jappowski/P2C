#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P2CMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

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
	
	void BindWidgetEvents();
	void UnbindWidgetEvents();

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
