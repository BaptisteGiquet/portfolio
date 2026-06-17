#include "Components/AGASSSessionEventManagerComponent.h"

#include "Actors/AGASSSessionEventActor.h"
#include "Components/AGASSRunStateComponent.h"
#include "Data/AGASSSessionEventDefinition.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Settings/AGASSTowerDeveloperSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSSessionEvents, Log, All);

namespace
{
	FString MakeEventIdKey(const FString& EventId)
	{
		FString Key = EventId.TrimStartAndEnd();
		Key.ToLowerInline();
		return Key;
	}

	FString MakeSoftObjectPathKey(const FSoftObjectPath& SoftObjectPath)
	{
		FString Key = SoftObjectPath.ToString().TrimStartAndEnd();
		Key.ToLowerInline();
		return Key;
	}
}

UAGASSSessionEventManagerComponent::UAGASSSessionEventManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAGASSSessionEventManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() != nullptr && GetOwner()->HasAuthority() && !AvailableEventDefinitions.IsEmpty())
	{
		StartEvaluationTimer();
	}
}

void UAGASSSessionEventManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ActiveEvents);
}

bool UAGASSSessionEventManagerComponent::InitializeAvailableEventsFromResolvedSelection(
	const FAGASSResolvedContentSelection& ResolvedSelection,
	FString& OutFailureMessage)
{
	AvailableEventDefinitions.Reset();
	AvailableDefinitionPaths.Reset();
	NextAutoActivationTimeByEventId.Reset();
	ActivationOrdinalByEventId.Reset();
	StopEvaluationTimer();
	OutFailureMessage.Reset();

	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		OutFailureMessage = TEXT("Session event initialization requires server authority.");
		return false;
	}

	const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get();
	if (TowerSettings == nullptr || !TowerSettings->bEnableSessionEvents)
	{
		return true;
	}

	TSet<FString> SeenDefinitionPathKeys;
	TSet<FString> SeenEventIdKeys;

	for (const FSoftObjectPath& DefinitionPath : ResolvedSelection.SessionEventDefinitionAssets)
	{
		if (DefinitionPath.IsNull())
		{
			continue;
		}

		const FString DefinitionPathKey = MakeSoftObjectPathKey(DefinitionPath);
		if (DefinitionPathKey.IsEmpty() || SeenDefinitionPathKeys.Contains(DefinitionPathKey))
		{
			continue;
		}

		SeenDefinitionPathKeys.Add(DefinitionPathKey);

		UAGASSSessionEventDefinition* const EventDefinition = Cast<UAGASSSessionEventDefinition>(DefinitionPath.TryLoad());
		if (EventDefinition == nullptr)
		{
			UE_LOG(
				LogAGASSSessionEvents,
				Warning,
				TEXT("Ignoring unresolved session event definition asset '%s'."),
				*DefinitionPath.ToString());
			continue;
		}

		const FString EventIdKey = MakeEventIdKey(EventDefinition->EventId);
		if (EventIdKey.IsEmpty())
		{
			UE_LOG(
				LogAGASSSessionEvents,
				Warning,
				TEXT("Ignoring session event definition '%s' because it does not declare an EventId."),
				*DefinitionPath.ToString());
			continue;
		}

		if (SeenEventIdKeys.Contains(EventIdKey))
		{
			UE_LOG(
				LogAGASSSessionEvents,
				Warning,
				TEXT("Ignoring session event definition '%s' because EventId '%s' is already provided by another resolved event definition."),
				*DefinitionPath.ToString(),
				*EventDefinition->EventId);
			continue;
		}

		SeenEventIdKeys.Add(EventIdKey);
		AvailableEventDefinitions.Add(EventDefinition);
		AvailableDefinitionPaths.Add(DefinitionPath);
	}

	if (!AvailableEventDefinitions.IsEmpty())
	{
		StartEvaluationTimer();
	}

	return true;
}

