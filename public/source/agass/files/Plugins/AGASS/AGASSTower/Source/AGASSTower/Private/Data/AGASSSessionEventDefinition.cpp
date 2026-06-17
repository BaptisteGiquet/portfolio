#include "Data/AGASSSessionEventDefinition.h"

bool UAGASSSessionEventDefinition::CanAutoActivate_Implementation(const UAGASSSessionEventManagerComponent* EventManager) const
{
	return true;
}

void UAGASSSessionEventDefinition::ConfigureEventActor_Implementation(
	AAGASSSessionEventActor* EventActor,
	UAGASSSessionEventManagerComponent* EventManager) const
{
}
