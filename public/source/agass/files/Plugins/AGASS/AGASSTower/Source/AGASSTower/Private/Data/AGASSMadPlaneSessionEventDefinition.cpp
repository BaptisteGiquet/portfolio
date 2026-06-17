#include "Data/AGASSMadPlaneSessionEventDefinition.h"

#include "Actors/AGASSMadPlaneSessionEventActor.h"
#include "Components/AGASSSessionEventManagerComponent.h"
#include "SessionEvents/AGASSTowerStructureAnalysis.h"

bool UAGASSMadPlaneSessionEventDefinition::CanAutoActivate_Implementation(
	const UAGASSSessionEventManagerComponent* EventManager) const
{
	if (EventManager == nullptr || EventManager->GetWorld() == nullptr)
	{
		return false;
	}

	FAGASSTowerStructureSnapshot Snapshot;
	return ResolveTallestEligibleTower(
		*EventManager->GetWorld(),
		FMath::Max(MinimumTowerHeightMeters, 0.f) * 100.f,
		Snapshot);
}

void UAGASSMadPlaneSessionEventDefinition::ConfigureEventActor_Implementation(
	AAGASSSessionEventActor* EventActor,
	UAGASSSessionEventManagerComponent* EventManager) const
{
	Super::ConfigureEventActor_Implementation(EventActor, EventManager);

	if (AAGASSMadPlaneSessionEventActor* const MadPlaneEvent = Cast<AAGASSMadPlaneSessionEventActor>(EventActor))
	{
		MadPlaneEvent->ApplyDefinitionSettings(*this);
	}
}
