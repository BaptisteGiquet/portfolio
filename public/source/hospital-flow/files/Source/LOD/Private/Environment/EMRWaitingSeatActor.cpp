#include "Environment/EMRWaitingSeatActor.h"

#include "Components/ArrowComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "Components/SceneComponent.h"

AEMRWaitingSeatActor::AEMRWaitingSeatActor()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;

    ApproachArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ApproachArrow"));
    ApproachArrow->SetupAttachment(SceneRoot);
    ApproachArrow->ArrowColor = FColor::Cyan;
    ApproachArrow->bHiddenInGame = true;

    WaitingSeat = CreateDefaultSubobject<UEMRWaitingSeatComponent>(TEXT("WaitingSeat"));
    WaitingSeat->SetupAttachment(SceneRoot);
}
