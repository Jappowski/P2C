#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P2CLobbyPlayerEntryWidget.generated.h"

class AP2CPlayerState;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class P2C_API UP2CLobbyPlayerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeFromPlayerState(
		AP2CPlayerState* InPlayerState
	);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReadyStateText;

private:
	UPROPERTY()
	TObjectPtr<AP2CPlayerState> CachedPlayerState;
	
	UFUNCTION()
	void HandlePlayerDataChanged();

	void RefreshView();
	void UnbindPlayerState();
};