#pragma once

#include "P2CCharacter.h"
#include "Components/SphereComponent.h"
#include "P2CStaminaPickup.generated.h"

UCLASS(Abstract)
class P2C_API AP2CStaminaPickup : public AActor
{
	GENERATED_BODY()

public:
	AP2CStaminaPickup();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "P2C|Pickup"
	)
	TObjectPtr<USphereComponent> CollisionComponent;
	
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "P2C|Pickup"
	)
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "P2C|Pickup",
		meta = (ClampMin = "1.0")
	)
	float StaminaRestoreAmount = 20.0f;
	
private:
	UFUNCTION()
	void HandleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	bool TryCollect(AP2CCharacter* Character);
};