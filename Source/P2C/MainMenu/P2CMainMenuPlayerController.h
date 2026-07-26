#pragma once
#include "UI/MainMenu/P2CMainMenuWidget.h"

#include "P2CMainMenuPlayerController.generated.h"

UCLASS()
class P2C_API AP2CMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MainMenu")
	TSubclassOf<UP2CMainMenuWidget> MainMenuWidgetClass;
private:
	void CreateMainMenu();
	void RemoveMainMenu();
	
	UPROPERTY(Transient)
	TObjectPtr<UP2CMainMenuWidget> MainMenuWidget;
};