bool UAGASSSessionEventManagerComponent::TryActivateEventById(const FString& EventId, FString& OutFailureMessage)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		OutFailureMessage = TEXT("Session events can only be activated on the server.");
		return false;
	}

	if (!IsRunActive())
	{
		OutFailureMessage = TEXT("Session events cannot activate after the run has ended.");
		return false;
	}

	const int32 DefinitionIndex = FindAvailableDefinitionIndexByEventId(EventId);
	if (!AvailableEventDefinitions.IsValidIndex(DefinitionIndex) || !AvailableDefinitionPaths.IsValidIndex(DefinitionIndex))
	{
		OutFailureMessage = FString::Printf(TEXT("No available session event definition matches '%s'."), *EventId);
		return false;
	}

	UAGASSSessionEventDefinition* const EventDefinition = AvailableEventDefinitions[DefinitionIndex];
	if (EventDefinition == nullptr)
	{
		OutFailureMessage = FString::Printf(TEXT("The session event definition for '%s' is no longer loaded."), *EventId);
		return false;
	}

	return ActivateDefinition(*EventDefinition, AvailableDefinitionPaths[DefinitionIndex], OutFailureMessage);
}

int32 UAGASSSessionEventManagerComponent::RegisterAndGetActivationOrdinal(const FString& EventId)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return 0;
	}

	const FString EventIdKey = MakeEventIdKey(EventId);
	if (EventIdKey.IsEmpty())
	{
		return 0;
	}

	int32& Ordinal = ActivationOrdinalByEventId.FindOrAdd(EventIdKey);
	++Ordinal;
	return Ordinal;
}

bool UAGASSSessionEventManagerComponent::FinishActiveEventByInstanceId(const FGuid& InstanceId)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const int32 ActiveEventIndex = FindActiveEventIndexByInstanceId(InstanceId);
	if (!ActiveEvents.IsValidIndex(ActiveEventIndex))
	{
		return false;
	}

	RemoveActiveEventAtIndex(ActiveEventIndex, true);
	return true;
}

void UAGASSSessionEventManagerComponent::ShutdownEventRuntime()
{
	StopEvaluationTimer();

	if (GetOwner() != nullptr && GetOwner()->HasAuthority())
	{
		for (int32 ActiveEventIndex = ActiveEvents.Num() - 1; ActiveEventIndex >= 0; --ActiveEventIndex)
		{
			RemoveActiveEventAtIndex(ActiveEventIndex, true);
		}
	}

	AvailableEventDefinitions.Reset();
	AvailableDefinitionPaths.Reset();
	NextAutoActivationTimeByEventId.Reset();
	ActivationOrdinalByEventId.Reset();
}

void UAGASSSessionEventManagerComponent::OnRep_ActiveEvents()
{
	ActiveEventsChangedEvent.Broadcast();
}

void UAGASSSessionEventManagerComponent::HandleActiveEventActorDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == nullptr || GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (int32 ActiveEventIndex = 0; ActiveEventIndex < ActiveEvents.Num(); ++ActiveEventIndex)
	{
		if (ActiveEvents[ActiveEventIndex].EventActor == DestroyedActor)
		{
			ActiveEvents.RemoveAt(ActiveEventIndex);
			ActiveEventsChangedEvent.Broadcast();
			return;
		}
	}
}

float UAGASSSessionEventManagerComponent::GetCurrentServerWorldTimeSeconds() const
{
	if (const AGameStateBase* const GameState = Cast<AGameStateBase>(GetOwner()))
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
}

const UAGASSRunStateComponent* UAGASSSessionEventManagerComponent::GetRunStateComponent() const
{
	return GetOwner() != nullptr
		? GetOwner()->FindComponentByClass<UAGASSRunStateComponent>()
		: nullptr;
}

float UAGASSSessionEventManagerComponent::GetRunStartTimeSeconds() const
{
	if (const UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent())
	{
		return RunStateComponent->GetRunStartTimeSeconds();
	}

	return 0.f;
}

bool UAGASSSessionEventManagerComponent::IsRunActive() const
{
	const UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent();
	return RunStateComponent != nullptr && RunStateComponent->IsRunActive();
}

bool UAGASSSessionEventManagerComponent::HasActiveEventInstanceForId(const FString& EventId) const
{
	const FString EventIdKey = MakeEventIdKey(EventId);
	return ActiveEvents.ContainsByPredicate([&EventIdKey](const FAGASSActiveSessionEventInfo& ActiveEvent)
	{
		return MakeEventIdKey(ActiveEvent.EventId) == EventIdKey;
	});
}

