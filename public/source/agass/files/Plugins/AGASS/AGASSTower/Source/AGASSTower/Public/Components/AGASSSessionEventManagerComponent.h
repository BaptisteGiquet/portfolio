#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Types/AGASSModsTypes.h"
#include "AGASSSessionEventManagerComponent.generated.h"

class AAGASSSessionEventActor;
class UAGASSSessionEventDefinition;
class UAGASSRunStateComponent;

USTRUCT(BlueprintType)
struct AGASSTOWER_API FAGASSActiveSessionEventInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Events")
	FGuid InstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Events")
	FString EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Events")
	FSoftObjectPath DefinitionAssetPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Events")
	TObjectPtr<AAGASSSessionEventActor> EventActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Events")
	float ActivatedServerTimeSeconds = 0.f;
};

DECLARE_MULTICAST_DELEGATE(FAGASSSessionEventsChangedEvent);

UCLASS(ClassGroup = (AGASS), meta = (BlueprintSpawnableComponent))
class AGASSTOWER_API UAGASSSessionEventManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGASSSessionEventManagerComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool InitializeAvailableEventsFromResolvedSelection(const FAGASSResolvedContentSelection& ResolvedSelection, FString& OutFailureMessage);
	bool TryActivateEventById(const FString& EventId, FString& OutFailureMessage);
	bool FinishActiveEventByInstanceId(const FGuid& InstanceId);
	void ShutdownEventRuntime();
	int32 RegisterAndGetActivationOrdinal(const FString& EventId);

	const TArray<FAGASSActiveSessionEventInfo>& GetActiveEvents() const
	{
		return ActiveEvents;
	}

	const TArray<TObjectPtr<UAGASSSessionEventDefinition>>& GetAvailableEventDefinitions() const
	{
		return AvailableEventDefinitions;
	}

	FAGASSSessionEventsChangedEvent& OnActiveEventsChanged()
	{
		return ActiveEventsChangedEvent;
	}

private:
	UFUNCTION()
	void OnRep_ActiveEvents();

	UFUNCTION()
	void HandleActiveEventActorDestroyed(AActor* DestroyedActor);

	float GetCurrentServerWorldTimeSeconds() const;
	const UAGASSRunStateComponent* GetRunStateComponent() const;
	float GetRunStartTimeSeconds() const;
	bool IsRunActive() const;
	bool HasActiveEventInstanceForId(const FString& EventId) const;
	int32 FindActiveEventIndexByInstanceId(const FGuid& InstanceId) const;
	int32 FindAvailableDefinitionIndexByEventId(const FString& EventId) const;
	float BuildAutoActivationDelaySeconds(const UAGASSSessionEventDefinition& Definition) const;
	void EnsureAutoActivationScheduled(const UAGASSSessionEventDefinition& Definition);
	void ScheduleNextAutoActivation(const UAGASSSessionEventDefinition& Definition, bool bIsInitialSchedule);
	void StartEvaluationTimer();
	void StopEvaluationTimer();
	void EvaluateAutoActivation();
	bool ActivateDefinition(UAGASSSessionEventDefinition& Definition, const FSoftObjectPath& DefinitionPath, FString& OutFailureMessage);
	void RemoveActiveEventAtIndex(int32 ActiveEventIndex, bool bDestroyActor);

	UPROPERTY(ReplicatedUsing = OnRep_ActiveEvents)
	TArray<FAGASSActiveSessionEventInfo> ActiveEvents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAGASSSessionEventDefinition>> AvailableEventDefinitions;

	UPROPERTY(Transient)
	TArray<FSoftObjectPath> AvailableDefinitionPaths;

	UPROPERTY(Transient)
	TMap<FString, float> NextAutoActivationTimeByEventId;

	UPROPERTY(Transient)
	TMap<FString, int32> ActivationOrdinalByEventId;

	FTimerHandle AutoActivationTimerHandle;
	FAGASSSessionEventsChangedEvent ActiveEventsChangedEvent;
};
