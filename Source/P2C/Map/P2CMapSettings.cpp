#include "P2CMapSettings.h"

FName UP2CMapSettings::GetCategoryName() const
{
	return TEXT("Game");
}

TSoftObjectPtr<UWorld> UP2CMapSettings::GetMap(
	const EP2CMapType MapType) const
{
	switch (MapType)
	{
	case EP2CMapType::MainMenu:
		return MainMenuMap;

	case EP2CMapType::Lobby:
		return LobbyMap;

	case EP2CMapType::Arena:
		return ArenaMap;

	default:
		return nullptr;
	}
}