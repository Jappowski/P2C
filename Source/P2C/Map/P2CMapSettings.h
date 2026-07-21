#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P2CMapType.h"
#include "P2CMapSettings.generated.h"

class UWorld;

UCLASS(
	Config = Game,
	DefaultConfig,
	meta = (DisplayName = "P2C Maps"))
class P2C_API UP2CMapSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	TSoftObjectPtr<UWorld> GetMap(
		EP2CMapType MapType) const;

private:
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Maps")
	TSoftObjectPtr<UWorld> MainMenuMap;

	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Maps")
	TSoftObjectPtr<UWorld> LobbyMap;

	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Maps")
	TSoftObjectPtr<UWorld> ArenaMap;
};