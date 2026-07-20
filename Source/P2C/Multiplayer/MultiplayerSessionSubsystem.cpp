#include "MultiplayerSessionSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Map/P2CTravelSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerSessions, Log, All);

namespace
{
    const FName MatchTypeKey(TEXT("MatchType"));
    const FString MatchTypeValue(TEXT("Arena"));
}

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem()
: CreateSessionCompleteDelegate(
    FOnCreateSessionCompleteDelegate::CreateUObject(
        this,
        &UMultiplayerSessionSubsystem::HandleCreateSessionComplete)),
FindSessionsCompleteDelegate(
    FOnFindSessionsCompleteDelegate::CreateUObject(
        this,
        &UMultiplayerSessionSubsystem::HandleFindSessionsComplete)),
JoinSessionCompleteDelegate(
      FOnJoinSessionCompleteDelegate::CreateUObject(
          this,
          &UMultiplayerSessionSubsystem::HandleJoinSessionComplete)),
DestroySessionCompleteDelegate(
       FOnDestroySessionCompleteDelegate::CreateUObject(
           this,
           &UMultiplayerSessionSubsystem::HandleDestroySessionComplete))
{
}

void UMultiplayerSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!GEngine)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot register failure handlers: GEngine is invalid."));

        return;
    }

    NetworkFailureDelegateHandle =
        GEngine->OnNetworkFailure().AddUObject(
            this,
            &UMultiplayerSessionSubsystem::HandleNetworkFailure);

    TravelFailureDelegateHandle =
        GEngine->OnTravelFailure().AddUObject(
            this,
            &UMultiplayerSessionSubsystem::HandleTravelFailure);

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("Network and travel failure handlers registered."));
}

void UMultiplayerSessionSubsystem::Deinitialize()
{
    if (GEngine)
    {
        if (NetworkFailureDelegateHandle.IsValid())
        {
            GEngine->OnNetworkFailure().Remove(
                NetworkFailureDelegateHandle);
        }

        if (TravelFailureDelegateHandle.IsValid())
        {
            GEngine->OnTravelFailure().Remove(
                TravelFailureDelegateHandle);
        }
    }

    NetworkFailureDelegateHandle.Reset();
    TravelFailureDelegateHandle.Reset();
    bIsRecoveringFromFailure = false;
    
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();
    if (SessionInterface.IsValid())
    {
        if (CreateSessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface
                ->ClearOnCreateSessionCompleteDelegate_Handle(
                    CreateSessionCompleteDelegateHandle);
        }

        if (FindSessionsCompleteDelegateHandle.IsValid())
        {
            SessionInterface
                ->ClearOnFindSessionsCompleteDelegate_Handle(
                    FindSessionsCompleteDelegateHandle);
        }

        if (JoinSessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface
                ->ClearOnJoinSessionCompleteDelegate_Handle(
                    JoinSessionCompleteDelegateHandle);
        }

        if (DestroySessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface
                ->ClearOnDestroySessionCompleteDelegate_Handle(
                    DestroySessionCompleteDelegateHandle);
        }
    }

    CreateSessionCompleteDelegateHandle.Reset();
    FindSessionsCompleteDelegateHandle.Reset();
    JoinSessionCompleteDelegateHandle.Reset();
    DestroySessionCompleteDelegateHandle.Reset();

    LastSessionSettings.Reset();
    LastSessionSearch.Reset();
    CachedSessionResults.Reset();

    Super::Deinitialize();
}

