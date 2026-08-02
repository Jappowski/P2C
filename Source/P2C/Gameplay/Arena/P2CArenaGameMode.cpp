#include "P2CArenaGameMode.h"

#include "EngineUtils.h"
#include "P2CArenaGameState.h"
#include "P2CCharacter.h"
#include "P2CPlayerController.h"
#include "VisualizeTexture.h"
#include "Bomb/P2CBomb.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Gameplay/P2CGameRules.h"
#include "Map/P2CTravelSubsystem.h"
#include "Pickups/P2CStaminaPickup.h"
#include "Player/P2CPlayerState.h"
#include "Player/Components/P2CPlayerStatsComponent.h"

class ATargetPoint;
DEFINE_LOG_CATEGORY_STATIC(LogP2CArena, Log, All);

AP2CArenaGameMode::AP2CArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseSeamlessTravel = true;
	
	GameStateClass = AP2CArenaGameState::StaticClass();
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

void AP2CArenaGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	if (!HasAuthority())
	{
		Super::Logout(Exiting);
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (IsValid(ArenaGameState) && ArenaGameState->GetArenaPhase() == EP2CArenaPhase::RoundEnded)
	{
		Super::Logout(Exiting);
		return;
	}
	
	AP2CPlayerState* ExitingPlayerState = IsValid(Exiting)
			? Exiting->GetPlayerState<AP2CPlayerState>()
			: nullptr;
	
	const FString ExitingPlayerName = IsValid(ExitingPlayerState)
			? ExitingPlayerState->GetPlayerName()
			: TEXT("Unknown");
	
	if (IsValid(ExitingPlayerState) && ExitingPlayerState->IsAlive())
	{
		ExitingPlayerState->SetIsAlive(false);
	}

	UE_LOG(
		LogP2CArena,
		Log,
		TEXT("Player disconnected from Arena: %s"),
		*ExitingPlayerName
	);
	
	CancelCurrentBombCycle();
	Super::Logout(Exiting);
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&ThisClass::ResolveArenaAfterDisconnect
	);
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
	
	if (!IsValid(Thrower))
	{
		return false;
	}
	
	AP2CPlayerState* ThrowerState = Cast<AP2CPlayerState>(Thrower->GetPlayerState());
	if (!IsValid(ThrowerState) || !ThrowerState->IsAlive())
	{
		return false;
	}
	
	if (!ActiveBomb->IsHeldBy(Thrower))
	{
		return false;
	}
	
	UP2CPlayerStatsComponent* StatsComponent = Thrower->GetPlayerStatsComponent();
	if (!IsValid(StatsComponent))
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT(
				"Cannot throw bomb: %s has no StatsComponent."
			),
			*GetNameSafe(Thrower)
		);

		return false;
	}
	
	if (!StatsComponent->CanConsumeStamina(BombThrowStaminaCost))
	{
		return false;
	}
	
	const bool bBombLaunched = ActiveBomb->LaunchFromHolder(Thrower->GetActorForwardVector());

	if (!bBombLaunched)
	{
		return false;
	}
	
	return StatsComponent->ConsumeStamina(BombThrowStaminaCost);
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
	bPreparationStarted = false;
	
	SetAllPlayerMovementEnabled(true);
	if (AP2CArenaGameState* ArenaGameState = GetGameState<AP2CArenaGameState>())
	{
		ArenaGameState->SetArenaPhase(EP2CArenaPhase::BombActive);
	}
	
	StartBombFuse();
	StartStaminaPickupCycle();
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

		AP2CCharacter* Character = Cast<AP2CCharacter>(PlayerController->GetPawn());
		if (!IsValid(Character))
		{
			continue;
		}

		if (bEnabled)
		{
			const AP2CPlayerState* PlayerState = PlayerController->GetPlayerState<AP2CPlayerState>();
			const bool bCanMove = IsValid(PlayerState) && PlayerState->IsAlive();

			SetCharacterMovementEnabled(
				Character,
				bCanMove
			);
		}
		else
		{
			SetCharacterMovementEnabled(
				Character,
				false
			);
		}
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
	CacheStaminaPickupSpawnPoints();
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
		AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(BasePlayerState);

		if (!IsValid(P2CPlayerState))
		{
			return;
		}
		
		P2CPlayerState->ResetArenaState();
		
		APawn* Pawn = P2CPlayerState->GetPawn();
		AP2CCharacter* Character = Cast<AP2CCharacter>(Pawn);

		if (!IsValid(Character))
		{
			continue;
		}

		Character->SetActorHiddenInGame(false);
		Character->SetActorEnableCollision(true);

		SetCharacterMovementEnabled(Character, false);
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

