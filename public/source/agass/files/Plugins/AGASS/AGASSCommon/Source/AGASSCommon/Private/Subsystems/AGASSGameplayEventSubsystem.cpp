#include "Subsystems/AGASSGameplayEventSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(const FAGASSGameplayEvent& GameplayEvent)
{
	if (!GameplayEvent.EventName.IsNone())
	{
		GameplayEventReceivedEvent.Broadcast(GameplayEvent);
	}
}

void UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(const FName EventName, const int32 IntValue, const float FloatValue, const bool bFlagValue)
{
	FAGASSGameplayEvent GameplayEvent;
	GameplayEvent.EventName = EventName;
	GameplayEvent.IntValue = IntValue;
	GameplayEvent.FloatValue = FloatValue;
	GameplayEvent.bFlagValue = bFlagValue;

	BroadcastGameplayEvent(GameplayEvent);
}

void UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(UObject* WorldContextObject, const FName EventName, const int32 IntValue, const float FloatValue, const bool bFlagValue)
{
	if (WorldContextObject == nullptr || EventName.IsNone())
	{
		return;
	}

	UWorld* const World = WorldContextObject->GetWorld();
	UGameInstance* const GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	UAGASSGameplayEventSubsystem* const GameplayEventSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UAGASSGameplayEventSubsystem>() : nullptr;
	if (GameplayEventSubsystem == nullptr)
	{
		return;
	}

	GameplayEventSubsystem->BroadcastGameplayEvent(EventName, IntValue, FloatValue, bFlagValue);
}
