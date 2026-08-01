#include "P2CArenaGameState.h"

#include "Net/UnrealNetwork.h"

AP2CArenaGameState::AP2CArenaGameState()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AP2CArenaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AP2CArenaGameState, ArenaPhase);
	DOREPLIFETIME(AP2CArenaGameState, AlivePlayerCount);
}

EP2CArenaPhase AP2CArenaGameState::GetArenaPhase()
{
	return ArenaPhase;
}

int32 AP2CArenaGameState::GetAlivePlayerCount()
{
	return AlivePlayerCount;
}

void AP2CArenaGameState::SetArenaPhase(EP2CArenaPhase NewPhase)
{
	if (!HasAuthority())
	{
		ensureMsgf(false, TEXT("Only the server can set the arena phase."));
		return;
	}
	
	if (ArenaPhase == NewPhase)
	{
		return;
	}
	
	ArenaPhase = NewPhase;
	// Broadcast becouse onrep is not executed on the server.
	OnArenaPhaseChanged.Broadcast(ArenaPhase);
	
	ForceNetUpdate();
}

void AP2CArenaGameState::SetAlivePlayerCount(int32 NewAlivePlayerCount)
{
	if (!HasAuthority())
	{
		ensureMsgf(false, TEXT("Only the server can set the alive player count."));
		return;
	}
	const int32 ClampedCount = FMath::Max(0, NewAlivePlayerCount);
	if (AlivePlayerCount == ClampedCount)
	{
		return;
	}
	
	AlivePlayerCount = ClampedCount;
	OnAlivePlayerCountChanged.Broadcast(AlivePlayerCount);
	ForceNetUpdate();
}

void AP2CArenaGameState::OnRep_ArenaPhase()
{
	OnArenaPhaseChanged.Broadcast(ArenaPhase);
}

void AP2CArenaGameState::OnRep_AlivePlayerCount()
{
	OnAlivePlayerCountChanged.Broadcast(AlivePlayerCount);
}