void AP2CArenaGameMode::StartBombFuse()
{
	if (!HasAuthority() || !IsValid(ActiveBomb))
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(BombFuseTimerHandle);
	
	const float FuseDuration = FMath::FRandRange(
		MinimumBombFuseDuration,
		MaximumBombFuseDuration
	);
	
	GetWorldTimerManager().SetTimer(
		BombFuseTimerHandle,
		this,
		&ThisClass::HandleBombFuseExpired,
		FuseDuration,
		false
	);
	
	UE_LOG(
		LogP2CArena,
		Log,
		TEXT("Bomb fuse started. Duration: %.2f seconds."),
		FuseDuration
	);
}

void AP2CArenaGameMode::HandleBombFuseExpired()
{
	if (!HasAuthority() || !IsValid(ActiveBomb))
	{
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState) || ArenaGameState->GetArenaPhase() != EP2CArenaPhase::BombActive)
	{
		return;
	}
	
	AP2CCharacter* ResponsibleCharacter = ActiveBomb->GetResponsibleCharacter();
	ResetStaminaPickupCycle();
	ArenaGameState->SetArenaPhase(EP2CArenaPhase::ResolvingExplosion);
	ActiveBomb->Explode();
	
	if (!IsValid(ResponsibleCharacter))
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT(
				"Bomb exploded, but responsible "
				"character is invalid."
			)
		);
		return;
	}

	EliminateCharacter(ResponsibleCharacter);
	RefreshAlivePlayerCount();
	ResolveExplosion();
}

void AP2CArenaGameMode::EliminateCharacter(AP2CCharacter* Character)
{
	if (!HasAuthority() || !IsValid(Character))
	{
		return;
	}
	
	AP2CPlayerState* PlayerState = Character->GetPlayerState<AP2CPlayerState>();
	UP2CPlayerStatsComponent* PlayerStatsComponent = Character->GetPlayerStatsComponent();
	
	if (!IsValid(PlayerState) || !IsValid(PlayerStatsComponent))
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT(
				"Cannot eliminate %s: missing "
				"PlayerState or StatsComponent."
			),
			*GetNameSafe(Character)
		);
		return;
	}
	
	if (!PlayerState->IsAlive())
	{
		return;
	}
	
	// Bomb one shots player by design
	PlayerStatsComponent->ApplyDamage(PlayerStatsComponent->GetMaxHealth());
	
	PlayerState->SetIsAlive(false);
	Character->SetActorHiddenInGame(true);
	Character->SetActorEnableCollision(false);
	SetCharacterMovementEnabled(Character, false);
	PendingOverviewPlayerController = Cast<AP2CPlayerController>(Character->GetController());
	
	UE_LOG(
		LogP2CArena,
		Log,
		TEXT("Player %s was eliminated."),
		*PlayerState->GetPlayerName()
	);
}

void AP2CArenaGameMode::ResolveExplosion()
{
	if (!HasAuthority())
	{
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState))
	{
		return;
	}
	
	if (ArenaGameState->GetAlivePlayerCount() <= 1)
	{
		EndArenaRound();
		return;
	}
	
	GetWorldTimerManager().SetTimer(
		NextBombCycleTimerHandle,
		this,
		&ThisClass::CompletedExplosionResolution,
		ExplosionResolutionDuration,
		false
		);
}

