#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/Data/P2CSessionSearchResult.h"
#include "P2CSessionEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FP2COnJoinRequested, int32);

UCLASS()
class P2C_API UP2CSessionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FP2CSessionSearchResult& InSessionResult, int32 InCachedResultIndex);
	
	FP2COnJoinRequested OnJoinRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleJoinButtonClicked();
	
	void RefreshDisplayedData();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> HostNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayersText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> PingText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

private:
	FP2CSessionSearchResult SessionResult;
	int32 CachedResultIndex = INDEX_NONE;
};