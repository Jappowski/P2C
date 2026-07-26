#include "Lobby/P2CLobbyGameMode.h"

#include "P2CPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Player/P2CPlayerState.h"
#include "Map/P2CMapType.h"
#include "Map/P2CTravelSubsystem.h"
#include "Lobby/P2CLobbyGameState.h"

AP2CLobbyGameMode::AP2CLobbyGameMode()
{
	PlayerControllerClass = AP2CPlayerController::StaticClass();
	PlayerStateClass = AP2CPlayerState::StaticClass();
	GameStateClass = AP2CLobbyGameState::StaticClass();
	
	DefaultPawnClass = nullptr;
	bUseSeamlessTravel = true;
}

void AP2CLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Player joined lobby. Connected players: %d"),
		GameState ? GameState->PlayerArray.Num() : 0
	);
}

void AP2CLobbyGameMode::Logout(AController* ExitingController)
{
	Super::Logout(ExitingController);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Player left lobby. Connected players: %d"),
		GameState ? GameState->PlayerArray.Num() : 0
	);
}

bool AP2CLobbyGameMode::AreAllPlayersReady() const
{
	if (!IsValid(GameState))
	{
		return false;
	}

	if (GameState->PlayerArray.Num() < MinimumPlayersToStart)
	{
		return false;
	}

	for (APlayerState* BasePlayerState : GameState->PlayerArray)
	{
		const AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(BasePlayerState);

		if (!IsValid(P2CPlayerState) || !P2CPlayerState->IsReady())
		{
			return false;
		}
	}

	return true;
}

bool AP2CLobbyGameMode::CanStartMatch() const
{
	return HasAuthority()
		&& !bMatchTravelStarted
		&& AreAllPlayersReady();
}

bool AP2CLobbyGameMode::TryStartMatch(
	APlayerController* RequestingController
)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsHostController(RequestingController))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Start match rejected: requester is not the host.")
		);

		return false;
	}

	if (bMatchTravelStarted)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Start match rejected: travel is already in progress.")
		);

		return false;
	}

	if (!AreAllPlayersReady())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Start match rejected: not all players are ready.")
		);

		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();

	if (!IsValid(GameInstance))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Cannot start match: GameInstance is invalid.")
		);

		return false;
	}

	UP2CTravelSubsystem* TravelSubsystem =
		GameInstance->GetSubsystem<UP2CTravelSubsystem>();

	if (!IsValid(TravelSubsystem))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Cannot start match: TravelSubsystem is invalid.")
		);

		return false;
	}

	bMatchTravelStarted = true;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Lobby validation succeeded. Travelling to Arena.")
	);

	const bool bTravelStarted = TravelSubsystem->ServerTravelToMap(
			EP2CMapType::Arena,
			false
		);

	if (!bTravelStarted)
	{
		bMatchTravelStarted = false;

		UE_LOG(
			LogTemp,
			Error,
			TEXT("Cannot start match: ServerTravel failed.")
		);

		return false;
	}

	return true;
}

bool AP2CLobbyGameMode::IsHostController(
	const APlayerController* RequestingController
) const
{
	if (!IsValid(RequestingController))
	{
		return false;
	}

	const ENetMode NetMode = GetNetMode();

	if (NetMode == NM_Standalone)
	{
		return RequestingController->IsLocalController();
	}

	return NetMode == NM_ListenServer
		&& RequestingController->IsLocalController();
}