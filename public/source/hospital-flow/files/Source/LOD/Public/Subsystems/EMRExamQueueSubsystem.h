#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Data/EMRExamMachineCompletionMapping.h"
#include "Data/EMRExamQueueMapping.h"
#include "EMRExamQueueSubsystem.generated.h"

class AEMRBaseMachine;
class UGameplayEffect;
class AEMRPatient;


USTRUCT()
struct FEMRExamQueueEffectHandles
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<FGameplayTag, FActiveGameplayEffectHandle> EffectHandles;
};

USTRUCT()
struct FEMRExamCompletionEffectHandles
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<FGameplayTag, FActiveGameplayEffectHandle> EffectHandles;
};

USTRUCT()
struct FEMRExamRequirementCacheEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag Pathology;

    UPROPERTY()
    FGameplayTagContainer RequiredExamTags;

    UPROPERTY()
    FGameplayTagContainer RequiredCompletionTags;

    UPROPERTY()
    bool bMatchByHierarchy = false;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatientAddedToExamQueue, FGameplayTag, ExamTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPatientQueuedForExam, AEMRPatient*, Patient, FGameplayTag, ExamTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExamCompleted, FGameplayTag, ExamTag);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnExamCompletedNative, FGameplayTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatientSentBackToReception, AEMRPatient*, Patient);

/**
 * ExamQueue manager for patients
 * Server-only : handle add/remove of patient in Machine queues
 */

UCLASS()
class LOD_API UEMRExamQueueSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable, Category = "EMR|ExamQueue")
    FOnPatientAddedToExamQueue OnPatientAddedToExamQueue;

    UPROPERTY(BlueprintAssignable, Category = "EMR|ExamQueue")
    FOnPatientQueuedForExam OnPatientQueuedForExam;

	UPROPERTY(BlueprintAssignable, Category = "EMR|ExamQueue")
	FOnExamCompleted OnExamCompleted;

    FOnExamCompletedNative& OnExamCompletedNative() { return ExamCompletedNativeDelegate; }
	
    UPROPERTY(BlueprintAssignable, Category = "EMR|ExamQueue")
    FOnPatientSentBackToReception OnPatientSentBackToReception;
	
    UFUNCTION(BlueprintCallable, Category = "EMR|ExamQueue")
    void AddPatientToExamQueue(AEMRPatient* Patient, const TArray<FGameplayTag>& ExamTags);
    UFUNCTION(BlueprintCallable, Category = "EMR|ExamQueue")
    void BypassExamQueueToTreatment(AEMRPatient* Patient);
    void ChangePatientPhaseTag(AEMRPatient* Patient, FGameplayTag NewPhaseTag);

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamQueue")
    AEMRPatient* GetNextPatientFromExamQueue(FGameplayTag ExamTag);

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamQueue")
    AEMRPatient* GetNextPatientForMachine(FGameplayTag MachineTag);


    UFUNCTION(BlueprintPure, Category = "EMR|ExamQueue")
    bool IsQueueEmpty(FGameplayTag ExamTag) const;

    UFUNCTION(BlueprintPure, Category = "EMR|ExamQueue")
    int32 GetQueueSize(FGameplayTag ExamTag) const;

    UFUNCTION(BlueprintCallable, Category = "EMR|Exam")
    void CompleteExamForPatient(AEMRPatient* Patient, FGameplayTag ExamType, bool bSuccess);

    UFUNCTION(BlueprintPure, Category = "EMR|Exam")
    FGameplayTagContainer GetMissingRequiredExamsForPatient(AEMRPatient* Patient) const;

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamQueue")
    void RemovePatientFromAllQueues(AEMRPatient* Patient);

    UFUNCTION(BlueprintPure, Category = "EMR|ExamQueue")
    TArray<AEMRPatient*> GetAllPatientsInExamQueue(FGameplayTag ExamTag) const;

    UFUNCTION(BlueprintPure, Category = "EMR|ExamQueue")
    FGameplayTag GetMachineTagForExam(FGameplayTag ExamTag) const;

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamQueue")
    void AssignExamWaitingSeat(AEMRPatient* Patient, const FGameplayTag& MachineTag);

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamQueue")
    void ReleaseExamWaitingSeat(AEMRPatient* Patient, const FGameplayTag& MachineTag);

    UFUNCTION(BlueprintPure, Category = "EMR|ExamQueue")
    bool AreAllExamWaitingSeatsFull() const;

    bool TryAssignLabAnalyzerWaitingPatientToBed();

