#include "P2CArenaGameMode.h"

#include "P2CArenaGameState.h"
#include "P2CCharacter.h"
#include "P2CPlayerController.h"
#include "Bomb/P2CBomb.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Gameplay/P2CGameRules.h"
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
	
	if (AP2CCharacter* Character = Cast<AP2CCharacter>(NewPlayer->GetPawn()))
	{
		SetCharacterMovementEnabled(Character, false);
	}

	RefreshAlivePlayerCount();
	TryStartPreparation();
}

bool AP2CArenaGameMode::TryThrowBomb(AP2CPlayerController* RequestingController) const
{
	if (!HasAuthority() || !IsValid(RequestingController) || !IsValid(ActiveBomb))
	{
		return false;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState) || ArenaGameState->GetArenaPhase() != EP2CArenaPhase::BombActive)
	{
		return false;
	}
	
	AP2CCharacter* Thrower = Cast<AP2CCharacter>(RequestingController->GetPawn());
	AP2CPlayerState* ThrowerState = Cast<AP2CPlayerState>(Thrower->GetPlayerState());
	
	if (!IsValid(Thrower) || !IsValid(ThrowerState) || !ThrowerState->IsAlive())
	{
		return false;
	}
	
	if (!ActiveBomb->IsHeldBy(Thrower))
	{
		return false;
	}
	
	return ActiveBomb->LaunchFromHolder(Thrower->GetActorForwardVector());
}

bool AP2CArenaGameMode::TryPassBombToCharacter(AP2CBomb* Bomb, AP2CCharacter* TargetCharacter) const
{
	if (!HasAuthority() || !IsValid(Bomb) || Bomb != ActiveBomb || !IsValid(TargetCharacter))
	{
		return false;
	}
	
	AP2CArenaGameState* ArenaGameState = GetGameState<AP2CArenaGameState>();
	if (!IsValid(ArenaGameState) || ArenaGameState->GetArenaPhase() != EP2CArenaPhase::BombActive)
	{
		return false;
	}
	
	AP2CPlayerState* TargetPlayerState = TargetCharacter->GetPlayerState<AP2CPlayerState>();
	if (!IsValid(TargetPlayerState) || !TargetPlayerState->IsAlive())
	{
		return false;
	}
	
	Bomb->AssignToHolder(TargetCharacter);
	return true;
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

void AP2CArenaGameMode::TryStartPreparation()
{
	if (bPreparationStarted || IsValid(ActiveBomb))
	{
		return;
	}
	AP2CCharacter* InitialHolder = ChooseRandomAliveCharacter();

	if (!IsValid(InitialHolder))
	{
		return;
	}
	
	StartPreparation(InitialHolder);
}

void AP2CArenaGameMode::StartPreparation(AP2CCharacter* InitialHolder)
{
	if (bPreparationStarted || !BombClass)
	{
		return;
	}
	
	bPreparationStarted = true;
	SetAllPlayerMovementEnabled(false);
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveBomb = GetWorld()->SpawnActor<AP2CBomb>(BombClass,
		InitialHolder->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParameters);
	
	if (!IsValid(ActiveBomb))
	{
		bPreparationStarted = false;
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn arena bomb."));
		return;
	}
	
	ActiveBomb->AssignToHolder(InitialHolder);
	if (AP2CArenaGameState* ArenaGameState = GetGameState<AP2CArenaGameState>())
	{
		ArenaGameState->SetArenaPhase(EP2CArenaPhase::Preparing);
	}
	
	GetWorldTimerManager().SetTimer(
		PreparationTimerHandle,
		this,
		&ThisClass::ActiveBombRound, 
		PreparationDuration,
		false
		);
}

void AP2CArenaGameMode::ActiveBombRound()
{
	if (!IsValid(ActiveBomb))
	{
		return;
	}
	
	SetAllPlayerMovementEnabled(true);
	if (AP2CArenaGameState* ArenaGameState = GetGameState<AP2CArenaGameState>())
	{
		ArenaGameState->SetArenaPhase(EP2CArenaPhase::BombActive);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Round started"));
}

void AP2CArenaGameMode::SetAllPlayerMovementEnabled(bool bEnabled)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (!IsValid(PlayerController))
		{
			continue;
		}
		
		SetCharacterMovementEnabled(Cast<AP2CCharacter>(PlayerController->GetPawn()), bEnabled);
	}
}

void AP2CArenaGameMode::SetCharacterMovementEnabled(AP2CCharacter* Character, bool bEnabled)
{
	if (!IsValid(Character))
	{
		return;
	}
	
	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		return;
	}
	
	MovementComponent->SetMovementMode(bEnabled ? MOVE_Walking : MOVE_None);
}

AP2CCharacter* AP2CArenaGameMode::ChooseRandomAliveCharacter()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}
	
	TArray<AP2CCharacter*> Candidates;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (!IsValid(PlayerController))
		{
			continue;
		}
		AP2CPlayerState* PlayerState = PlayerController->GetPlayerState<AP2CPlayerState>();
		AP2CCharacter* Character = Cast<AP2CCharacter>(PlayerController->GetPawn());
		if (!IsValid(PlayerState) || !PlayerState->IsAlive() || !IsValid(Character))
		{
			continue;
		}
		
		Candidates.Add(Character);
	}
	
	if (Candidates.Num() < P2CGameRules::MinimumPlayersToStart)
	{
		return nullptr;
	}
	
	const int32 RandomIndex = FMath::RandHelper(Candidates.Num());

	return Candidates[RandomIndex];
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
