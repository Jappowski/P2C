#include "P2CPlayerStatsComponent.h"

#include "Net/UnrealNetwork.h"

UP2CPlayerStatsComponent::UP2CPlayerStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UP2CPlayerStatsComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasServerAuthority())
	{
		ResetStats();
	}
}

void UP2CPlayerStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UP2CPlayerStatsComponent, CurrentHealth);
	DOREPLIFETIME_CONDITION(UP2CPlayerStatsComponent, CurrentStamina, COND_OwnerOnly);
	
}

float UP2CPlayerStatsComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UP2CPlayerStatsComponent::GetMaxHealth() const
{
	return MaxHealth;
}

float UP2CPlayerStatsComponent::GetCurrentStamina() const
{
	return CurrentStamina;
}

float UP2CPlayerStatsComponent::GetMaxStamina() const
{
	return MaxStamina;
}

bool UP2CPlayerStatsComponent::CanConsumeStamina(float Amount) const
{
	if (Amount <= 0.0f)
	{
		return false;
	}

	return CurrentStamina >= Amount;
}

bool UP2CPlayerStatsComponent::ApplyDamage(float DamageAmount)
{
	if (!HasServerAuthority())
	{
		ensureMsgf(false, TEXT("ApplyDamage may only be called by the server."));
		return false;
	}
	
	if (DamageAmount <= 0.0f || HasNoHealth())
	{
		return false;
	}
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(
		CurrentHealth - DamageAmount,
		0.0f,
		MaxHealth
	);
	
	BroadcastHealthChanged();
	ForceOwnerNetUpdate();

	return PreviousHealth > 0.0f && CurrentHealth <= 0.0f;
}

void UP2CPlayerStatsComponent::RestoreHealth(float Amount)
{
	if (!HasServerAuthority())
	{
		ensureMsgf(false, TEXT("RestoreHealth may only be called by the server."));
		return;
	}
	
	if (Amount <= 0.0f || HasNoHealth())
	{
		return;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	if (FMath::IsNearlyEqual(PreviousHealth, CurrentHealth))
	{
		return;
	}
	
	BroadcastHealthChanged();
	ForceOwnerNetUpdate();
}

bool UP2CPlayerStatsComponent::ConsumeStamina(float Amount)
{
	if (!HasServerAuthority())
	{
		ensureMsgf(false, TEXT("ConsumeStamina may only be called by the server."));
		return false;
	}

	if (!CanConsumeStamina(Amount))
	{
		return false;
	}

	CurrentStamina = FMath::Clamp(
		CurrentStamina - Amount,
		0.0f,
		MaxStamina
	);

	BroadcastStaminaChanged();
	ForceOwnerNetUpdate();

	return true;
}

void UP2CPlayerStatsComponent::RestoreStamina(float Amount)
{
	if (!HasServerAuthority())
	{
		ensureMsgf(false, TEXT("RestoreStamina may only be called by the server."));
		return;
	}

	if (Amount <= 0.0f)
	{
		return;
	}

	const float PreviousStamina = CurrentStamina;

	CurrentStamina = FMath::Clamp(
		CurrentStamina + Amount,
		0.0f,
		MaxStamina
	);

	if (FMath::IsNearlyEqual(PreviousStamina, CurrentStamina))
	{
		return;
	}

	BroadcastStaminaChanged();
	ForceOwnerNetUpdate();
}

void UP2CPlayerStatsComponent::ResetStats()
{
	if (!HasServerAuthority())
	{
		ensureMsgf(false, TEXT("ResetStats may only be called by the server."));
		return;
	}

	const bool bHealthChanged = !FMath::IsNearlyEqual(
		CurrentHealth,
		MaxHealth
	);

	const bool bStaminaChanged = !FMath::IsNearlyEqual(
		CurrentStamina,
		MaxStamina
	);

	CurrentHealth = MaxHealth;
	CurrentStamina = MaxStamina;

	if (bHealthChanged)
	{
		BroadcastHealthChanged();
	}

	if (bStaminaChanged)
	{
		BroadcastStaminaChanged();
	}

	if (bHealthChanged || bStaminaChanged)
	{
		ForceOwnerNetUpdate();
	}
}

void UP2CPlayerStatsComponent::OnRep_CurrentHealth()
{
	BroadcastHealthChanged();
}

void UP2CPlayerStatsComponent::OnRep_CurrentStamina()
{
	BroadcastStaminaChanged();
}

bool UP2CPlayerStatsComponent::HasServerAuthority() const
{
	const AActor* Owner = GetOwner();
	return IsValid(Owner) && Owner->HasAuthority();
}

bool UP2CPlayerStatsComponent::HasNoHealth() const
{
	return CurrentHealth <= 0;
}

void UP2CPlayerStatsComponent::BroadcastHealthChanged() const
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UP2CPlayerStatsComponent::BroadcastStaminaChanged() const
{
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UP2CPlayerStatsComponent::ForceOwnerNetUpdate() const
{
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}
