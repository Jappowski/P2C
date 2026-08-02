#include "MultiplayerSessionSubsystem.h"
#include "Map/P2CTravelSubsystem.h"
#include "Services/P2CConnectionRecoveryService.h"
#include "Services/P2COnlineSessionService.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerSessions, Log, All);

void UMultiplayerSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!InitializeOnlineSessionService())
    {
        return;
    }

    if (!InitializeConnectionRecoveryService())
    {
        DeinitializeOnlineSessionService();
        return;
    }

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("Multiplayer session subsystem initialized."));
}

bool UMultiplayerSessionSubsystem::InitializeOnlineSessionService(){
    OnlineSessionService =NewObject<UP2COnlineSessionService>(this);

    if (!IsValid(OnlineSessionService))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Could not create "
                "OnlineSessionService."));

        return false;
    }

    OnlineSessionService->OnCreateSessionCompleted
        .AddUObject(
            this,
            &UMultiplayerSessionSubsystem::HandleOnlineCreateSessionCompleted);

    OnlineSessionService->OnFindSessionsCompleted
    .AddUObject(
            this,
            &UMultiplayerSessionSubsystem::HandleOnlineFindSessionsCompleted);

    OnlineSessionService->OnJoinSessionCompleted
        .AddUObject(
            this,
            &UMultiplayerSessionSubsystem::HandleOnlineJoinSessionCompleted);

    OnlineSessionService->OnDestroySessionCompleted
        .AddUObject(
            this,
            &UMultiplayerSessionSubsystem::HandleOnlineDestroySessionCompleted);

    OnlineSessionService->Initialize(GetGameInstance());
    return true;
}

bool UMultiplayerSessionSubsystem::InitializeConnectionRecoveryService()
{
    ConnectionRecoveryService =NewObject<UP2CConnectionRecoveryService>(this);

    if (!IsValid(ConnectionRecoveryService))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Could not create "
                "ConnectionRecoveryService."));

        return false;
    }

    ConnectionRecoveryService->OnRecoveryRequested
        .AddUObject(
            this,
            &UMultiplayerSessionSubsystem::
                HandleConnectionRecoveryRequested);

    ConnectionRecoveryService->Initialize(GetGameInstance());
    return true;
}

void UMultiplayerSessionSubsystem::Deinitialize()
{
    DeinitializeConnectionRecoveryService();
    DeinitializeOnlineSessionService();

    Super::Deinitialize();
}

void UMultiplayerSessionSubsystem::DeinitializeConnectionRecoveryService()
{
    if (!IsValid(ConnectionRecoveryService))
    {
        return;
    }

    ConnectionRecoveryService->OnRecoveryRequested.RemoveAll(this);
    ConnectionRecoveryService->Deinitialize();
    ConnectionRecoveryService = nullptr;
}

void UMultiplayerSessionSubsystem::DeinitializeOnlineSessionService()
{
    if (!IsValid(OnlineSessionService))
    {
        return;
    }

    OnlineSessionService->OnCreateSessionCompleted.RemoveAll(this);
    OnlineSessionService->OnFindSessionsCompleted.RemoveAll(this);
    OnlineSessionService->OnJoinSessionCompleted.RemoveAll(this);
    OnlineSessionService->OnDestroySessionCompleted.RemoveAll(this);
    OnlineSessionService->Deinitialize();
    OnlineSessionService = nullptr;
}

void UMultiplayerSessionSubsystem::CreateSession(const int32 NumPublicConnections)
{
    if (IsValid(ConnectionRecoveryService))
    {
        ConnectionRecoveryService->ResetRecoveryState();
    }

    if (!IsValid(OnlineSessionService))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Cannot create session: "
                "OnlineSessionService is invalid."));

        OnCreateSessionCompleted.Broadcast(false);
        return;
    }

    OnlineSessionService->CreateSession(
        NumPublicConnections);
}

void UMultiplayerSessionSubsystem::FindSessions(const int32 MaxSearchResults)
{
    if (IsValid(ConnectionRecoveryService))
    {
        ConnectionRecoveryService->ResetRecoveryState();
    }

    if (!IsValid(OnlineSessionService))
    {
        OnFindSessionsCompleted.Broadcast(false, 0);
        return;
    }

    OnlineSessionService->FindSessions(
        MaxSearchResults);
}

