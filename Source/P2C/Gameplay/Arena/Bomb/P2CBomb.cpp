#include "P2CBomb.h"

#include "EngineUtils.h"
#include "P2CCharacter.h"
#include "Projects.h"
#include "Chaos/AABBTree.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Gameplay/Arena/P2CArenaGameMode.h"
#include "Net/UnrealNetwork.h"

class AP2CArenaGameMode;

AP2CBomb::AP2CBomb()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	AActor::SetReplicateMovement(true);

	CollisionComponent =CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	BombMesh =CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(CollisionComponent);

	ProjectileMovement =CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->bAutoActivate = false;
	
	BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AP2CBomb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AP2CBomb, CurrentHolder);
	DOREPLIFETIME(AP2CBomb, BombState);
}

AP2CCharacter* AP2CBomb::GetCurrentHolder() const
{
	return CurrentHolder;
}

EP2CBombState AP2CBomb::GetBombState() const
{
	return BombState;
}

bool AP2CBomb::IsHeldBy(const AP2CCharacter* Character) const
{
	return CurrentHolder == Character;
}

void AP2CBomb::AssignToHolder(AP2CCharacter* NewHolder)
{
	if (!HasAuthority())
	{
		ensureMsgf(false, TEXT("AssignToHolder may only be called by the server"));
		return;
	}
	
	if (!IsValid(NewHolder))
	{
		ensureMsgf(false, TEXT("Cannot assign bomb to an invalid holder"));
		return;
	}
	
	if (IsValid(LastHolder))
	{
		CollisionComponent->IgnoreActorWhenMoving(LastHolder, false);
	}
	
	LastHolder = nullptr;
	
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	
	CurrentHolder = NewHolder;
	BombState = EP2CBombState::Attached;
	// sets for network
	SetOwner(NewHolder);
	
	AttachToCurrentHolder();
	ApplyStatePresentation();
	
	OnBombHolderChanged.Broadcast(NewHolder);
	ForceNetUpdate();
}

bool AP2CBomb::LaunchFromHolder(const FVector& Direction)
{
	if (
		!HasAuthority() ||
		BombState != EP2CBombState::Attached ||
		!IsValid(CurrentHolder)
	)
	{
		return false;
	}

	const FVector NormalizedDirection = Direction.GetSafeNormal();

	if (NormalizedDirection.IsNearlyZero())
	{
		return false;
	}

	LastHolder = CurrentHolder;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	CurrentHolder = nullptr;
	BombState = EP2CBombState::Flying;

	SetOwner(nullptr);
	ApplyStatePresentation();
	
	ProjectileMovement->Velocity = NormalizedDirection * ThrowSpeed;
	ProjectileMovement->Activate(true);
	ProjectileMovement->UpdateComponentVelocity();

	OnBombHolderChanged.Broadcast(nullptr);

	ForceNetUpdate();

	return true;
}

void AP2CBomb::BeginPlay()
{
	Super::BeginPlay();
	
	CollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&ThisClass::HandleBombOverlap
	);
	
	ProjectileMovement->OnProjectileStop.AddDynamic(
		this,
		&ThisClass::HandleProjectileStop
	);
}

void AP2CBomb::OnRep_CurrentHolder()
{
	AttachToCurrentHolder();
	OnBombHolderChanged.Broadcast(CurrentHolder);
}

void AP2CBomb::OnRep_BombState()
{
	ApplyStatePresentation();
}

void AP2CBomb::HandleProjectileStop(const FHitResult& ImpactResult)
{
	UE_LOG(LogTemp, Error, TEXT("PROJECTILE STOP"));
}

void AP2CBomb::HandleBombOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || BombState != EP2CBombState::Flying)
	{
		return;
	}

	AP2CCharacter* HitCharacter =Cast<AP2CCharacter>(OtherActor);

	if (!IsValid(HitCharacter) || HitCharacter == LastHolder)
	{
		return;
	}

	AP2CArenaGameMode* ArenaGameMode = GetWorld()
			? GetWorld()->GetAuthGameMode<AP2CArenaGameMode>()
			: nullptr;

	if (!IsValid(ArenaGameMode))
	{
		return;
	}

	ArenaGameMode->TryPassBombToCharacter(
		this,
		HitCharacter
	);
}

void AP2CBomb::AttachToCurrentHolder()
{
	if (!IsValid(CurrentHolder))
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		return;
	}
	
	USkeletalMeshComponent* HolderMesh = CurrentHolder->GetMesh();
	if (!IsValid(HolderMesh))
	{
		return;
	}
	
	AttachToComponent(HolderMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolderSocketName);
}

void AP2CBomb::ApplyStatePresentation()
{
	switch (BombState)
	{
	case EP2CBombState::Attached:
		CollisionComponent->SetGenerateOverlapEvents(false);
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EP2CBombState::Flying:
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionComponent->SetGenerateOverlapEvents(true);
		break;
	case EP2CBombState::Returning:
	case EP2CBombState::Exploded:
		default:
		CollisionComponent->SetGenerateOverlapEvents(false);
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}
