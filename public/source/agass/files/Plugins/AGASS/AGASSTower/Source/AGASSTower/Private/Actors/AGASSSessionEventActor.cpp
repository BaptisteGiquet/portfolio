#include "Actors/AGASSSessionEventActor.h"

#include "Components/AGASSSessionEventManagerComponent.h"
#include "Net/UnrealNetwork.h"

AAGASSSessionEventActor::AAGASSSessionEventActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
}

void AAGASSSessionEventActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EventInstanceId);
	DOREPLIFETIME(ThisClass, EventId);
}

void AAGASSSessionEventActor::InitializeEventInstance(
	const FGuid& InInstanceId,
	const FString& InEventId,
	UAGASSSessionEventManagerComponent* InOwningManager)
{
	EventInstanceId = InInstanceId;
	EventId = InEventId;
	OwningEventManager = InOwningManager;
}

void AAGASSSessionEventActor::RequestFinishEvent()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UAGASSSessionEventManagerComponent* const EventManager = OwningEventManager.Get())
	{
		EventManager->FinishActiveEventByInstanceId(EventInstanceId);
		return;
	}

	HandleEventEnded();
	Destroy();
}

void AAGASSSessionEventActor::HandleEventActivated_Implementation()
{
}

void AAGASSSessionEventActor::HandleEventEnded_Implementation()
{
}