int32 UAGASSSessionEventManagerComponent::FindActiveEventIndexByInstanceId(const FGuid& InstanceId) const
{
	return ActiveEvents.IndexOfByPredicate([&InstanceId](const FAGASSActiveSessionEventInfo& ActiveEvent)
	{
		return ActiveEvent.InstanceId == InstanceId;
	});
}

int32 UAGASSSessionEventManagerComponent::FindAvailableDefinitionIndexByEventId(const FString& EventId) const
{
	const FString EventIdKey = MakeEventIdKey(EventId);
	return AvailableEventDefinitions.IndexOfByPredicate([&EventIdKey](const UAGASSSessionEventDefinition* Definition)
	{
		return Definition != nullptr && MakeEventIdKey(Definition->EventId) == EventIdKey;
	});
}

float UAGASSSessionEventManagerComponent::BuildAutoActivationDelaySeconds(const UAGASSSessionEventDefinition& Definition) const
{
	const float DelayMinSeconds = FMath::Max(Definition.AutoActivationDelayMinSeconds, 0.f);
	const float DelayMaxSeconds = FMath::Max(Definition.AutoActivationDelayMaxSeconds, DelayMinSeconds);
	return FMath::FRandRange(DelayMinSeconds, DelayMaxSeconds);
}

void UAGASSSessionEventManagerComponent::EnsureAutoActivationScheduled(const UAGASSSessionEventDefinition& Definition)
{
	const FString EventIdKey = MakeEventIdKey(Definition.EventId);
	if (EventIdKey.IsEmpty() || NextAutoActivationTimeByEventId.Contains(EventIdKey))
	{
		return;
	}

	ScheduleNextAutoActivation(Definition, true);
}

void UAGASSSessionEventManagerComponent::ScheduleNextAutoActivation(const UAGASSSessionEventDefinition& Definition, const bool bIsInitialSchedule)
{
	const FString EventIdKey = MakeEventIdKey(Definition.EventId);
	if (EventIdKey.IsEmpty())
	{
		return;
	}

	const float CurrentServerTimeSeconds = GetCurrentServerWorldTimeSeconds();
	const float BaseScheduleTimeSeconds = bIsInitialSchedule
		? GetRunStartTimeSeconds() + FMath::Max(Definition.AutoActivationEarliestRunTimeSeconds, 0.f)
		: CurrentServerTimeSeconds;
	const float ScheduledAutoActivationTimeSeconds = BaseScheduleTimeSeconds + BuildAutoActivationDelaySeconds(Definition);
	NextAutoActivationTimeByEventId.Add(EventIdKey, ScheduledAutoActivationTimeSeconds);
}

void UAGASSSessionEventManagerComponent::StartEvaluationTimer()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || AvailableEventDefinitions.IsEmpty())
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(AutoActivationTimerHandle))
	{
		return;
	}

	const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get();
	if (TowerSettings == nullptr || !TowerSettings->bEnableSessionEvents)
	{
		return;
	}

	const float EvaluationIntervalSeconds = FMath::Max(TowerSettings->SessionEventEvaluationIntervalSeconds, 0.1f);
	World->GetTimerManager().SetTimer(
		AutoActivationTimerHandle,
		this,
		&ThisClass::EvaluateAutoActivation,
		EvaluationIntervalSeconds,
		true);
}

void UAGASSSessionEventManagerComponent::StopEvaluationTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoActivationTimerHandle);
	}
}

void UAGASSSessionEventManagerComponent::EvaluateAutoActivation()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	const UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent();
	if (RunStateComponent == nullptr || RunStateComponent->GetRunPhase() != EAGASSRunPhase::Active)
	{
		StopEvaluationTimer();
		return;
	}

	if (!RunStateComponent->IsRunActive())
	{
		return;
	}

	for (int32 DefinitionIndex = 0; DefinitionIndex < AvailableEventDefinitions.Num(); ++DefinitionIndex)
	{
		UAGASSSessionEventDefinition* const EventDefinition = AvailableEventDefinitions[DefinitionIndex];
		if (EventDefinition == nullptr || !AvailableDefinitionPaths.IsValidIndex(DefinitionIndex))
		{
			continue;
		}

		EnsureAutoActivationScheduled(*EventDefinition);

		const FString EventIdKey = MakeEventIdKey(EventDefinition->EventId);
		const float* const NextAutoActivationTimeSeconds = NextAutoActivationTimeByEventId.Find(EventIdKey);
		if (NextAutoActivationTimeSeconds == nullptr)
		{
			continue;
		}

		if (GetCurrentServerWorldTimeSeconds() < *NextAutoActivationTimeSeconds)
		{
			continue;
		}

		if (!EventDefinition->CanAutoActivate(this))
		{
			continue;
		}

		FString FailureMessage;
		if (!ActivateDefinition(*EventDefinition, AvailableDefinitionPaths[DefinitionIndex], FailureMessage) && !FailureMessage.IsEmpty())
		{
			UE_LOG(
				LogAGASSSessionEvents,
				Verbose,
				TEXT("Automatic session event activation skipped for '%s'. reason=\"%s\""),
				*EventDefinition->EventId,
				*FailureMessage);
		}
	}
}

