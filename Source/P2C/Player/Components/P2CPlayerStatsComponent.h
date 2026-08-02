#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P2CPlayerStatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FP2CHealthChangedSignature,
	float,
	CurrentHealth,
	float,
	MaxHealth
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FP2CStaminaChangedSignature,
	float,
	CurrentStamina,
	float,
	MaxStamina
);

UCLASS(ClassGroup = (P2C), meta = (BlueprintSpawnableComponent))
class UP2CPlayerStatsComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UP2CPlayerStatsComponent();
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintPure, Category = "P2C|Stats")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Stats")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Stats")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Stats")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Stats")
	bool CanConsumeStamina(float Amount) const;
	
	/**
	 * Server-only
	 *
	 * Returns true when this damage changed health from a positive value to zero
	 */
	bool ApplyDamage(float DamageAmount);
	
	/**
	 * Server-only
	 */
	void RestoreHealth(float Amount);
	
	/**
	 * Server-only
	 *
	 * Returns false when the owner does not have enough stamina
	 */
	bool ConsumeStamina(float Amount);
	
	/**
	 * Server-only
	 */
	void RestoreStamina(float Amount);
	
	/**
	 * Server-only
	 */
	void ResetStats();
	
	UPROPERTY(BlueprintAssignable, Category = "P2C|Stats")
	FP2CHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "P2C|Stats")
	FP2CStaminaChangedSignature OnStaminaChanged;

protected:
	UFUNCTION()
	void OnRep_CurrentHealth();
	
	UFUNCTION()
	void OnRep_CurrentStamina();
	
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "P2C|Stats",
		meta = (ClampMin = "1.0")
	)
	float MaxHealth = 100.0f;
	
	UPROPERTY(
		ReplicatedUsing = OnRep_CurrentHealth,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "P2C|Stats"
	)
	float CurrentHealth = 100.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "P2C|Stats",
		meta = (ClampMin = "0.0")
	)
	float MaxStamina = 100.0f;
	
	UPROPERTY(
		ReplicatedUsing = OnRep_CurrentStamina,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "P2C|Stats"
	)
	float CurrentStamina = 100.0f;

private:
	bool HasServerAuthority() const;
	bool HasNoHealth() const;

	void BroadcastHealthChanged() const;
	void BroadcastStaminaChanged() const;
	void ForceOwnerNetUpdate() const;
};