void UMultiplayerSessionSubsystem::CreateSession(const int32 NumPublicConnections)
{
    bIsRecoveringFromFailure = false;
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot create session: session interface is invalid."));

        OnCreateSessionCompleted.Broadcast(false);
        return;
    }

    if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Warning,
            TEXT("Cannot create session: GameSession already exists."));

        OnCreateSessionCompleted.Broadcast(false);
        return;
    }

    const IOnlineSubsystem* OnlineSubsystem =
        Online::GetSubsystem(GetWorld());

    const FName SubsystemName = OnlineSubsystem
        ? OnlineSubsystem->GetSubsystemName()
        : NAME_None;

    const bool bIsLanMatch =
        SubsystemName == FName(TEXT("NULL"));

    LastSessionSettings = MakeShared<FOnlineSessionSettings>();

    LastSessionSettings->bIsLANMatch = bIsLanMatch;
    LastSessionSettings->bShouldAdvertise = true;
    LastSessionSettings->bAllowJoinInProgress = true;
    LastSessionSettings->bIsDedicated = false;
    LastSessionSettings->bUsesPresence = false;
    LastSessionSettings->bAllowJoinViaPresence = false;

    LastSessionSettings->NumPrivateConnections = 0;
    LastSessionSettings->NumPublicConnections =
        FMath::Max(1, NumPublicConnections);

    LastSessionSettings->Set(
        MatchTypeKey,
        MatchTypeValue,
        EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    CreateSessionCompleteDelegateHandle =
        SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
            CreateSessionCompleteDelegate);

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("Creating session. Subsystem: %s, LAN: %s, Slots: %d"),
        *SubsystemName.ToString(),
        bIsLanMatch ? TEXT("true") : TEXT("false"),
        LastSessionSettings->NumPublicConnections);

    const bool bRequestStarted = SessionInterface->CreateSession(
        0,
        NAME_GameSession,
        *LastSessionSettings);

    if (!bRequestStarted)
    {
        SessionInterface
            ->ClearOnCreateSessionCompleteDelegate_Handle(
                CreateSessionCompleteDelegateHandle);

        CreateSessionCompleteDelegateHandle.Reset();

        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("CreateSession request could not be started."));

        OnCreateSessionCompleted.Broadcast(false);
    }
}

void UMultiplayerSessionSubsystem::FindSessions(const int32 MaxSearchResults)
{
    bIsRecoveringFromFailure = false;
    CachedSessionResults.Reset();

    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot find sessions: session interface is invalid."));

        OnFindSessionsCompleted.Broadcast(false, 0);
        return;
    }

    if (LastSessionSearch.IsValid() &&
        LastSessionSearch->SearchState ==
            EOnlineAsyncTaskState::InProgress)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Warning,
            TEXT("Cannot find sessions: search is already in progress."));

        OnFindSessionsCompleted.Broadcast(false, 0);
        return;
    }

    const IOnlineSubsystem* OnlineSubsystem =
        Online::GetSubsystem(GetWorld());

    const FName SubsystemName = OnlineSubsystem
        ? OnlineSubsystem->GetSubsystemName()
        : NAME_None;

    const bool bIsLanQuery =
        SubsystemName == FName(TEXT("NULL"));

    LastSessionSearch = MakeShared<FOnlineSessionSearch>();

    LastSessionSearch->MaxSearchResults =
        FMath::Max(1, MaxSearchResults);

    LastSessionSearch->bIsLanQuery = bIsLanQuery;
    LastSessionSearch->PingBucketSize = 50;

    FindSessionsCompleteDelegateHandle =
        SessionInterface
            ->AddOnFindSessionsCompleteDelegate_Handle(
                FindSessionsCompleteDelegate);

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT(
            "Finding sessions. Subsystem: %s, LAN: %s, Max results: %d"),
        *SubsystemName.ToString(),
        bIsLanQuery ? TEXT("true") : TEXT("false"),
        LastSessionSearch->MaxSearchResults);

    const bool bRequestStarted = SessionInterface->FindSessions(
        0,
        LastSessionSearch.ToSharedRef());

    if (!bRequestStarted)
    {
        SessionInterface
            ->ClearOnFindSessionsCompleteDelegate_Handle(
                FindSessionsCompleteDelegateHandle);

        FindSessionsCompleteDelegateHandle.Reset();

        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("FindSessions request could not be started."));

        OnFindSessionsCompleted.Broadcast(false, 0);
    }
}

IOnlineSessionPtr UMultiplayerSessionSubsystem::GetSessionInterface() const
{
    return Online::GetSessionInterface(GetWorld());
}

void UMultiplayerSessionSubsystem::HandleCreateSessionComplete(const FName SessionName,const bool bWasSuccessful)
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (SessionInterface.IsValid() &&
        CreateSessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface
            ->ClearOnCreateSessionCompleteDelegate_Handle(
                CreateSessionCompleteDelegateHandle);
    }

    CreateSessionCompleteDelegateHandle.Reset();

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("CreateSession completed. Session: %s, Success: %s"),
        *SessionName.ToString(),
        bWasSuccessful ? TEXT("true") : TEXT("false"));

    OnCreateSessionCompleted.Broadcast(bWasSuccessful);

    if (!bWasSuccessful)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Session was created, but World is invalid."));

        return;
    }

    UP2CTravelSubsystem* TravelSubsystem = GetGameInstance()->GetSubsystem<UP2CTravelSubsystem>();

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

    const bool bTravelStarted =
        TravelSubsystem->ServerTravelToMap(
            EP2CMapType::Arena,
            true);

    if (!bTravelStarted)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Session was created, but ServerTravel failed."));
    }
}

