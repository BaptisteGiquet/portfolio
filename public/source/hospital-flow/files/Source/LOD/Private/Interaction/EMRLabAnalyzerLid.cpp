#include "Interaction/EMRLabAnalyzerLid.h"

#include "Components/StaticMeshComponent.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "EMRLabAnalyzerLog.h"
#include "GAS/EMRTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMRLabAnalyzerMachine.h"
#include "LOD/EMRCollisionChannels.h"

AEMRLabAnalyzerLid::AEMRLabAnalyzerLid()
{
    PrimaryActorTick.bCanEverTick = false;

    LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
    SetRootComponent(LidMesh);
    LidMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    LidMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    LidMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    LidMesh->SetMobility(EComponentMobility::Movable);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    if (InteractableComponent)
    {
        InteractableComponent->SetDisplayName(FText::FromString(TEXT("Lab Analyzer Lid")));
        InteractableComponent->SetInteractionEventTag(EMRTags::Machine::LabAnalyzer);
    }
}

FGameplayEventData AEMRLabAnalyzerLid::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    FGameplayEventData EventData = InteractableComponent
    ? InteractableComponent->BuildInteractionEventData(InterActor, EMRTags::Machine::LabAnalyzer)
    : FGameplayEventData();

    if (OwningMachine)
    {
        EventData.EventTag = EMRTags::Machine::LabAnalyzer;
        EventData.Target = OwningMachine;
        EventData.OptionalObject = OwningMachine;
    }

    return EventData;
}

FText AEMRLabAnalyzerLid::GetInteractableDisplayName_Implementation() const
{
    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(GetName());
}

bool AEMRLabAnalyzerLid::IsInteractableEnabled_Implementation() const
{
    if (!InteractableComponent || !InteractableComponent->IsEnabled())
    {
        return false;
    }

    if (!OwningMachine || !OwningMachine->GetCurrentOperator())
    {
        return false;
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    if (World->GetNetMode() == NM_DedicatedServer)
    {
        return true;
    }

    const APlayerController* LocalPlayerController = World->GetFirstPlayerController();
    const APawn* LocalPawn = LocalPlayerController ? LocalPlayerController->GetPawn() : nullptr;
    const AEMRPlayerCharacter* LocalPlayerCharacter = Cast<AEMRPlayerCharacter>(LocalPawn);
    return LocalPlayerCharacter && LocalPlayerCharacter == OwningMachine->GetCurrentOperator();
}

void AEMRLabAnalyzerLid::SetOwningMachine(AEMRLabAnalyzerMachine* InOwningMachine)
{
    OwningMachine = InOwningMachine;
    EMR_LAB_LOG(Log, "Lid %s linked to machine=%s", *GetNameSafe(this), *GetNameSafe(OwningMachine));
}

bool AEMRLabAnalyzerLid::IsLidComponent(const UPrimitiveComponent* Component) const
{
    return Component && LidMesh && Component == LidMesh;
}