private:
    TMap<FGameplayTag, TArray<AEMRPatient*>> ExamQueueMap;

    TArray<FGameplayTag> GetQueuedExamTagsForPatient(AEMRPatient* Patient) const;
    FGameplayTag ResolveDeterministicNextMachineQueuedExam(AEMRPatient* Patient) const;
    void ClearStaleMachineAssignmentsForPatient(AEMRPatient* Patient, const FGameplayTag& PreserveMachineTag) const;
    void DispatchNextQueuedExamIntent(AEMRPatient* Patient, const FGameplayTag& CompletedExamTag, bool bLabOnlyRemaining);
    bool IsMachineTypeCurrentlyAvailable(const FGameplayTag& MachineTypeTag, const AEMRPatient* Patient = nullptr) const;
    AEMRBaseMachine* FindMachineAssignedToPatient(const FGameplayTag& MachineTypeTag, const AEMRPatient* Patient) const;
    bool IsLabAnalyzerExamTag(const FGameplayTag& ExamTag) const;

    void ApplyQueueTagToPatient(AEMRPatient* Patient, FGameplayTag ExamTag);

    void RemoveQueueTagFromPatient(AEMRPatient* Patient, FGameplayTag ExamTag);

    void RemoveExamCompletedTagsFromPatient(AEMRPatient* Patient);

    FGameplayTag GetQueueTagForExam(FGameplayTag ExamTag) const;

    void NotifyMachinesOfType(FGameplayTag ExamTag);

    class UEMRExamWaitingManagerComponent* FindExamWaitingManager(const FGameplayTag& MachineTag) const;
    bool ReleaseReceptionSeatIfPresent(AEMRPatient* Patient) const;

    const FEMRExamQueueMapping* GetExamQueueMapping(const FGameplayTag& ExamTag) const;

    bool HasAnyQueueTags(AEMRPatient* Patient) const;

    bool DoesPatientSatisfyRequiredExams(AEMRPatient* Patient) const;

    void SendPatientBackToReception(AEMRPatient* Patient);
    void ApplyReturnToReceptionTag(AEMRPatient* Patient);
    void RemoveReturnToReceptionTag(AEMRPatient* Patient);

    void TransitionPatientToTreatment(AEMRPatient* Patient);

    UPROPERTY()
    TMap<AEMRPatient*, FEMRExamQueueEffectHandles> PatientQueueEffects;

    UPROPERTY()
    TMap<AEMRPatient*, FEMRExamCompletionEffectHandles> PatientCompletedExamEffects;

    UPROPERTY()
    TMap<AEMRPatient*, FActiveGameplayEffectHandle> PatientReturnToReceptionEffects;

    UPROPERTY()
    TMap<AEMRPatient*, FActiveGameplayEffectHandle> PatientPhaseEffects;

    void ApplyPhaseEffect(AEMRPatient* Patient, FGameplayTag PhaseTag);

    void ApplyQueueTagEffect(AEMRPatient* Patient, FGameplayTag ExamTag);

    void StoreQueueEffectHandle(AEMRPatient* Patient, FGameplayTag ExamTag, FActiveGameplayEffectHandle Handle);

    void RebuildExamQueueMappingsCache() const;

    void BuildExamRequirementMappingsCache() const;
    void BuildExamCompletionMappingsCache() const;

    FGameplayTagContainer GetRequiredExamCompletionTagsForPatient(AEMRPatient* Patient) const;

    FGameplayTagContainer GetRequiredExamTagsForPatient(AEMRPatient* Patient) const;

    FGameplayTagContainer GetCompletedExamTagsOnPatient(AEMRPatient* Patient) const;

    void ApplyCompletedExamTag(AEMRPatient* Patient, const FGameplayTag& ExamTag);

    void StoreCompletedExamEffectHandle(AEMRPatient* Patient, FGameplayTag CompletedTag, FActiveGameplayEffectHandle Handle);

    FGameplayTag ResolveQueuedExamTagForPatient(
        AEMRPatient* Patient,
        const FGameplayTag& ExamType,
        const FGameplayTag& QueueTag,
        const FGameplayTag& PreferredExamTag,
        const FGameplayTag& MatchRootTag);

    FGameplayTag ConvertExamTagToCompletionTag(const FGameplayTag& ExamTag) const;

    FGameplayTag ConvertCompletionTagToExamTag(const FGameplayTag& CompletedTag) const;

    void LoadSubsystemConfig();

    void OnLoadedSubsystemConfig();

    mutable TArray<FEMRExamQueueMapping> CachedExamQueueMappings;

    mutable TArray<FEMRExamMachineCompletionMapping> CachedExamCompletionMappings;

    mutable TMap<FGameplayTag, FGameplayTagContainer> CachedRequiredExamsByPathology;
    mutable TMap<FGameplayTag, FGameplayTagContainer> CachedRequiredCompletionsByPathology;

    FOnExamCompletedNative ExamCompletedNativeDelegate;
};
