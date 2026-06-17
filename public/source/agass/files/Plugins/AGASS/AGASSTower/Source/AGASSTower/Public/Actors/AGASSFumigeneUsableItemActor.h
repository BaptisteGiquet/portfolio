#pragma once

#include "CoreMinimal.h"
#include "Actors/AGASSUsableItemActor.h"
#include "AGASSFumigeneUsableItemActor.generated.h"

class UAudioComponent;
class UCurveFloat;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class USoundAttenuation;
class USoundBase;

UENUM(BlueprintType)
enum class EAGASSFumigeneActivationState : uint8
{
	Inactive,
	Igniting,
	Burning,
	Spent
};

UCLASS(Blueprintable)
class AGASSTOWER_API AAGASSFumigeneUsableItemActor : public AAGASSUsableItemActor
{
	GENERATED_BODY()

public:
	AAGASSFumigeneUsableItemActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool TryUse(AController* UsingController) override;

protected:
	virtual bool CanBeClaimedByController(const AController* Controller) const override;
	virtual void HandleFirstPersonCarryPresentationUpdated() override;

private:
	UFUNCTION()
	void OnRep_ActivationState();

	bool IsSignalActive() const;
	bool TryConsumeActiveMadPlaneSignal();
	void RefreshPresentationAssets();
	void ApplyNiagaraColorParameters(UNiagaraComponent* NiagaraComponent) const;
	void ClearCosmeticTimers();
	void ActivateIgnitionEffect();
	void StartIgnitionPhase(bool bFromReplication);
	void StartBurningPhase(bool bFromReplication);
	void FinishActivation();
	void StopAllCosmeticPresentation();
	void RefreshLightIntensityFromServerTime();
	double GetReplicatedServerWorldTimeSeconds() const;
	void HandleIgnitionLeadInElapsed();
	void HandleIgnitionPhaseFinished();
	void HandleBurningPhaseFinished();

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components")
	TObjectPtr<UNiagaraComponent> IgnitionEffectComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components")
	TObjectPtr<UNiagaraComponent> ActiveBurnEffectComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components")
	TObjectPtr<UAudioComponent> IgnitionAudioComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components")
	TObjectPtr<UAudioComponent> ActiveLoopAudioComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components")
	TObjectPtr<UPointLightComponent> BurnLightComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components|FirstPerson")
	TObjectPtr<UNiagaraComponent> FirstPersonIgnitionEffectComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components|FirstPerson")
	TObjectPtr<UNiagaraComponent> FirstPersonActiveBurnEffectComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Fumigene|Components|FirstPerson")
	TObjectPtr<UPointLightComponent> FirstPersonBurnLightComponent;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	TSoftObjectPtr<UNiagaraSystem> IgnitionEffectTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	TSoftObjectPtr<UNiagaraSystem> ActiveBurnEffectTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	TSoftObjectPtr<USoundBase> IgnitionSound;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	TSoftObjectPtr<USoundAttenuation> IgnitionSoundAttenuationOverride;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	TSoftObjectPtr<USoundBase> ActiveLoopSound;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	TSoftObjectPtr<USoundAttenuation> ActiveLoopSoundAttenuationOverride;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	TSoftObjectPtr<UCurveFloat> BurnLightIntensityCurve;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	FLinearColor SparkColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	FLinearColor SmokeColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	FLinearColor BurnLightColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation", meta = (ClampMin = "0.0"))
	float SparkColorIntensityMultiplier = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation", meta = (ClampMin = "0.0"))
	float BurnLightIntensity = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation", meta = (ClampMin = "0.0"))
	float BurnLightAttenuationRadius = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	FName SparkColorParameterName = TEXT("SparkColor");

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	FName SmokeColorParameterName = TEXT("SmokeColor");

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Presentation")
	bool bApplySmokeColorParameter = true;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Audio", meta = (ClampMin = "0.0"))
	float IgnitionFadeInSeconds = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Audio", meta = (ClampMin = "0.0"))
	float IgnitionFadeOutSeconds = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Audio", meta = (ClampMin = "0.0"))
	float ActiveLoopFadeInSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Audio", meta = (ClampMin = "0.0"))
	float ActiveLoopFadeOutSeconds = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Timing", meta = (ClampMin = "0.0"))
	float IgnitionLeadInSeconds = 0.32f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Timing", meta = (ClampMin = "0.0"))
	float PreBurnDurationSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Fumigene|Timing", meta = (ClampMin = "0.0"))
	float BurnDurationSeconds = 6.0f;

	UPROPERTY(ReplicatedUsing = OnRep_ActivationState)
	EAGASSFumigeneActivationState ActivationState = EAGASSFumigeneActivationState::Inactive;

	UPROPERTY(Replicated)
	double ActivationStateStartServerTimeSeconds = 0.0;

	UPROPERTY(Replicated)
	double ActiveWindowEndServerTimeSeconds = 0.0;

	UPROPERTY(Replicated)
	bool bHasBeenActivated = false;

	UPROPERTY(Replicated)
	bool bMadPlaneSignalAccepted = false;

	FTimerHandle IgnitionLeadInTimerHandle;
	FTimerHandle IgnitionPhaseFinishTimerHandle;
	FTimerHandle BurnPhaseFinishTimerHandle;
};
