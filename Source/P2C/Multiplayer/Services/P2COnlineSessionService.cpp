#include "P2COnlineSessionService.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogP2COnlineSessions, Log, All);

namespace
{
	const FName MatchTypeKey(TEXT("MatchType"));
	const FString MatchTypeValue(TEXT("Arena"));
}

UP2COnlineSessionService::UP2COnlineSessionService()
: CreateSessionCompleteDelegate(
	FOnCreateSessionCompleteDelegate::CreateUObject(
		this,
		&UP2COnlineSessionService::HandleCreateSessionComplete)),
FindSessionsCompleteDelegate(
	FOnFindSessionsCompleteDelegate::CreateUObject(
		this,
		&UP2COnlineSessionService::HandleFindSessionsComplete)),
JoinSessionCompleteDelegate(
	  FOnJoinSessionCompleteDelegate::CreateUObject(
		  this,
		  &UP2COnlineSessionService::HandleJoinSessionComplete)),
DestroySessionCompleteDelegate(
	   FOnDestroySessionCompleteDelegate::CreateUObject(
		   this,
		   &UP2COnlineSessionService::HandleDestroySessionComplete))
{
}

void UP2COnlineSessionService::Initialize(UGameInstance* InGameInstance)
{
    if (!IsValid(InGameInstance))
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT(
                "Cannot initialize online session service: "
                "GameInstance is invalid."));

        return;
    }

    OwningGameInstance = InGameInstance;

    UE_LOG(
        LogP2COnlineSessions,
        Log,
        TEXT("Online session service initialized."));
}

void UP2COnlineSessionService::Deinitialize()
{
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

    OnCreateSessionCompleted.Clear();
    OnFindSessionsCompleted.Clear();
    OnJoinSessionCompleted.Clear();
    OnDestroySessionCompleted.Clear();

    ClearCachedData();
    OwningGameInstance.Reset();

    UE_LOG(
        LogP2COnlineSessions,
        Log,
        TEXT("Online session service deinitialized."));
}

UWorld* UP2COnlineSessionService::GetOwningWorld() const
{
    return OwningGameInstance.IsValid()
        ? OwningGameInstance->GetWorld()
        : nullptr;
}

IOnlineSessionPtr UP2COnlineSessionService::GetSessionInterface() const
{
    UWorld* World = GetOwningWorld();

    return IsValid(World)
        ? Online::GetSessionInterface(World)
        : nullptr;
}

bool UP2COnlineSessionService::HasActiveSession() const
{
    const IOnlineSessionPtr SessionInterface =
        GetSessionInterface();

    return SessionInterface.IsValid() &&
        SessionInterface
            ->GetNamedSession(NAME_GameSession) != nullptr;
}

const TArray<FP2CSessionSearchResult>&UP2COnlineSessionService::GetCachedSessionResults() const
{
    return CachedSessionResults;
}

void UP2COnlineSessionService::ClearCachedData()
{
    LastSessionSettings.Reset();
    LastSessionSearch.Reset();
    CachedSessionResults.Reset();
}

void UP2COnlineSessionService::CreateSession(const int32 NumPublicConnections)
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot create session: session interface is invalid."));

        OnCreateSessionCompleted.Broadcast(false);
        return;
    }

    if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
    {
        UE_LOG(
            LogP2COnlineSessions,
            Warning,
            TEXT("Cannot create session: GameSession already exists."));

        OnCreateSessionCompleted.Broadcast(false);
        return;
    }

    const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetOwningWorld());

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
        LogP2COnlineSessions,
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
            LogP2COnlineSessions,
            Error,
            TEXT("CreateSession request could not be started."));

        OnCreateSessionCompleted.Broadcast(false);
    }
}

void UP2COnlineSessionService::FindSessions(const int32 MaxSearchResults)
{
    CachedSessionResults.Reset();

    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot find sessions: session interface is invalid."));

        OnFindSessionsCompleted.Broadcast(false, 0);
        return;
    }

    if (LastSessionSearch.IsValid() 
        && LastSessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
    {
        UE_LOG(
            LogP2COnlineSessions,
            Warning,
            TEXT("Cannot find sessions: search is already in progress."));

        OnFindSessionsCompleted.Broadcast(false, 0);
        return;
    }

    const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetOwningWorld());

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
        LogP2COnlineSessions,
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
            LogP2COnlineSessions,
            Error,
            TEXT("FindSessions request could not be started."));

        OnFindSessionsCompleted.Broadcast(false, 0);
    }
}

void UP2COnlineSessionService::HandleCreateSessionComplete(const FName SessionName,const bool bWasSuccessful)
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
        LogP2COnlineSessions,
        Log,
        TEXT("CreateSession completed. Session: %s, Success: %s"),
        *SessionName.ToString(),
        bWasSuccessful ? TEXT("true") : TEXT("false"));

    OnCreateSessionCompleted.Broadcast(bWasSuccessful);
}

