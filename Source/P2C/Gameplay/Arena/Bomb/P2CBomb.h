#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "P2CBomb.generated.h"

class AP2CCharacter;
class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EP2CBombState : uint8
{
	Attached,
	Flying,
	Returning,
	Exploded
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FP2CBombHolderChangedSignature,
	AP2CCharacter*,
	NewHolder
);
UCLASS()
class P2C_API AP2CBomb : public AActor
{
	GENERATED_BODY()

public:
	AP2CBomb();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "P2C|Bomb")
	AP2CCharacter* GetCurrentHolder() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Bomb")
	EP2CBombState GetBombState() const;

	UFUNCTION(BlueprintPure, Category = "P2C|Bomb")
	bool IsHeldBy(const AP2CCharacter* Character) const;

	/**
	 * Server-only
	 */
	void AssignToHolder(AP2CCharacter* NewHolder);
	
	/**
	* Server-only
	* Returns true, when bomb is thrown correctly
	*/
	bool LaunchFromHolder(const FVector& Direction);
		
	/**
	 * Server-only gameplay information.
	 *
	 * Attached  -> CurrentHolder
	 * Flying    -> LastHolder
	 * Returning -> LastHolder
	*/
	AP2CCharacter* GetResponsibleCharacter() const;

	/**
	 * Server-only
	*/
	void Explode();

	UPROPERTY(BlueprintAssignable, Category = "P2C|Bomb")
	FP2CBombHolderChangedSignature OnBombHolderChanged;
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnRep_CurrentHolder();
	
	UFUNCTION()
	void OnRep_BombState();
	
	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);
	
	UFUNCTION()
	void HandleBombOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
	
	void AttachToCurrentHolder();
	void ApplyStatePresentation();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "P2C|Bomb")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "P2C|Bomb")
	TObjectPtr<UStaticMeshComponent> BombMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "P2C|Bomb")
	FName HolderSocketName = TEXT("hand_r");
	
	UPROPERTY(
		ReplicatedUsing = OnRep_CurrentHolder,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "P2C|Bomb"
	)
	TObjectPtr<AP2CCharacter> CurrentHolder;
	
	UPROPERTY(Transient)
	TObjectPtr<AP2CCharacter> LastHolder;
	
	UPROPERTY(ReplicatedUsing = OnRep_BombState,
		VisibleInstanceOnly, 
		BlueprintReadOnly,
		Category = "P2C|Bomb")
	EP2CBombState BombState = EP2CBombState::Attached;
	
	UPROPERTY(
	VisibleAnywhere,
	BlueprintReadOnly,
	Category = "P2C|Bomb"
	)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Bomb",
	meta = (ClampMin = "1.0")
	)
	float ThrowSpeed = 1800.0f;
	
	UPROPERTY(
	EditDefaultsOnly,
	BlueprintReadOnly,
	Category = "P2C|Bomb",
	meta = (ClampMin = "0.1")
	)
	float FlightDuration = 1.25f;
	
private:
	void BeginReturn();
	void HandleFlightTimeout();
	
	FTimerHandle FlightTimeoutHandle;
};