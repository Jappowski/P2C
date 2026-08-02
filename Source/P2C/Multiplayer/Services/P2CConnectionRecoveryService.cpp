#include "P2CConnectionRecoveryService.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"

DEFINE_LOG_CATEGORY_STATIC(LogP2CConnectionRecovery, Log, All);

void UP2CConnectionRecoveryService::Initialize(UGameInstance* InGameInstance)
{
    if (!IsValid(InGameInstance))
    {
        UE_LOG(
            LogP2CConnectionRecovery,
            Error,
            TEXT(
                "Cannot initialize connection recovery service: "
                "GameInstance is invalid."));

        return;
    }

    if (NetworkFailureDelegateHandle.IsValid() || TravelFailureDelegateHandle.IsValid())
    {
        UE_LOG(
            LogP2CConnectionRecovery,
            Warning,
            TEXT(
                "Connection recovery service "
                "is already initialized."));

        return;
    }

    if (!GEngine)
    {
        UE_LOG(
            LogP2CConnectionRecovery,
            Error,
            TEXT(
                "Cannot initialize connection recovery service: "
                "GEngine is invalid."));

        return;
    }

    OwningGameInstance = InGameInstance;
    bIsRecovering = false;

    NetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(
            this,
            &UP2CConnectionRecoveryService::
                HandleNetworkFailure);

    TravelFailureDelegateHandle = GEngine->OnTravelFailure().AddUObject(
            this,
            &UP2CConnectionRecoveryService::
                HandleTravelFailure);

    UE_LOG(
        LogP2CConnectionRecovery,
        Log,
        TEXT(
            "Connection recovery service initialized."));
}

void UP2CConnectionRecoveryService::Deinitialize()
{
    if (GEngine)
    {
        if (NetworkFailureDelegateHandle.IsValid())
        {
            GEngine->OnNetworkFailure().Remove(NetworkFailureDelegateHandle);
        }

        if (TravelFailureDelegateHandle.IsValid())
        {
            GEngine->OnTravelFailure().Remove(TravelFailureDelegateHandle);
        }
    }

    NetworkFailureDelegateHandle.Reset();
    TravelFailureDelegateHandle.Reset();

    OnRecoveryRequested.Clear();

    OwningGameInstance.Reset();
    bIsRecovering = false;

    UE_LOG(
        LogP2CConnectionRecovery,
        Log,
        TEXT(
            "Connection recovery service deinitialized."));
}

void UP2CConnectionRecoveryService::ResetRecoveryState()
{
    bIsRecovering = false;

    UE_LOG(
        LogP2CConnectionRecovery,
        Verbose,
        TEXT("Connection recovery state reset."));
}

bool UP2CConnectionRecoveryService::IsFailureForOwningGameInstance(const UWorld* World) const
{
    return IsValid(World)
        && OwningGameInstance.IsValid()
        && World->GetGameInstance() == OwningGameInstance.Get();
}

void UP2CConnectionRecoveryService::HandleNetworkFailure(
    UWorld* World,
    UNetDriver*,
    const ENetworkFailure::Type FailureType,
    const FString& ErrorString)
{
    if (!IsFailureForOwningGameInstance(World))
    {
        return;
    }

    RequestRecovery(TEXT("Network"), FailureType, ErrorString);
}

void UP2CConnectionRecoveryService::HandleTravelFailure(
    UWorld* World,
    const ETravelFailure::Type FailureType,
    const FString& ErrorString)
{
    if (!IsFailureForOwningGameInstance(World))
    {
        return;
    }

    RequestRecovery(
        TEXT("Travel"), FailureType, ErrorString);
}

void UP2CConnectionRecoveryService::RequestRecovery(
    const TCHAR* FailureCategory,
    const int32 FailureType,
    const FString& ErrorString)
{
    if (bIsRecovering)
    {
        UE_LOG(
            LogP2CConnectionRecovery,
            Verbose,
            TEXT(
                "Ignoring additional %s failure during recovery. "
                "Type: %d"),
            FailureCategory,
            FailureType);

        return;
    }

    bIsRecovering = true;

    UE_LOG(
        LogP2CConnectionRecovery,
        Error,
        TEXT("%s failure. Type: %d, Error: %s"),
        FailureCategory,
        FailureType,
        *ErrorString);

    UE_LOG(
        LogP2CConnectionRecovery,
        Log,
        TEXT("Requesting connection recovery."));

    OnRecoveryRequested.Broadcast();
}