#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "UObject/Object.h"

#include "Multiplayer/Data/P2CSessionSearchResult.h"

#include "P2COnlineSessionService.generated.h"

class UGameInstance;
class UWorld;

DECLARE_MULTICAST_DELEGATE_OneParam(FP2COnCreateSessionCompleted, bool);

DECLARE_MULTICAST_DELEGATE_TwoParams(FP2COnFindSessionsCompleted, bool, int32);

DECLARE_MULTICAST_DELEGATE_TwoParams(FP2COnJoinSessionCompleted, bool, const FString&);

DECLARE_MULTICAST_DELEGATE_OneParam(FP2COnDestroySessionCompleted, bool);

UCLASS()
class P2C_API UP2COnlineSessionService final : public UObject
{
    GENERATED_BODY()

public:
    UP2COnlineSessionService();
    void Initialize(UGameInstance* InGameInstance);
    void Deinitialize();

    void CreateSession(int32 NumPublicConnections);
    void FindSessions(int32 MaxSearchResults);
    void JoinSession(int32 CachedResultIndex);
    void DestroySession();
    bool HasActiveSession() const;
    const TArray<FP2CSessionSearchResult>&GetCachedSessionResults() const;
    void ClearCachedData();

    FP2COnCreateSessionCompleted OnCreateSessionCompleted;
    FP2COnFindSessionsCompleted OnFindSessionsCompleted;
    FP2COnJoinSessionCompleted OnJoinSessionCompleted;
    FP2COnDestroySessionCompleted OnDestroySessionCompleted;

private:
    IOnlineSessionPtr GetSessionInterface() const;
    UWorld* GetOwningWorld() const;
    void HandleCreateSessionComplete(FName SessionName,bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName,EOnJoinSessionCompleteResult::Type Result);
    void HandleDestroySessionComplete(FName SessionName,bool bWasSuccessful);

    TWeakObjectPtr<UGameInstance> OwningGameInstance;

    TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
    TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

    TArray<FP2CSessionSearchResult> CachedSessionResults;

    FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;

    FDelegateHandle CreateSessionCompleteDelegateHandle;
    FDelegateHandle FindSessionsCompleteDelegateHandle;
    FDelegateHandle JoinSessionCompleteDelegateHandle;
    FDelegateHandle DestroySessionCompleteDelegateHandle;
};