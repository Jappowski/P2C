#include "P2CTravelSubsystem.h"

#include "P2CMapSettings.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogP2CTravel, Log, All);

TSoftObjectPtr<UWorld>
UP2CTravelSubsystem::GetMapReference(const EP2CMapType MapType)
{
	const UP2CMapSettings* MapSettings = GetDefault<UP2CMapSettings>();

	if (!IsValid(MapSettings))
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("P2C map settings are invalid."));

		return nullptr;
	}

	return MapSettings->GetMap(MapType);
}

bool UP2CTravelSubsystem::OpenMap(const EP2CMapType MapType) const
{
	const TSoftObjectPtr<UWorld> MapReference = GetMapReference(MapType);

	if (MapReference.IsNull())
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("Cannot open map: map reference is empty."));

		return false;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("Cannot open map: World is invalid."));

		return false;
	}

	UE_LOG(
		LogP2CTravel,
		Log,
		TEXT("Opening map: %s"),
		*MapReference.ToSoftObjectPath().ToString());

	UGameplayStatics::OpenLevelBySoftObjectPtr(
		World,
		MapReference,
		true);

	return true;
}

bool UP2CTravelSubsystem::ServerTravelToMap(const EP2CMapType MapType,const bool bListenServer) const
{
	const TSoftObjectPtr<UWorld> MapReference = GetMapReference(MapType);

	if (MapReference.IsNull())
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("Cannot server travel: map reference is empty."));

		return false;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("Cannot server travel: World is invalid."));

		return false;
	}

	FString TravelUrl = MapReference.ToSoftObjectPath().GetLongPackageName();

	if (TravelUrl.IsEmpty())
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("Cannot server travel: package name is empty."));

		return false;
	}

	if (bListenServer)
	{
		TravelUrl += TEXT("?listen");
	}

	UE_LOG(
		LogP2CTravel,
		Log,
		TEXT("Server travelling to: %s"),
		*TravelUrl);

	return World->ServerTravel(TravelUrl);
}

bool UP2CTravelSubsystem::ClientTravelToAddress(const FString& Address) const
{
	if (Address.IsEmpty())
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("Cannot client travel: address is empty."));

		return false;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT("Cannot client travel: World is invalid."));

		return false;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();

	if (!IsValid(PlayerController))
	{
		UE_LOG(
			LogP2CTravel,
			Error,
			TEXT(
				"Cannot client travel: "
				"PlayerController is invalid."));

		return false;
	}

	UE_LOG(
		LogP2CTravel,
		Log,
		TEXT("Client travelling to address: %s"),
		*Address);

	PlayerController->ClientTravel(
		Address,
		ETravelType::TRAVEL_Absolute);

	return true;
}
