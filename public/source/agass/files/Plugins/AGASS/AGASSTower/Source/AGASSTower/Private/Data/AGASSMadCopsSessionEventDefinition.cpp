#include "Data/AGASSMadCopsSessionEventDefinition.h"

#include "Actors/AGASSMadCopsSessionEventActor.h"
#include "Components/AGASSSessionEventManagerComponent.h"

void UAGASSMadCopsSessionEventDefinition::ConfigureEventActor_Implementation(
	AAGASSSessionEventActor* EventActor,
	UAGASSSessionEventManagerComponent* EventManager) const
{
	Super::ConfigureEventActor_Implementation(EventActor, EventManager);

	if (AAGASSMadCopsSessionEventActor* const MadCopsEvent = Cast<AAGASSMadCopsSessionEventActor>(EventActor))
	{
		MadCopsEvent->ApplyDefinitionSettings(*this);
	}
}
