#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AGASSSessionEventDefinition.generated.h"

class AAGASSSessionEventActor;
class UAGASSSessionEventManagerComponent;

UCLASS(BlueprintType, Blueprintable)
class AGASSTOWER_API UAGASSSessionEventDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (DisplayName = "Event Id", ToolTip = "Stable identifier used by the runtime to distinguish this event from other available events."))
	FString EventId;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (DisplayName = "Event Actor Class", ToolTip = "Blueprint class that will be spawned when this event becomes active."))
	TSoftClassPtr<AAGASSSessionEventActor> EventActorClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (ClampMin = "0.0", DisplayName = "Can Start After Run Time (Seconds)", ToolTip = "The event cannot start before the run has been active for at least this many seconds. Example: 100 means the event is blocked for the first 100 seconds of the run."))
	float AutoActivationEarliestRunTimeSeconds = 0.f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (ClampMin = "0.0", DisplayName = "Random Start Delay Min (Seconds)", ToolTip = "Once the minimum run time above has been reached, the manager rolls a random delay between this value and the max value below before starting the event. This same random range is used again when scheduling the next automatic activation after a successful activation."))
	float AutoActivationDelayMinSeconds = 0.f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (ClampMin = "0.0", DisplayName = "Random Start Delay Max (Seconds)", ToolTip = "Upper bound of the random start delay. If this value is lower than the min value, the runtime will clamp it up to the min value."))
	float AutoActivationDelayMaxSeconds = 0.f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (DisplayName = "Allow Multiple Active Instances", ToolTip = "If enabled, several instances of this same event may be active at the same time. If disabled, the runtime waits until the current instance ends before starting another one of the same event."))
	bool bAllowConcurrentInstances = true;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Events")
	bool CanAutoActivate(const UAGASSSessionEventManagerComponent* EventManager) const;
	virtual bool CanAutoActivate_Implementation(const UAGASSSessionEventManagerComponent* EventManager) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Events")
	void ConfigureEventActor(AAGASSSessionEventActor* EventActor, UAGASSSessionEventManagerComponent* EventManager) const;
	virtual void ConfigureEventActor_Implementation(AAGASSSessionEventActor* EventActor, UAGASSSessionEventManagerComponent* EventManager) const;
};
