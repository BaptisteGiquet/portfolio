#include "Environment/EMRPatientEntryPoint.h"

#include "Components/ArrowComponent.h"

AEMRPatientEntryPoint::AEMRPatientEntryPoint()
{
    ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
    RootComponent = ArrowComponent;
}

FTransform AEMRPatientEntryPoint::GetSpawnTransform() const
{
    return GetActorTransform();
}
