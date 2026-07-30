#include "P2CBomb.h"

#include "P2CCharacter.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"

AP2CBomb::AP2CBomb()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;
	
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetGenerateOverlapEvents(false);
	
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(RootComponent);
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
	
	CurrentHolder = NewHolder;
	BombState = EP2CBombState::Attached;
	// sets for network
	SetOwner(NewHolder);
	
	AttachToCurrentHolder();
	ApplyStatePresentation();
	
	OnBombHolderChanged.Broadcast(NewHolder);
	ForceNetUpdate();
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
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EP2CBombState::Flying:
	case EP2CBombState::Returning:
	case EP2CBombState::Exploded:
		default:
		break;
	}
}