void UMultiplayerSessionSubsystem::HandleFindSessionsComplete(const bool bWasSuccessful)
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (SessionInterface.IsValid() &&
        FindSessionsCompleteDelegateHandle.IsValid())
    {
        SessionInterface
            ->ClearOnFindSessionsCompleteDelegate_Handle(
                FindSessionsCompleteDelegateHandle);
    }

    FindSessionsCompleteDelegateHandle.Reset();
    CachedSessionResults.Reset();

    if (!bWasSuccessful || !LastSessionSearch.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("FindSessions failed."));

        OnFindSessionsCompleted.Broadcast(false, 0);
        return;
    }

    const int32 RawResultCount =
        LastSessionSearch->SearchResults.Num();

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("FindSessions completed. Raw results: %d"),
        RawResultCount);

    for (int32 Index = 0; Index < RawResultCount; ++Index)
    {
        const FOnlineSessionSearchResult& SearchResult =
            LastSessionSearch->SearchResults[Index];

        if (!SearchResult.IsValid())
        {
            continue;
        }

        FString FoundMatchType;

        const bool bHasMatchType =
            SearchResult.Session.SessionSettings.Get(
                MatchTypeKey,
                FoundMatchType);

        if (!bHasMatchType ||
            FoundMatchType != MatchTypeValue)
        {
            UE_LOG(
                LogMultiplayerSessions,
                Verbose,
                TEXT("Ignoring unrelated session at index %d."),
                Index);

            continue;
        }

        FP2CSessionSearchResult ResultInfo;

        ResultInfo.ResultIndex = Index;
        ResultInfo.HostName =
            SearchResult.Session.OwningUserName;

        if (ResultInfo.HostName.IsEmpty())
        {
            ResultInfo.HostName = TEXT("LAN Host");
        }

        ResultInfo.MaxPlayers =
            SearchResult
                .Session
                .SessionSettings
                .NumPublicConnections;

        ResultInfo.CurrentPlayers = FMath::Max(
            0,
            ResultInfo.MaxPlayers -
                SearchResult.Session.NumOpenPublicConnections);

        ResultInfo.PingInMs = SearchResult.PingInMs;

        CachedSessionResults.Add(ResultInfo);

        UE_LOG(
            LogMultiplayerSessions,
            Log,
            TEXT(
                "Session found. Index: %d, Host: %s, "
                "Players: %d/%d, Ping: %d ms"),
            ResultInfo.ResultIndex,
            *ResultInfo.HostName,
            ResultInfo.CurrentPlayers,
            ResultInfo.MaxPlayers,
            ResultInfo.PingInMs);
    }

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("Matching P2C sessions: %d"),
        CachedSessionResults.Num());

    OnFindSessionsCompleted.Broadcast(
        true,
        CachedSessionResults.Num());
}

void UMultiplayerSessionSubsystem::JoinSession(const int32 CachedResultIndex)
{
    bIsRecoveringFromFailure = false;
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot join session: session interface is invalid."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    if (!LastSessionSearch.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot join session: no session search is available."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    if (!CachedSessionResults.IsValidIndex(CachedResultIndex))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot join session: cached result index %d is invalid."),
            CachedResultIndex);

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    const int32 RawResultIndex =
        CachedSessionResults[CachedResultIndex].ResultIndex;

    if (!LastSessionSearch->SearchResults.IsValidIndex(RawResultIndex))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot join session: raw result index %d is invalid."),
            RawResultIndex);

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    const FOnlineSessionSearchResult& SearchResult =
        LastSessionSearch->SearchResults[RawResultIndex];

    if (!SearchResult.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot join session: selected search result is invalid."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    if (JoinSessionCompleteDelegateHandle.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Warning,
            TEXT("Cannot join session: join operation is already active."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    JoinSessionCompleteDelegateHandle =
        SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
            JoinSessionCompleteDelegate);

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("Joining cached session result %d, raw result %d."),
        CachedResultIndex,
        RawResultIndex);

    const bool bRequestStarted = SessionInterface->JoinSession(
        0,
        NAME_GameSession,
        SearchResult);

    if (!bRequestStarted)
    {
        SessionInterface
            ->ClearOnJoinSessionCompleteDelegate_Handle(
                JoinSessionCompleteDelegateHandle);

        JoinSessionCompleteDelegateHandle.Reset();

        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("JoinSession request could not be started."));

        OnJoinSessionCompleted.Broadcast(false);
    }
}

