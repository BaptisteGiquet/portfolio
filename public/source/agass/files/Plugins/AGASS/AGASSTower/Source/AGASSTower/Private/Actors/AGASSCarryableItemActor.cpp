#include "Actors/AGASSCarryableItemActor.h"

#include "Data/AGASSItemDefinition.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

AAGASSCarryableItemActor::AAGASSCarryableItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);
	SetMinNetUpdateFrequency(10.f);
}

void AAGASSCarryableItemActor::BeginPlay()
{
	Super::BeginPlay();

	HandleItemDefinitionChanged();
	HandleCarryStateChanged(bIsCarried);
}

void AAGASSCarryableItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemDefinition);
	DOREPLIFETIME(ThisClass, HoldingPlayerState);
	DOREPLIFETIME(ThisClass, bIsCarried);
}

bool AAGASSCarryableItemActor::CanBeClaimedBy(const AController* Controller) const
{
	return Controller != nullptr
		&& Controller->PlayerState != nullptr
		&& !bIsCarried
		&& HoldingPlayerState == nullptr
		&& CanBeClaimedByController(Controller);
}

bool AAGASSCarryableItemActor::Claim(AController* Controller)
{
	return BeginCarry(Controller, false);
}

bool AAGASSCarryableItemActor::ClaimForImmediateCarry(AController* Controller)
{
	return BeginCarry(Controller, true);
}

void AAGASSCarryableItemActor::ReleaseClaim(const bool bRestoreTransform)
{
	EndCarry(bRestoreTransform, nullptr);
}

void AAGASSCarryableItemActor::ReleaseFromCarry(const FTransform& ReleaseTransform)
{
	EndCarry(false, &ReleaseTransform);
}

void AAGASSCarryableItemActor::SetItemDefinition(UAGASSItemDefinition* NewItemDefinition)
{
	if (!HasAuthority() || ItemDefinition == NewItemDefinition)
	{
		return;
	}

	ItemDefinition = NewItemDefinition;
	HandleItemDefinitionChanged();
	ForceNetUpdate();
}

const UAGASSItemDefinition* AAGASSCarryableItemActor::GetItemDefinition() const
{
	return ItemDefinition;
}

bool AAGASSCarryableItemActor::IsCarried() const
{
	return bIsCarried;
}

UClass* AAGASSCarryableItemActor::ResolveSpawnActorClass() const
{
	return ItemDefinition != nullptr ? ItemDefinition->ResolveItemActorClass() : GetClass();
}

bool AAGASSCarryableItemActor::BeginCarry(AController* Controller, const bool bSkipClaimValidation)
{
	if (!HasAuthority()
		|| Controller == nullptr
		|| Controller->PlayerState == nullptr
		|| bIsCarried
		|| HoldingPlayerState != nullptr
		|| (!bSkipClaimValidation && !CanBeClaimedBy(Controller))
		|| (bSkipClaimValidation && !CanBeClaimedByController(Controller)))
	{
		return false;
	}

	ClaimStartTransform = GetActorTransform();
	HoldingPlayerState = Controller->PlayerState;
	bIsCarried = true;
	HandleCarryStarted(Controller);
	HandleCarryStateChanged(true);
	ForceNetUpdate();
	return true;
}

void AAGASSCarryableItemActor::EndCarry(const bool bRestoreTransform, const FTransform* ReleaseTransformOverride)
{
	if (!HasAuthority() || !bIsCarried)
	{
		return;
	}

	HandleCarryEnded();
	HoldingPlayerState = nullptr;
	bIsCarried = false;

	if (ReleaseTransformOverride != nullptr)
	{
		SetActorTransform(*ReleaseTransformOverride, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else if (bRestoreTransform)
	{
		SetActorTransform(ClaimStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	HandleCarryStateChanged(false);
	ForceNetUpdate();
}

bool AAGASSCarryableItemActor::CanBeClaimedByController(const AController* Controller) const
{
	return true;
}

void AAGASSCarryableItemActor::HandleItemDefinitionChanged()
{
}

void AAGASSCarryableItemActor::HandleCarryStarted(AController* ClaimingController)
{
}

void AAGASSCarryableItemActor::HandleCarryEnded()
{
}

void AAGASSCarryableItemActor::HandleCarryStateChanged(const bool bNowCarried)
{
}

void AAGASSCarryableItemActor::OnRep_ItemDefinition()
{
	HandleItemDefinitionChanged();
}

void AAGASSCarryableItemActor::OnRep_HoldingPlayerState()
{
	if (bIsCarried)
	{
		HandleCarryStateChanged(true);
	}
}

void AAGASSCarryableItemActor::OnRep_IsCarried()
{
	HandleCarryStateChanged(bIsCarried);
}
