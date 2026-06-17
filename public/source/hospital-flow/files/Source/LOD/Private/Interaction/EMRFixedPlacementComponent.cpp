#include "Interaction/EMRFixedPlacementComponent.h"

#include "Net/UnrealNetwork.h"

UEMRFixedPlacementComponent::UEMRFixedPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEMRFixedPlacementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEMRFixedPlacementComponent, AnchorTransform);
	DOREPLIFETIME(UEMRFixedPlacementComponent, bHasAnchor);
}

void UEMRFixedPlacementComponent::SetAnchorTransform(const FTransform& InTransform)
{
	AnchorTransform = InTransform;
	bHasAnchor = true;
}
