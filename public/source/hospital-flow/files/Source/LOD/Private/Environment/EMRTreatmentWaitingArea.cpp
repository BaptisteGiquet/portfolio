#include "Environment/EMRTreatmentWaitingArea.h"

#include "Components/BoxComponent.h"
#include "Environment/EMRTreatmentWaitingManagerComponent.h"

AEMRTreatmentWaitingArea::AEMRTreatmentWaitingArea()
{
    AreaVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaVolume"));
    TreatmentWaitingManager = CreateDefaultSubobject<UEMRTreatmentWaitingManagerComponent>(TEXT("TreatmentWaitingManager"));
    SetRootComponent(AreaVolume);

    bReplicates = true;

    AreaVolume->SetBoxExtent(FVector(200.f));
    AreaVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