void UMultiplayerSessionSubsystem::HandleJoinSessionComplete(const FName SessionName,const EOnJoinSessionCompleteResult::Type Result)
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (SessionInterface.IsValid() &&
        JoinSessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface
            ->ClearOnJoinSessionCompleteDelegate_Handle(
                JoinSessionCompleteDelegateHandle);
    }

    JoinSessionCompleteDelegateHandle.Reset();

    const bool bJoinSucceeded =
        Result == EOnJoinSessionCompleteResult::Success;

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("JoinSession completed. Session: %s, Result: %d"),
        *SessionName.ToString(),
        static_cast<int32>(Result));

    if (!bJoinSucceeded)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("JoinSession failed with result: %d"),
            static_cast<int32>(Result));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT(
                "Session joined, but session interface "
                "is no longer valid."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    FString ConnectString;

    if (!SessionInterface->GetResolvedConnectString(
            SessionName,
            ConnectString))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Could not resolve the session connect string."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Session joined, but World is invalid."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    APlayerController* PlayerController =
        World->GetFirstPlayerController();

    if (!IsValid(PlayerController))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Session joined, but PlayerController is invalid."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    UP2CTravelSubsystem* TravelSubsystem =GetGameInstance()->GetSubsystem<UP2CTravelSubsystem>();

    if (!IsValid(TravelSubsystem) ||!TravelSubsystem->ClientTravelToAddress(ConnectString))
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Client travel could not be started."));

        OnJoinSessionCompleted.Broadcast(false);
        return;
    }

    OnJoinSessionCompleted.Broadcast(true);
}

void UMultiplayerSessionSubsystem::LeaveSession()
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("Cannot leave session: session interface is invalid."));

        OnLeaveSessionCompleted.Broadcast(false);

        if (bIsRecoveringFromFailure)
        {
            GetTravelSubsystem()->OpenMap(EP2CMapType::MainMenu);
        }

        return;
    }

    if (DestroySessionCompleteDelegateHandle.IsValid())
    {
        UE_LOG(
            LogMultiplayerSessions,
            Warning,
            TEXT("Cannot leave session: destroy operation is already active."));

        OnLeaveSessionCompleted.Broadcast(false);
        return;
    }

    const FNamedOnlineSession* ExistingSession =
        SessionInterface->GetNamedSession(NAME_GameSession);

    if (ExistingSession == nullptr)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Log,
            TEXT("No active GameSession exists. Returning to MainMenu."));

        LastSessionSettings.Reset();
        LastSessionSearch.Reset();
        CachedSessionResults.Reset();

        OnLeaveSessionCompleted.Broadcast(true);
        GetTravelSubsystem()->OpenMap(EP2CMapType::MainMenu);
        return;
    }

    DestroySessionCompleteDelegateHandle =
        SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
            DestroySessionCompleteDelegate);

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("Leaving GameSession."));

    const bool bRequestStarted =
        SessionInterface->DestroySession(NAME_GameSession);

    if (!bRequestStarted)
    {
        SessionInterface
            ->ClearOnDestroySessionCompleteDelegate_Handle(
                DestroySessionCompleteDelegateHandle);

        DestroySessionCompleteDelegateHandle.Reset();

        UE_LOG(
            LogMultiplayerSessions,
            Error,
            TEXT("DestroySession request could not be started."));

        OnLeaveSessionCompleted.Broadcast(false);

        if (bIsRecoveringFromFailure)
        {
            UE_LOG(
                LogMultiplayerSessions,
                Warning,
                TEXT(
                    "DestroySession could not start during recovery. "
                    "Returning to MainMenu anyway."));

            GetTravelSubsystem()->OpenMap(EP2CMapType::MainMenu);
        }
    }
}

