#include "Interaction/EMRLabAnalyzerTubeSpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AEMRLabAnalyzerTubeSpawnPoint::AEMRLabAnalyzerTubeSpawnPoint()
{
    PrimaryActorTick.bCanEverTick = false;

    SpawnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnRoot"));
    SetRootComponent(SpawnRoot);

    ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
    ArrowComponent->SetupAttachment(SpawnRoot);
    ArrowComponent->bIsScreenSizeScaled = true;
}
