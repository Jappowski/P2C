#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "UObject/Object.h"
#include "P2CConnectionRecoveryService.generated.h"

class UGameInstance;
class UNetDriver;

DECLARE_MULTICAST_DELEGATE(FP2COnConnectionRecoveryRequested);

UCLASS()
class P2C_API UP2CConnectionRecoveryService final : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UGameInstance* InGameInstance);
	void Deinitialize();

	void ResetRecoveryState();

	bool IsRecovering() const
	{
		return bIsRecovering;
	}

	FP2COnConnectionRecoveryRequested OnRecoveryRequested;

private:
	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString);

	void HandleTravelFailure(
		UWorld* World,
		ETravelFailure::Type FailureType,
		const FString& ErrorString);

	bool IsFailureForOwningGameInstance(
		const UWorld* World) const;

	void RequestRecovery(
		const TCHAR* FailureCategory,
		int32 FailureType,
		const FString& ErrorString);

	UPROPERTY(Transient)
	TWeakObjectPtr<UGameInstance> OwningGameInstance;

	FDelegateHandle NetworkFailureDelegateHandle;
	FDelegateHandle TravelFailureDelegateHandle;

	bool bIsRecovering = false;
};