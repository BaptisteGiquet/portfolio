#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffectTypes.h"
#include "Data/EMRTreatmentForPathologyMapping.h"
#include "Data/EMRTreatmentQueueMapping.h"
#include "EMRTreatmentSubsystem.generated.h"

class UGameplayEffect;
class AEMRPatient;
class APlayerController;
class AEMRTreatmentBed;
class UAbilitySystemComponent;
class AActor;
class UEMRDifficultyTuningData;
class UEMRTreatmentWaitingManagerComponent;
class UEMRWaitingSeatComponent;
class UEMRMagicBoxSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatientAddedToTreatmentQueue, FGameplayTag, PathologyTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatientReadyToPay, AEMRPatient*, Patient);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTreatmentResolvedNative, AEMRPatient* /*Patient*/, FGameplayTag /*TreatmentTag*/, bool /*bPatientFullyCured*/);


USTRUCT()
struct FEMRTreatmentQueueEffectHandles
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FGameplayTag, FActiveGameplayEffectHandle> EffectHandles;
};


/**
	Subsystem gérant les traitements des patients
	Responsabilités:
		Assignation des lits
		File d'attente si lits occupés
		Verrouillage des patients pendant traitement
**/
UCLASS()
class LOD_API UEMRTreatmentSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "EMR|Treatment")
	FOnPatientAddedToTreatmentQueue OnPatientAddedToTreatmentQueue;

	UPROPERTY(BlueprintAssignable, Category = "EMR|Treatment")
	FOnPatientReadyToPay OnPatientReadyToPay;

    FOnTreatmentResolvedNative& OnTreatmentResolvedNative() { return TreatmentResolvedNativeDelegate; }
	
    /**
     * Main entry point: adds a patient to the treatment system
     * Called by ExamQueueSubsystem::TransitionPatientToTreatment()
     */
    UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
    void AddPatientToTreatmentQueue(AEMRPatient* Patient);

    UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
    void HandlePatientAbandonment(AEMRPatient* Patient);

   
    UFUNCTION(BlueprintPure, Category = "EMR|Treatment")
    bool HasAvailableBed() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Treatment")
    bool AreAllTreatmentWaitingSeatsFull() const;

    // Returns the number of patients waiting for a bed
    UFUNCTION(BlueprintPure, Category = "EMR|Treatment")
    int32 GetWaitingQueueSize() const;

    bool TryAssignPatientToBedForExam(AEMRPatient* Patient);
    bool IsPatientAssignedToBed(AEMRPatient* Patient) const;
    void PromoteExamWaitingPatientToTreatment(AEMRPatient* Patient);
    bool ResolvePatientAsFullyTreated(AEMRPatient* Patient);
    void HandleMagicBoxFreed();



    //Called when a bed becomes free (patient healed/left)
    UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
    void OnBedFreed(AEMRTreatmentBed* Bed);


    UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
    void RegisterTreatmentBed(AEMRTreatmentBed* Bed);
	
    UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
    void UnregisterTreatmentBed(AEMRTreatmentBed* Bed);

    UFUNCTION(BlueprintCallable, Category = "EMR|Exam")
    void CompleteTreatmentForPatient(AEMRPatient* Patient, FGameplayTag TreatmentCompleted, bool bSuccess);


    FGameplayTagContainer GetTreatmentsFromPatient(AEMRPatient* Patient);

private:
    // ─────────────────────────────────────────────────────────────────────
    // Internal data
    // ─────────────────────────────────────────────────────────────────────

    /** List of all registered beds */
    UPROPERTY()
    TArray<AEMRTreatmentBed*> RegisteredBeds;

    /** Bed → Assigned patient mapping (nullptr = free) */
    UPROPERTY()
    TMap<AEMRTreatmentBed*, AEMRPatient*> BedAssignmentsMap;

    /** Patients waiting for a free bed */
    UPROPERTY()
    TArray<AEMRPatient*> WaitingQueue;

    /** Patient → Treating player mapping */
    UPROPERTY()
    TMap<AEMRPatient*, APlayerController*> LockedPatients;

    /** Timers for auto-unlock after timeout */
	UPROPERTY()
    TMap<AEMRPatient*, FTimerHandle> LockTimers;

    /** Duration before automatic unlock (seconds) */
    UPROPERTY(EditDefaultsOnly, Category = "EMR|Treatment")
    float LockTimeoutDuration = 60.0f;

    /** Handles of applied phase effects */
    UPROPERTY()
    TMap<AEMRPatient*, FActiveGameplayEffectHandle> PatientPhaseEffects;

    UPROPERTY()
    TMap<AEMRPatient*, FEMRTreatmentQueueEffectHandles> PatientQueueEffects;

	
    // ─────────────────────────────────────────────────────────────────────
    // Internal methods
    // ─────────────────────────────────────────────────────────────────────

    //Attempts to assign a bed to the patient@return true if assignment succeeded
	void TryAssignPatientToBed();
    bool TryAssignPatientToMagicBox(AEMRPatient* Patient);
	
	void AssignPatientToBed(AEMRPatient* Patient, AEMRTreatmentBed* Bed);

	void ChangePatientPhaseTag(AEMRPatient* Patient, FGameplayTag NewPhaseTag);

    void ApplyQueueTagToPatient(AEMRPatient* Patient, FGameplayTag TreatmentTag);

    void ApplyQueueTagEffect(AEMRPatient* Patient, FGameplayTag TreatmentTag);

    FGameplayTag GetQueueTagForTreatment(FGameplayTag TreatmentTag);

    void StoreQueueEffectHandle(AEMRPatient* Patient, FGameplayTag TreatmentTag, FActiveGameplayEffectHandle Handle);


    void ApplyPhaseEffect(AEMRPatient* Patient, FGameplayTag PhaseTag);

    float ComputePatientMoneyReward(AEMRPatient* Patient, UAbilitySystemComponent* PatientASC) const;
	float ComputePatientReputationReward(AEMRPatient* Patient, UAbilitySystemComponent* PatientASC) const;

    void ApplyPatientReward(AEMRPatient* Patient, float MoneyRewardAmount, float ReputationRewardAmount);
    void SendPatientToExit(AEMRPatient* Patient);
    AActor* FindClosestHospitalExit(AActor* OriginActor) const;

    void BuildTreatmentQueueMappingsCache() const;

    void BuildTreatmentsFromPathologyMappingsCache() const;

    void LoadSubsystemConfig();

    void OnLoadedSubsystemConfig();

    void AssignTreatmentWaitingSeat(AEMRPatient* Patient);
    void ReleaseTreatmentWaitingSeat(AEMRPatient* Patient);
    UEMRTreatmentWaitingManagerComponent* FindTreatmentWaitingManager() const;
    bool ReleaseReceptionSeatIfPresent(AEMRPatient* Patient) const;

    void LoadDifficultyTuning() const;
    const UEMRDifficultyTuningData* GetDifficultyTuning() const;

    AEMRTreatmentBed* FindBedAssignedToPatient(AEMRPatient* Patient) const;

    mutable TArray<FEMRTreatmentQueueMapping> CachedTreatmentQueueMappings;

    mutable TMap<FGameplayTag, FGameplayTagContainer> CachedTreatmentsByPathology;

    mutable TWeakObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning;

    FOnTreatmentResolvedNative TreatmentResolvedNativeDelegate;
};
