#include "Interaction/EMRLabAnalyzerTrash.h"

#include "Interaction/EMRInteractableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LOD/EMRCollisionChannels.h"

AEMRLabAnalyzerTrash::AEMRLabAnalyzerTrash()
{
    PrimaryActorTick.bCanEverTick = false;

    TrashMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrashMesh"));
    SetRootComponent(TrashMesh);
    TrashMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TrashMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    TrashMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    if (InteractableComponent)
    {
        InteractableComponent->SetDisplayName(FText::FromString(TEXT("Lab Analyzer Trash")));
    }
}

FGameplayEventData AEMRLabAnalyzerTrash::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    if (InteractableComponent)
    {
        return InteractableComponent->BuildInteractionEventData(InterActor);
    }

    return FGameplayEventData();
}

FText AEMRLabAnalyzerTrash::GetInteractableDisplayName_Implementation() const
{
    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(GetName());
}

bool AEMRLabAnalyzerTrash::IsInteractableEnabled_Implementation() const
{
    return InteractableComponent ? InteractableComponent->IsEnabled() : true;
}
