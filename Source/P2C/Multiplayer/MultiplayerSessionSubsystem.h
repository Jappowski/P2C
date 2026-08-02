#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/P2CSessionSearchResult.h"
#include "MultiplayerSessionSubsystem.generated.h"

class UP2CConnectionRecoveryService;
class UP2COnlineSessionService;
class UP2CTravelSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnCreateSessionCompleted,
    bool,
    bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnFindSessionsCompleted,
    bool,
    bWasSuccessful,
    int32,
    ResultCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnJoinSessionCompleted,
    bool,
    bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnLeaveSessionCompleted,
    bool,
    bWasSuccessful);

UCLASS()
class P2C_API UMultiplayerSessionSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void CreateSession(int32 NumPublicConnections);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void FindSessions(int32 MaxSearchResults);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void JoinSession(int32 CachedResultIndex);

    UFUNCTION(BlueprintCallable, Category = "Multiplayer|Sessions")
    void LeaveSession();

    UFUNCTION(BlueprintPure, Category = "Multiplayer|Sessions")
    TArray<FP2CSessionSearchResult>GetCachedSessionResults() const;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnCreateSessionCompleted OnCreateSessionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnFindSessionsCompleted OnFindSessionsCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnJoinSessionCompleted OnJoinSessionCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Multiplayer|Sessions")
    FOnLeaveSessionCompleted OnLeaveSessionCompleted;

private:
    void HandleOnlineCreateSessionCompleted(bool bWasSuccessful) const;
    void HandleOnlineFindSessionsCompleted(bool bWasSuccessful, int32 ResultCount) const;
    void HandleOnlineJoinSessionCompleted(bool bWasSuccessful, const FString& ConnectString) const;
    void HandleOnlineDestroySessionCompleted(bool bWasSuccessful) const;
    void HandleConnectionRecoveryRequested() const;

    void ReturnToMainMenu() const;
    
    bool InitializeOnlineSessionService();
    bool InitializeConnectionRecoveryService();

    void DeinitializeOnlineSessionService();
    void DeinitializeConnectionRecoveryService();

    bool IsRecoveringFromFailure() const;

    UP2CTravelSubsystem* GetTravelSubsystem() const;

    UPROPERTY(Transient)
    TObjectPtr<UP2COnlineSessionService>
        OnlineSessionService;

    UPROPERTY(Transient)
    TObjectPtr<UP2CConnectionRecoveryService>
        ConnectionRecoveryService;
};