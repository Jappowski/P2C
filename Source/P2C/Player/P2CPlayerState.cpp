#include "Player/P2CPlayerState.h"

#include "Net/UnrealNetwork.h"

AP2CPlayerState::AP2CPlayerState()
{
	bReplicates = true;
}

bool AP2CPlayerState::IsReady() const
{
	return bIsReady;
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
	
	OnRep_IsReady();
	ForceNetUpdate();
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

	OnReadyStateChanged.Broadcast(bIsReady);
}

void AP2CPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AP2CPlayerState, bIsReady);
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

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"Copied PlayerState properties for %s. Ready: %s"
		),
		*GetPlayerName(),
		bIsReady ? TEXT("true") : TEXT("false")
	);
}