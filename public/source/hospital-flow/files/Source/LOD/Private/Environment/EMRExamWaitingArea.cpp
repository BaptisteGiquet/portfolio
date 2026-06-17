#include "Environment/EMRExamWaitingArea.h"

#include "Components/BoxComponent.h"
#include "Environment/EMRExamWaitingManagerComponent.h"

AEMRExamWaitingArea::AEMRExamWaitingArea()
{
    AreaVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaVolume"));
    ExamWaitingManager = CreateDefaultSubobject<UEMRExamWaitingManagerComponent>(TEXT("ExamWaitingManager"));
    SetRootComponent(AreaVolume);

    bReplicates = true;

    AreaVolume->SetBoxExtent(FVector(200.f));
    AreaVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