void UMultiplayerSessionSubsystem::JoinSession(const int32 CachedResultIndex)
{
    if (IsValid(ConnectionRecoveryService))
    {
        ConnectionRecoveryService->ResetRecoveryState();
    }

    if (!IsValid(OnlineSessionService))
    {
        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    OnlineSessionService->JoinSession(
        CachedResultIndex);
}

void UMultiplayerSessionSubsystem::LeaveSession()
{
    if (!IsValid(OnlineSessionService))
    {
        OnLeaveSessionCompleted.Broadcast(false);

        if (IsRecoveringFromFailure())
        {
            ReturnToMainMenu();
        }

        return;
    }

    OnlineSessionService->DestroySession();
}

void UMultiplayerSessionSubsystem::HandleOnlineCreateSessionCompleted(const bool bWasSuccessful) const
{
    if (!bWasSuccessful)
    {
        return;
    }

    UP2CTravelSubsystem* TravelSubsystem =GetTravelSubsystem();

    if (!IsValid(TravelSubsystem))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Session was created, but "
                "TravelSubsystem is invalid."));

        return;
    }

    if (!TravelSubsystem->ServerTravelToMap(EP2CMapType::Lobby, true))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Session was created, but "
                "ServerTravel failed."));
        
        return;
    }
    
    OnCreateSessionCompleted.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionSubsystem::HandleOnlineFindSessionsCompleted(
    const bool bWasSuccessful,
    const int32 ResultCount) const
{
    OnFindSessionsCompleted.Broadcast(
        bWasSuccessful,
        ResultCount);
}

void UMultiplayerSessionSubsystem::HandleOnlineJoinSessionCompleted(
        const bool bWasSuccessful,
        const FString& ConnectString) const
{
    if (!bWasSuccessful)
    {
        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    UP2CTravelSubsystem* TravelSubsystem =GetTravelSubsystem();

    const bool bTravelStarted = IsValid(TravelSubsystem)
            && TravelSubsystem->ClientTravelToAddress(ConnectString);

    if (!bTravelStarted)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Session was joined, but "
                "ClientTravel could not be started."));
    }

    OnJoinSessionCompleted.Broadcast(bTravelStarted);
}

void UMultiplayerSessionSubsystem::HandleOnlineDestroySessionCompleted(const bool bWasSuccessful) const
{
    OnLeaveSessionCompleted.Broadcast(bWasSuccessful);

    if (bWasSuccessful || IsRecoveringFromFailure())
    {
        ReturnToMainMenu();
    }
}

void UMultiplayerSessionSubsystem::HandleConnectionRecoveryRequested() const
{
    if (!IsValid(OnlineSessionService))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Cannot clean up session during recovery: "
                "OnlineSessionService is invalid."));

        ReturnToMainMenu();
        return;
    }

    if (OnlineSessionService->HasActiveSession())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Log,
            TEXT(
                "Cleaning up session after "
                "connection failure."));

        OnlineSessionService->DestroySession();
        return;
    }

    OnlineSessionService->ClearCachedData();

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT(
            "No active session found during recovery. "
            "Returning to MainMenu."));

    ReturnToMainMenu();
}

TArray<FP2CSessionSearchResult>UMultiplayerSessionSubsystem::GetCachedSessionResults() const
{
    if (!IsValid(OnlineSessionService))
    {
        return {};
    }

    return OnlineSessionService->GetCachedSessionResults();
}

void UMultiplayerSessionSubsystem::ReturnToMainMenu() const
{
    UP2CTravelSubsystem* TravelSubsystem = GetTravelSubsystem();

    if (!IsValid(TravelSubsystem))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Cannot return to MainMenu: "
                "TravelSubsystem is invalid."));

        return;
    }

    const bool bTravelStarted =
        TravelSubsystem->OpenMap(
            EP2CMapType::MainMenu);

    if (!bTravelStarted)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Could not start travel "
                "to MainMenu."));
    }
}

bool UMultiplayerSessionSubsystem::IsRecoveringFromFailure() const
{
    return IsValid(ConnectionRecoveryService) && ConnectionRecoveryService->IsRecovering();
}

UP2CTravelSubsystem*UMultiplayerSessionSubsystem::GetTravelSubsystem() const
{
    const UGameInstance* GameInstance = GetGameInstance();

    return IsValid(GameInstance)
        ? GameInstance->GetSubsystem<UP2CTravelSubsystem>()
        : nullptr;
}