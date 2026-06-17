#include "Framework/EMRHubSummaryDisplayActor.h"

#include "Blueprint/UserWidget.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"

AEMRHubSummaryDisplayActor::AEMRHubSummaryDisplayActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    SummaryWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SummaryWidget"));
    SummaryWidgetComponent->SetupAttachment(Root);
    SummaryWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    SummaryWidgetComponent->SetDrawAtDesiredSize(true);
    SummaryWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SummaryWidgetComponent->SetGenerateOverlapEvents(false);
    SummaryWidgetComponent->SetWindowFocusable(false);
}

void AEMRHubSummaryDisplayActor::BeginPlay()
{
    Super::BeginPlay();
    InitializeSummaryWidget();
}

void AEMRHubSummaryDisplayActor::InitializeSummaryWidget()
{
    if (!SummaryWidgetComponent)
    {
        return;
    }

    if (SummaryWidgetClass && SummaryWidgetComponent->GetWidgetClass() != SummaryWidgetClass)
    {
        SummaryWidgetComponent->SetWidgetClass(SummaryWidgetClass);
    }

    if (!SummaryWidgetComponent->GetUserWidgetObject())
    {
        SummaryWidgetComponent->InitWidget();
    }
}

