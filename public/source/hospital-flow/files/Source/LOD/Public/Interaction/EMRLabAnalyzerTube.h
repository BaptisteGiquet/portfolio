#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Shop/EMRItemActor.h"
#include "Data/EMRLabAnalyzerTestData.h"
#include "EMRLabAnalyzerTube.generated.h"

class AEMRPatient;
class AEMRLabAnalyzerMachine;
class UDecalComponent;
class UDataTable;
class UMaterialInstanceDynamic;
class USceneComponent;

UCLASS()
class LOD_API AEMRLabAnalyzerTube : public AEMRItemActor
{
    GENERATED_BODY()

public:
    AEMRLabAnalyzerTube();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeTube(AEMRPatient* Patient, const FGameplayTagContainer& InRequiredTests);
    void MarkTestsCompleted(const FGameplayTagContainer& TestsToComplete);
    bool AreAllTestsCompleted() const;
    bool CanBeTrashed() const;
    void MarkAbandoned();

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer")
    bool IsBalanceTube() const { return bIsBalanceTube; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer")
    bool IsAbandoned() const { return bIsAbandoned; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer")
    const FGameplayTagContainer& GetRequiredTests() const { return RequiredTests; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer")
    const FGameplayTagContainer& GetCompletedTests() const { return CompletedTests; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer")
    AEMRPatient* GetOwningPatient() const { return OwningPatient.Get(); }

    bool HasTriggeredExamCompletion() const { return bExamCompletionTriggered; }
    void MarkExamCompletionTriggered();

    void GetTestDecalComponents(TArray<UDecalComponent*>& OutDecals) const;
    void UpdateDecalMarkersForComponents(const TArray<UDecalComponent*>& Decals, TArray<TObjectPtr<UMaterialInstanceDynamic>>& InOutMIDs);

    void SetBalanceTube(bool bInBalance);
    void SetInteractionLocked(bool bLocked);
    void SetInsertedInMachine(AEMRLabAnalyzerMachine* Machine);
    void SetStoredInRack(bool bInStored, USceneComponent* AttachAnchor = nullptr);

    virtual bool IsInteractableEnabled_Implementation() const override;

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer")
    bool IsStoredInRack() const { return bStoredInRack; }

protected:
    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Tube")
    FName TestDecalTag = TEXT("LabAnalyzerTestDecal");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Tube")
    bool bSortDecalsByName = true;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Tube")
    TObjectPtr<UDataTable> TestDefinitionsTable = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Tube")
    FName DecalColorParameterName = TEXT("Color");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Tube")
    bool bHideUnusedDecals = true;

    UPROPERTY(ReplicatedUsing = OnRep_TubeState, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    FGameplayTagContainer RequiredTests;

    UPROPERTY(ReplicatedUsing = OnRep_TubeState, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    FGameplayTagContainer CompletedTests;

    UPROPERTY(ReplicatedUsing = OnRep_TubeState, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    bool bIsAbandoned = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    bool bIsBalanceTube = false;

    UPROPERTY(ReplicatedUsing = OnRep_TubeState, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    bool bInteractionLocked = false;

    UPROPERTY(ReplicatedUsing = OnRep_TubeState, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    bool bInsertedInMachine = false;

    UPROPERTY(ReplicatedUsing = OnRep_TubeState, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    bool bStoredInRack = false;

    UPROPERTY(Replicated)
    TWeakObjectPtr<AEMRPatient> OwningPatient;

    UFUNCTION()
    void OnRep_TubeState();

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|LabAnalyzer")
    void HandleTubeStateChanged();

private:
    void BuildTestDefinitionCache();
    void CacheTestDecals();
    int32 GetSortOrderForTag(const FGameplayTag& TestTag) const;
    FLinearColor GetColorForTag(const FGameplayTag& TestTag) const;
    void UpdateDecalMarkers();
    void ApplyInsertionState();

    void BindToPatient(AEMRPatient* Patient);
    void UnbindFromPatient();
    void HandlePatientLeaving(const FGameplayTag CallbackTag, int32 NewCount);
    UFUNCTION()
    void HandlePatientDestroyed(AActor* DestroyedActor);
    void UpdateCarryState();

    TWeakObjectPtr<AEMRLabAnalyzerMachine> CurrentMachine;
    bool bExamCompletionTriggered = false;
    bool bWasCarried = false;
    bool bWasCarriedAny = false;
    FDelegateHandle PatientLeavingHandle;

    UPROPERTY(Transient)
    TMap<FGameplayTag, FEMRLabAnalyzerTestDefinition> CachedTestDefinitions;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> DecalMIDs;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UDecalComponent>> CachedTestDecals;
};