void UMultiplayerSessionSubsystem::HandleDestroySessionComplete(
    const FName SessionName,
    const bool bWasSuccessful)
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (SessionInterface.IsValid() &&
        DestroySessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface
            ->ClearOnDestroySessionCompleteDelegate_Handle(
                DestroySessionCompleteDelegateHandle);
    }

    DestroySessionCompleteDelegateHandle.Reset();

    const bool bWasRecoveringFromFailure =
        bIsRecoveringFromFailure;

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT("DestroySession completed. Session: %s, Success: %s"),
        *SessionName.ToString(),
        bWasSuccessful ? TEXT("true") : TEXT("false"));

    if (!bWasSuccessful)
    {
        OnLeaveSessionCompleted.Broadcast(false);

        if (bWasRecoveringFromFailure)
        {
            UE_LOG(
                LogMultiplayerSessions,
                Warning,
                TEXT(
                    "Session cleanup failed during recovery. "
                    "Returning to MainMenu anyway."));

            GetTravelSubsystem()->OpenMap(EP2CMapType::MainMenu);
        }

        return;
    }

    LastSessionSettings.Reset();
    LastSessionSearch.Reset();
    CachedSessionResults.Reset();

    OnLeaveSessionCompleted.Broadcast(true);
    
    GetTravelSubsystem()->OpenMap(EP2CMapType::MainMenu);
}

TArray<FP2CSessionSearchResult>UMultiplayerSessionSubsystem::GetCachedSessionResults() const
{
    return CachedSessionResults;
}

bool UMultiplayerSessionSubsystem::IsFailureForThisGameInstance(const UWorld* World) const
{
    return IsValid(World) &&
        World->GetGameInstance() == GetGameInstance();
}

void UMultiplayerSessionSubsystem::HandleNetworkFailure(
    UWorld* World,
    UNetDriver*,
    const ENetworkFailure::Type FailureType,
    const FString& ErrorString)
{
    if (!IsFailureForThisGameInstance(World))
    {
        return;
    }

    if (bIsRecoveringFromFailure)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Verbose,
            TEXT(
                "Ignoring additional network failure during recovery. "
                "Type: %d"),
            static_cast<int32>(FailureType));

        return;
    }

    UE_LOG(
        LogMultiplayerSessions,
        Error,
        TEXT("Network failure. Type: %d, Error: %s"),
        static_cast<int32>(FailureType),
        *ErrorString);

    RecoverFromConnectionFailure();
}

void UMultiplayerSessionSubsystem::HandleTravelFailure(
    UWorld* World,
    const ETravelFailure::Type FailureType,
    const FString& ErrorString)
{
    if (!IsFailureForThisGameInstance(World))
    {
        return;
    }
    
    if (bIsRecoveringFromFailure)
    {
        return;
    }

    UE_LOG(
        LogMultiplayerSessions,
        Error,
        TEXT("Travel failure. Type: %d, Error: %s"),
        static_cast<int32>(FailureType),
        *ErrorString);

    RecoverFromConnectionFailure();
}

void UMultiplayerSessionSubsystem::RecoverFromConnectionFailure()
{
    if (bIsRecoveringFromFailure)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Verbose,
            TEXT("Connection recovery is already in progress."));

        return;
    }

    bIsRecoveringFromFailure = true;

    const IOnlineSessionPtr SessionInterface =
        GetSessionInterface();

    if (SessionInterface.IsValid() &&
        SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
    {
        UE_LOG(
            LogMultiplayerSessions,
            Log,
            TEXT("Cleaning up session after connection failure."));

        LeaveSession();
        return;
    }

    LastSessionSettings.Reset();
    LastSessionSearch.Reset();
    CachedSessionResults.Reset();

    UE_LOG(
        LogMultiplayerSessions,
        Log,
        TEXT(
            "No active session found during recovery. "
            "Returning to MainMenu."));
    
    GetTravelSubsystem()->OpenMap(EP2CMapType::MainMenu);
}

UP2CTravelSubsystem* UMultiplayerSessionSubsystem::GetTravelSubsystem() const
{
    const UGameInstance* GameInstance = GetGameInstance();

    return IsValid(GameInstance)
        ? GameInstance->GetSubsystem<UP2CTravelSubsystem>()
        : nullptr;
}