void AP2CArenaGameMode::StartNextBombCycle()
{
	if (!HasAuthority())
	{
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState))
	{
		return;
	}
	
	if (ArenaGameState->GetAlivePlayerCount() <= 1)
	{
		EndArenaRound();
		return;
	}
	
	GetWorldTimerManager().ClearTimer(PreparationTimerHandle);
	GetWorldTimerManager().ClearTimer(BombFuseTimerHandle);
	
	if (IsValid(ActiveBomb))
	{
		ActiveBomb->Destroy();
		ActiveBomb = nullptr;
	}
	
	ResetAlivePlayersForNextCycle();
	bPreparationStarted = false;
	
	TryStartPreparation();
}

void AP2CArenaGameMode::ResetAlivePlayersForNextCycle()
{
	TArray<AP2CCharacter*> AliveCharacters;
	GatherAliveCharacters(AliveCharacters);

	TArray<APlayerStart*> AvailablePlayerStarts;
	GatherPlayerStarts(AvailablePlayerStarts);

	if (AvailablePlayerStarts.Num() < AliveCharacters.Num())
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT(
				"Not enough PlayerStarts. "
				"Alive players: %d, PlayerStarts: %d"
			),
			AliveCharacters.Num(),
			AvailablePlayerStarts.Num()
		);

		return;
	}

	ShufflePlayerStarts(AvailablePlayerStarts);

	for (int32 Index = 0; Index < AliveCharacters.Num(); ++Index)
	{
		ResetAliveCharacter(
			AliveCharacters[Index],
			AvailablePlayerStarts[Index]
		);
	}
}

void AP2CArenaGameMode::EndArenaRound()
{
	if (!HasAuthority())
	{
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState))
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(PreparationTimerHandle);
	GetWorldTimerManager().ClearTimer(BombFuseTimerHandle);
	GetWorldTimerManager().ClearTimer(NextBombCycleTimerHandle);
	
	SetAllPlayerMovementEnabled(false);
	
	AP2CPlayerState* WinnerPlayerState = nullptr;
	for (APlayerState* BasePlayerState : ArenaGameState->PlayerArray)
	{
		AP2CPlayerState* P2CPlayerState = Cast<AP2CPlayerState>(BasePlayerState);
		if (IsValid(P2CPlayerState) && P2CPlayerState->IsAlive())
		{
			WinnerPlayerState = P2CPlayerState;
			break;
		}
	}
	
	if (IsValid(WinnerPlayerState))
	{
		WinnerPlayerState->AddMatchPoints(1);
		ArenaGameState->SetRoundWinnerName(WinnerPlayerState->GetPlayerName());
	}
	else
	{
		ArenaGameState->SetRoundWinnerName(TEXT(""));
	}
	
	ResetStaminaPickupCycle();
	ArenaGameState->SetArenaPhase(EP2CArenaPhase::RoundEnded);
	
	GetWorldTimerManager().SetTimer(
		ReturnToLobbyTimerHandle,
		this,
		&ThisClass::ReturnToLobby,
		RoundEndDuration,
		false
	);
}

void AP2CArenaGameMode::ReturnToLobby()
{
	if (!HasAuthority())
	{
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState))
	{
		return;
	}
	
	for (APlayerState* BasePlayerState : ArenaGameState->PlayerArray)
	{
		AP2CPlayerState* PlayerState = Cast<AP2CPlayerState>(BasePlayerState);
		if (IsValid(PlayerState))
		{
			PlayerState->SetReady(false);
		}
	}
	
	UGameInstance* GameInstance = GetGameInstance();

	if (!IsValid(GameInstance))
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT("Cannot return to Lobby: GameInstance is invalid.")
		);
		return;
	}

	UP2CTravelSubsystem* TravelSubsystem = GameInstance->GetSubsystem<UP2CTravelSubsystem>();
	if (IsValid(TravelSubsystem))
	{
		const bool bTravelStarted = TravelSubsystem->ServerTravelToMap(EP2CMapType::Lobby, false);

		if (!bTravelStarted)
		{
			UE_LOG(
				LogP2CArena,
				Error,
				TEXT(
					"Could not start server travel "
					"from Arena to Lobby."
				)
			);
		}
	}
}

