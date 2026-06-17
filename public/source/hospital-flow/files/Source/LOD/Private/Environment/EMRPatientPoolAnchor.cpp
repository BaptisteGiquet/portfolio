#include "Environment/EMRPatientPoolAnchor.h"

#include "Components/ArrowComponent.h"

AEMRPatientPoolAnchor::AEMRPatientPoolAnchor()
{
    ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
    RootComponent = ArrowComponent;
}

FVector AEMRPatientPoolAnchor::GetPoolLocation() const
{
    return GetActorLocation();
}
