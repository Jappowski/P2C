#include "P2CArenaGameMode.h"

#include "P2CArenaGameState.h"
#include "P2CPlayerController.h"
#include "Player/P2CPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogP2CArena, Log, All);

AP2CArenaGameMode::AP2CArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	
	GameStateClass = AP2CArenaGameMode::StaticClass();
	PlayerStateClass = AP2CPlayerState::StaticClass();
	PlayerControllerClass = AP2CPlayerController::StaticClass();
}

void AP2CArenaGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	
	if (!HasAuthority())
	{
		return;
	}
	
	RefreshAlivePlayerCount();
}

void AP2CArenaGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (!HasAuthority())
	{
		return;
	}
	
	InitializeArena();
}

void AP2CArenaGameMode::InitializeArena()
{
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	
	if (!IsValid(ArenaGameState))
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT("Cannot initialize arena: invalid ArenaGameState.")
		);
		
		return;
	}
	
	for (APlayerState* BasePlayerState : ArenaGameState->PlayerArray)
	{
		AP2CPlayerState* P2CPlayerState =
			Cast<AP2CPlayerState>(BasePlayerState);

		if (IsValid(P2CPlayerState))
		{
			P2CPlayerState->ResetArenaState();
		}
	}
	
	ArenaGameState->SetArenaPhase(EP2CArenaPhase::Preparing);
	RefreshAlivePlayerCount();
	
	UE_LOG(
		LogP2CArena,
		Log,
		TEXT("Arena initialized. Connected players: %d"),
		ArenaGameState->GetAlivePlayerCount()
	)
}

void AP2CArenaGameMode::RefreshAlivePlayerCount()
{
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState))
	{
		return;
	}

	int32 AlivePlayers = 0;
	for (const auto& PlayerState : ArenaGameState->PlayerArray)
	{
		const AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(PlayerState);
		if (IsValid(P2CPlayerState) && P2CPlayerState->IsAlive())
		{
			++AlivePlayers;
		}
	}
	
	ArenaGameState->SetAlivePlayerCount(AlivePlayers);
}

AP2CArenaGameState* AP2CArenaGameMode::GetP2CArenaGameState() const
{
	return GetGameState<AP2CArenaGameState>();
}
