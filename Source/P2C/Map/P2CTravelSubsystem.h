#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "P2CMapType.h"
#include "P2CTravelSubsystem.generated.h"

UCLASS()
class P2C_API UP2CTravelSubsystem
	: public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "P2C|Travel")
	bool OpenMap(EP2CMapType MapType) const;

	UFUNCTION(BlueprintCallable, Category = "P2C|Travel")
	bool ServerTravelToMap(
		EP2CMapType MapType,
		bool bListenServer) const;

	bool ClientTravelToAddress(
		const FString& Address) const;

private:
	static TSoftObjectPtr<UWorld> GetMapReference(
		EP2CMapType MapType);
};