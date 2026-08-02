#include "P2CStaminaPickup.h"

#include "Player/P2CPlayerState.h"
#include "Player/Components/P2CPlayerStatsComponent.h"

AP2CStaminaPickup::AP2CStaminaPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(CollisionComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AP2CStaminaPickup::BeginPlay()
{
	Super::BeginPlay();
	
	CollisionComponent->OnComponentBeginOverlap.RemoveDynamic(
		this,
		&ThisClass::HandleOverlap
	);

	CollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&ThisClass::HandleOverlap
	);
}

void AP2CStaminaPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}
	
	AP2CCharacter* Character = Cast<AP2CCharacter>(OtherActor);
	if (!TryCollect(Character))
	{
		return;
	}
	
	Destroy();
}

bool AP2CStaminaPickup::TryCollect(AP2CCharacter* Character)
{
	if (!IsValid(Character))
	{
		return false;
	}
	
	AP2CPlayerState* PlayerState = Character->GetPlayerState<AP2CPlayerState>();
	if (!IsValid(PlayerState) || !PlayerState->IsAlive())
	{
		return false;
	}
	
	UP2CPlayerStatsComponent* PlayerStatsComponent = Character->GetPlayerStatsComponent();
	if (!IsValid(PlayerStatsComponent))
	{
		return false;
	}
	
	if (PlayerStatsComponent->GetCurrentStamina() >= PlayerStatsComponent->GetMaxStamina())
	{
		return false;
	}
	
	PlayerStatsComponent->RestoreStamina(StaminaRestoreAmount);
	return true;
}
