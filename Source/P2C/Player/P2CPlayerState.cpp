#include "Player/P2CPlayerState.h"

#include "Net/UnrealNetwork.h"

AP2CPlayerState::AP2CPlayerState()
{
	bReplicates = true;
}


void AP2CPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AP2CPlayerState, bIsReady);
	DOREPLIFETIME(AP2CPlayerState, MatchPoints);
	DOREPLIFETIME(AP2CPlayerState, bIsAlive);
}

bool AP2CPlayerState::IsReady() const
{
	return bIsReady;
}

bool AP2CPlayerState::IsAlive() const
{
	return bIsAlive;
}

int32 AP2CPlayerState::GetMatchPoints() const
{
	return MatchPoints;
}

void AP2CPlayerState::SetReady(const bool bNewReady)
{
	if (!HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SetReady rejected: PlayerState has no authority.")
		);

		return;
	}

	if (bIsReady == bNewReady)
	{
		return;
	}

	bIsReady = bNewReady;
	OnPlayerDataChanged.Broadcast();
	OnRep_IsReady();
	ForceNetUpdate();
}

void AP2CPlayerState::AddMatchPoints(int32 Points)
{
	if (!HasAuthority())
	{
		ensureMsgf(
			false,
			TEXT("AddMatchPoints may only be called by the server.")
		);

		return;
	}
	
	if (Points == 0)
	{
		return;
	}
	
	MatchPoints += Points;
	
	OnMatchPointsChanged.Broadcast(MatchPoints);
	ForceNetUpdate();
}

void AP2CPlayerState::SetIsAlive(bool bNewIsAlive)
{
	if (!HasAuthority())
	{
		ensureMsgf(
			false,
			TEXT("SetIsAlive may only be called by the server.")
		);
		
		return;
	}
	
	if (bIsAlive == bNewIsAlive)
	{
		return;
	}
	
	bIsAlive = bNewIsAlive;
	OnAliveStateChanged.Broadcast(bIsAlive);
	ForceNetUpdate();
}

void AP2CPlayerState::ResetArenaState()
{
	if (!HasAuthority())
	{
		ensureMsgf(
			false,
			TEXT("ResetArenaState may only be called by the server.")
		);
		
		return;
	}
	
	const bool bPointsChanged = MatchPoints != 0;
	const bool bAliveChanged = !bIsAlive;
	
	bIsAlive = true;

	if (bAliveChanged)
	{
		OnAliveStateChanged.Broadcast(bIsAlive);
	}

	if (bPointsChanged || bAliveChanged)
	{
		ForceNetUpdate();
	}
}

void AP2CPlayerState::OnRep_IsReady()
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Player %s ready state changed to: %s"),
		*GetPlayerName(),
		bIsReady ? TEXT("Ready") : TEXT("Not Ready")
	);
	OnPlayerDataChanged.Broadcast();
	OnReadyStateChanged.Broadcast(bIsReady);
}

void AP2CPlayerState::OnRep_MatchPoints()
{
	OnMatchPointsChanged.Broadcast(MatchPoints);
}

void AP2CPlayerState::OnRep_IsAlive()
{
	OnAliveStateChanged.Broadcast(bIsAlive);
}

void AP2CPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	
	OnPlayerDataChanged.Broadcast();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Player name replicated. PlayerState: %s, Name: %s"),
		*GetNameSafe(this),
		*GetPlayerName()
	);
}

void AP2CPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	AP2CPlayerState* NewPlayerState = Cast<AP2CPlayerState>(PlayerState);

	if (!IsValid(NewPlayerState))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Could not copy P2CPlayerState properties.")
		);

		return;
	}

	NewPlayerState->bIsReady = bIsReady;
	NewPlayerState->MatchPoints = MatchPoints;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"Copied PlayerState properties for %s. "
			"Ready: %s, MatchPoints: %d"
		),
		*GetPlayerName(),
		bIsReady ? TEXT("true") : TEXT("false"),
		MatchPoints
	);
}