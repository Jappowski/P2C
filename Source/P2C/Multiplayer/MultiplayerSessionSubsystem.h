#pragma once
#include "Map/P2CTravelSubsystem.h"
class UNetDriver;

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "MultiplayerSessionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FP2CSessionSearchResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer|Sessions")
    int32 ResultIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer|Sessions")
    FString HostName;

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer|Sessions")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer|Sessions")
    int32 MaxPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Multiplayer|Sessions")
    int32 PingInMs = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnCreateSessionCompleted,
    bool,
    bWasSuccessful
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnFindSessionsCompleted,
    bool,
    bWasSuccessful,
    int32,
    ResultCount
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnJoinSessionCompleted,
    bool,
    bWasSuccessful
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnLeaveSessionCompleted,
    bool,
    bWasSuccessful
);

UCLASS()
class P2C_API UMultiplayerSessionSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UMultiplayerSessionSubsystem();
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void CreateSession(int32 NumPublicConnections);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void FindSessions(int32 MaxSearchResults = 100);

    UFUNCTION(BlueprintPure,Category = "Multiplayer|Sessions")
    TArray<FP2CSessionSearchResult> GetCachedSessionResults() const;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnCreateSessionCompleted OnCreateSessionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnFindSessionsCompleted OnFindSessionsCompleted;
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void JoinSession(int32 CachedResultIndex);

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnJoinSessionCompleted OnJoinSessionCompleted;
    
    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void LeaveSession();

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnLeaveSessionCompleted OnLeaveSessionCompleted;

private:
    IOnlineSessionPtr GetSessionInterface() const;

    void HandleCreateSessionComplete(
        FName SessionName,
        bool bWasSuccessful);

    void HandleFindSessionsComplete(bool bWasSuccessful);
    
    void HandleJoinSessionComplete(
        FName SessionName,
        EOnJoinSessionCompleteResult::Type Result
    );
    
    void HandleDestroySessionComplete(
    FName SessionName,
    bool bWasSuccessful);

    void HandleNetworkFailure(
        UWorld* World,
        UNetDriver*,
        ENetworkFailure::Type FailureType,
        const FString& ErrorString
    );
    
    void HandleTravelFailure(
        UWorld* World,
        ETravelFailure::Type FailureType,
        const FString& ErrorString
    );

    bool IsFailureForThisGameInstance(const UWorld* World) const;
    void RecoverFromConnectionFailure();
    UP2CTravelSubsystem* GetTravelSubsystem() const;


    bool bIsRecoveringFromFailure = false;
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

    FDelegateHandle NetworkFailureDelegateHandle;
    FDelegateHandle TravelFailureDelegateHandle;
};