void AP2CArenaGameMode::GatherAliveCharacters(TArray<AP2CCharacter*>& OutCharacters) const
{
	OutCharacters.Reset();

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

		AP2CPlayerState* PlayerState = PlayerController->GetPlayerState<AP2CPlayerState>();
		AP2CCharacter* Character = Cast<AP2CCharacter>(PlayerController->GetPawn());

		if (IsValid(PlayerState) && PlayerState->IsAlive() && IsValid(Character))
		{
			OutCharacters.Add(Character);
		}
	}
}

void AP2CArenaGameMode::GatherPlayerStarts(TArray<APlayerStart*>& OutPlayerStarts) const
{
	OutPlayerStarts.Reset();
	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<APlayerStart> Iterator(World); Iterator; ++Iterator)
	{
		APlayerStart* PlayerStart = *Iterator;

		if (IsValid(PlayerStart))
		{
			OutPlayerStarts.Add(PlayerStart);
		}
	}
}

void AP2CArenaGameMode::ShufflePlayerStarts(TArray<APlayerStart*>& PlayerStarts) const
{
	for (int32 Index = PlayerStarts.Num() - 1; Index > 0; --Index)
	{
		const int32 RandomIndex = FMath::RandRange(0, Index);

		PlayerStarts.Swap(Index, RandomIndex);
	}
}

void AP2CArenaGameMode::ResetAliveCharacter(AP2CCharacter* Character, APlayerStart* PlayerStart)
{
	if (!IsValid(Character) || !IsValid(PlayerStart))
	{
		return;
	}
	
	Character->SetActorHiddenInGame(false);
	Character->SetActorEnableCollision(true);

	if (UP2CPlayerStatsComponent* StatsComponent = Character->GetPlayerStatsComponent())
	{
		StatsComponent->ResetStats();
	}

	if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	const FRotator SpawnRotation(
		0.0f,
		PlayerStart->GetActorRotation().Yaw,
		0.0f
	);

	Character->SetActorLocationAndRotation(
		PlayerStart->GetActorLocation(),
		SpawnRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	AP2CPlayerController* P2CPlayerController = Cast<AP2CPlayerController>(Character->GetController());

	if (IsValid(P2CPlayerController))
	{
		P2CPlayerController->SetControlRotation(SpawnRotation);
		
		P2CPlayerController->ClientHandleArenaTeleport(SpawnRotation);
	}

	UE_LOG(
		LogP2CArena,
		Verbose,
		TEXT(
			"Reset %s at %s."
		),
		*GetNameSafe(Character),
		*GetNameSafe(PlayerStart)
	);
}

void AP2CArenaGameMode::CompletedExplosionResolution()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (PendingOverviewPlayerController.IsValid())
	{
		PendingOverviewPlayerController->ClientEnterArenaOverview();
	}
	
	PendingOverviewPlayerController.Reset();
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState))
	{
		return;
	}
	
	if (ArenaGameState->GetAlivePlayerCount() <= 1)
	{
		EndArenaRound();
		return;
	}

	StartNextBombCycle();
}

void AP2CArenaGameMode::CacheStaminaPickupSpawnPoints()
{
	PickupSpawnPoints.Reset();
	UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		return;
	}
	
	for (TActorIterator<ATargetPoint> Iterator(World); Iterator; ++Iterator)
	{
		ATargetPoint* SpawnPoint = *Iterator;
		if (IsValid(SpawnPoint) && SpawnPoint->ActorHasTag(PickupSpawnTag))
		{
			PickupSpawnPoints.Add(SpawnPoint);
		}
	}
	
	UE_LOG(
		LogP2CArena,
		Log,
		TEXT("Found %d stamina pickup spawn points"),
		PickupSpawnPoints.Num()
	);
}