void UP2COnlineSessionService::HandleFindSessionsComplete(const bool bWasSuccessful)
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
            LogP2COnlineSessions,
            Error,
            TEXT("FindSessions failed."));

        OnFindSessionsCompleted.Broadcast(false, 0);
        return;
    }

    const int32 RawResultCount =
        LastSessionSearch->SearchResults.Num();

    UE_LOG(
        LogP2COnlineSessions,
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
                LogP2COnlineSessions,
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
            LogP2COnlineSessions,
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
        LogP2COnlineSessions,
        Log,
        TEXT("Matching P2C sessions: %d"),
        CachedSessionResults.Num());

    OnFindSessionsCompleted.Broadcast(
        true,
        CachedSessionResults.Num());
}

void UP2COnlineSessionService::JoinSession(const int32 CachedResultIndex)
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot join session: session interface is invalid."));

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }

    if (!LastSessionSearch.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot join session: no session search is available."));

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }

    if (!CachedSessionResults.IsValidIndex(CachedResultIndex))
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot join session: cached result index %d is invalid."),
            CachedResultIndex);

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }

    const int32 RawResultIndex =
        CachedSessionResults[CachedResultIndex].ResultIndex;

    if (!LastSessionSearch->SearchResults.IsValidIndex(RawResultIndex))
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot join session: raw result index %d is invalid."),
            RawResultIndex);

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }

    const FOnlineSessionSearchResult& SearchResult =
        LastSessionSearch->SearchResults[RawResultIndex];

    if (!SearchResult.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot join session: selected search result is invalid."));

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }

    if (JoinSessionCompleteDelegateHandle.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Warning,
            TEXT("Cannot join session: join operation is already active."));

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }
    
    JoinSessionCompleteDelegateHandle =
        SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
            JoinSessionCompleteDelegate);

    UE_LOG(
        LogP2COnlineSessions,
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
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        JoinSessionCompleteDelegateHandle.Reset();

        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("JoinSession request could not be started."));

        OnJoinSessionCompleted.Broadcast(false, FString());
    }
}

void UP2COnlineSessionService::HandleJoinSessionComplete(const FName SessionName,const EOnJoinSessionCompleteResult::Type Result)
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
        LogP2COnlineSessions,
        Log,
        TEXT("JoinSession completed. Session: %s, Result: %d"),
        *SessionName.ToString(),
        static_cast<int32>(Result));

    if (!bJoinSucceeded)
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("JoinSession failed with result: %d"),
            static_cast<int32>(Result));

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT(
                "Session joined, but session interface "
                "is no longer valid."));

        OnJoinSessionCompleted.Broadcast(false, FString());
        return;
    }

    FString ConnectString;

    if (!SessionInterface->GetResolvedConnectString(
            SessionName,
            ConnectString))
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT(
                "Could not resolve the session "
                "connect string."));

        OnJoinSessionCompleted.Broadcast(
            false,
            FString());

        return;
    }

    UE_LOG(
        LogP2COnlineSessions,
        Log,
        TEXT("Resolved connect string: %s"),
        *ConnectString);

    OnJoinSessionCompleted.Broadcast(
        true,
        ConnectString);
}

void UP2COnlineSessionService::DestroySession()
{
    const IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Error,
            TEXT("Cannot leave session: session interface is invalid."));

        OnDestroySessionCompleted.Broadcast(false);
        return;
    }

    if (DestroySessionCompleteDelegateHandle.IsValid())
    {
        UE_LOG(
            LogP2COnlineSessions,
            Warning,
            TEXT("Cannot leave session: destroy operation is already active."));

        OnDestroySessionCompleted.Broadcast(false);
        return;
    }

    const FNamedOnlineSession* ExistingSession = 
        SessionInterface->GetNamedSession(NAME_GameSession);

    if (ExistingSession == nullptr)
    {
        UE_LOG(
            LogP2COnlineSessions,
            Log,
            TEXT("No active GameSession exists."));

        ClearCachedData();
        OnDestroySessionCompleted.Broadcast(true);
        return;
    }

    DestroySessionCompleteDelegateHandle =
        SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
            DestroySessionCompleteDelegate);

    UE_LOG(
        LogP2COnlineSessions,
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
            LogP2COnlineSessions,
            Error,
            TEXT("DestroySession request could not be started."));

        OnDestroySessionCompleted.Broadcast(false);
    }
}

void UP2COnlineSessionService::HandleDestroySessionComplete(
    const FName SessionName,const 
    bool bWasSuccessful)
{
    const IOnlineSessionPtr SessionInterface =
        GetSessionInterface();

    if (SessionInterface.IsValid() &&
        DestroySessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface
            ->ClearOnDestroySessionCompleteDelegate_Handle(
                DestroySessionCompleteDelegateHandle);
    }

    DestroySessionCompleteDelegateHandle.Reset();

    UE_LOG(
        LogP2COnlineSessions,
        Log,
        TEXT(
            "DestroySession completed. "
            "Session: %s, Success: %s"),
        *SessionName.ToString(),
        bWasSuccessful ? TEXT("true") : TEXT("false"));

    if (bWasSuccessful)
    {
        ClearCachedData();
    }

    OnDestroySessionCompleted.Broadcast(
        bWasSuccessful);
}