bool UAGASSSessionEventManagerComponent::ActivateDefinition(
	UAGASSSessionEventDefinition& Definition,
	const FSoftObjectPath& DefinitionPath,
	FString& OutFailureMessage)
{
	if (!IsRunActive())
	{
		OutFailureMessage = TEXT("The run is no longer active.");
		return false;
	}

	const FString EventId = Definition.EventId.TrimStartAndEnd();
	if (EventId.IsEmpty())
	{
		OutFailureMessage = FString::Printf(TEXT("The session event definition '%s' does not declare a valid EventId."), *DefinitionPath.ToString());
		return false;
	}

	if (!Definition.bAllowConcurrentInstances && HasActiveEventInstanceForId(EventId))
	{
		OutFailureMessage = FString::Printf(TEXT("The session event '%s' already has an active instance."), *EventId);
		return false;
	}

	const FString EventIdKey = MakeEventIdKey(EventId);
	const float CurrentServerTimeSeconds = GetCurrentServerWorldTimeSeconds();

	UClass* const EventActorClass = Definition.EventActorClass.LoadSynchronous();
	if (EventActorClass == nullptr)
	{
		OutFailureMessage = FString::Printf(TEXT("The session event '%s' does not provide a valid EventActorClass."), *EventId);
		return false;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		OutFailureMessage = TEXT("The session event manager does not have a valid world.");
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAGASSSessionEventActor* const EventActor = World->SpawnActor<AAGASSSessionEventActor>(
		EventActorClass,
		FTransform::Identity,
		SpawnParameters);
	if (EventActor == nullptr)
	{
		OutFailureMessage = FString::Printf(TEXT("Failed to spawn an active actor for session event '%s'."), *EventId);
		return false;
	}

	const FGuid InstanceId = FGuid::NewGuid();
	EventActor->InitializeEventInstance(InstanceId, EventId, this);
	EventActor->OnDestroyed.AddDynamic(this, &ThisClass::HandleActiveEventActorDestroyed);

	FAGASSActiveSessionEventInfo ActiveEventInfo;
	ActiveEventInfo.InstanceId = InstanceId;
	ActiveEventInfo.EventId = EventId;
	ActiveEventInfo.DefinitionAssetPath = DefinitionPath;
	ActiveEventInfo.EventActor = EventActor;
	ActiveEventInfo.ActivatedServerTimeSeconds = CurrentServerTimeSeconds;
	ActiveEvents.Add(ActiveEventInfo);

	ScheduleNextAutoActivation(Definition, false);
	ActiveEventsChangedEvent.Broadcast();

	Definition.ConfigureEventActor(EventActor, this);
	EventActor->HandleEventActivated();
	OutFailureMessage.Reset();
	return true;
}

void UAGASSSessionEventManagerComponent::RemoveActiveEventAtIndex(const int32 ActiveEventIndex, const bool bDestroyActor)
{
	if (!ActiveEvents.IsValidIndex(ActiveEventIndex))
	{
		return;
	}

	const TObjectPtr<AAGASSSessionEventActor> EventActor = ActiveEvents[ActiveEventIndex].EventActor;
	ActiveEvents.RemoveAt(ActiveEventIndex);

	if (EventActor != nullptr)
	{
		EventActor->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleActiveEventActorDestroyed);

		if (bDestroyActor && IsValid(EventActor))
		{
			EventActor->HandleEventEnded();
			EventActor->Destroy();
		}
	}

	ActiveEventsChangedEvent.Broadcast();
}