void AP2CArenaGameMode::StartStaminaPickupCycle()
{
	if (!HasAuthority())
	{
		return;
	}
	ResetStaminaPickupCycle();
	if (!StaminaPickupClass || PickupSpawnPoints.IsEmpty())
	{
		return;
	}
	
	const float MinDelay =
		FMath::Min(
			PickupSpawnDelayMin,
			PickupSpawnDelayMax
		);

	const float MaxDelay =
		FMath::Max(
			PickupSpawnDelayMin,
			PickupSpawnDelayMax
		);

	const float SpawnDelay = FMath::FRandRange(MinDelay, MaxDelay);
	if (SpawnDelay <= 0.0f)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this,
			&ThisClass::SpawnStaminaPickup
		);

		return;
	}
	
	GetWorldTimerManager().SetTimer(
		PickupSpawnTimerHandle,
		this,
		&ThisClass::SpawnStaminaPickup,
		SpawnDelay,
		false
	);
}

void AP2CArenaGameMode::SpawnStaminaPickup()
{
	if (!HasAuthority())
	{
		return;
	}
	
	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();
	if (!IsValid(ArenaGameState) || ArenaGameState->GetArenaPhase() != EP2CArenaPhase::BombActive)
	{
		return;
	}
	
	if (!StaminaPickupClass || PickupSpawnPoints.IsEmpty() ||ActiveStaminaPickup.IsValid())
	{
		return;
	}
	
	TArray<ATargetPoint*> ValidSpawnPoints;

	for (const TWeakObjectPtr<ATargetPoint>& SpawnPoint : PickupSpawnPoints)
	{
		if (SpawnPoint.IsValid())
		{
			ValidSpawnPoints.Add(SpawnPoint.Get());
		}
	}
	
	if (ValidSpawnPoints.IsEmpty())
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT(
				"Cannot spawn stamina pickup: "
				"no valid spawn points."
			)
		);

		return;
	}
	
	ATargetPoint* SelectedSpawnPoint =
		ValidSpawnPoints[
			FMath::RandRange(
				0,
				ValidSpawnPoints.Num() - 1
			)
		];

	FActorSpawnParameters SpawnParameters;

	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AP2CStaminaPickup* SpawnedPickup = GetWorld()->SpawnActor<AP2CStaminaPickup>(
			StaminaPickupClass,
			SelectedSpawnPoint->GetActorTransform(),
			SpawnParameters
		);
	
	if (!IsValid(SpawnedPickup))
	{
		UE_LOG(
			LogP2CArena,
			Error,
			TEXT("Failed to spawn stamina pickup.")
		);

		return;
	}
	
	ActiveStaminaPickup = SpawnedPickup;

	UE_LOG(
		LogP2CArena,
		Log,
		TEXT("Spawned stamina pickup at %s."),
		*GetNameSafe(SelectedSpawnPoint)
	);
}

void AP2CArenaGameMode::ResetStaminaPickupCycle()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PickupSpawnTimerHandle);

	if (ActiveStaminaPickup.IsValid())
	{
		ActiveStaminaPickup->Destroy();
	}

	ActiveStaminaPickup.Reset();
}

void AP2CArenaGameMode::ResolveArenaAfterDisconnect()
{
	if (!HasAuthority())
	{
		return;
	}

	AP2CArenaGameState* ArenaGameState = GetP2CArenaGameState();

	if (!IsValid(ArenaGameState))
	{
		return;
	}
	
	RefreshAlivePlayerCount();
	const int32 AlivePlayerCount = ArenaGameState->GetAlivePlayerCount();
	if (AlivePlayerCount <= 1)
	{
		EndArenaRound();
		return;
	}

	ArenaGameState->SetArenaPhase(
			EP2CArenaPhase::Preparing
		);

	TryStartPreparation();
}

void AP2CArenaGameMode::CancelCurrentBombCycle()
{
	GetWorldTimerManager().ClearTimer(PreparationTimerHandle);
	GetWorldTimerManager().ClearTimer(BombFuseTimerHandle);
	GetWorldTimerManager().ClearTimer(NextBombCycleTimerHandle);
	GetWorldTimerManager().ClearTimer(ReturnToLobbyTimerHandle);
	GetWorldTimerManager().ClearTimer(PickupSpawnTimerHandle);

	bPreparationStarted = false;
	SetAllPlayerMovementEnabled(false);

	if (IsValid(ActiveBomb))
	{
		ActiveBomb->Destroy();
		ActiveBomb = nullptr;
	}

	ResetStaminaPickupCycle();
}
