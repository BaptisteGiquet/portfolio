#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Data/EMRNavigationTargetMapping.h"
#include "EMRNavigationIntentSubsystem.generated.h"

class AEMRBaseMachine;
class AEMRPatient;
class UAbilitySystemComponent;
class UDataTable;

USTRUCT(BlueprintType)
struct FEMRNavigationIntentMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag MessageTag;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AEMRPatient> Patient;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> TargetActor;

    UPROPERTY(BlueprintReadOnly)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FRotator TargetRotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly)
    bool bHasTargetLocation = false;

    UPROPERTY(BlueprintReadOnly)
    bool bHasTargetRotation = false;

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag MachineTypeTag;

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag TriggerTag;
};


USTRUCT()
struct FEMRNavigationTagListener
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag ObservedTag;

    FDelegateHandle DelegateHandle;
};

/**
 * Bridges patient gameplay tag changes to Gameplay Message Subsystem broadcasts for navigation intents.
 */
UCLASS()
class LOD_API UEMRNavigationIntentSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Register a patient so the subsystem can listen to its navigation-related gameplay tags. */
    void RegisterPatient(AEMRPatient* Patient);

    /**
     * Broadcasts a navigation intent directly to the Gameplay Message Subsystem.
     * Optional target actor/location/rotation let callers override where the patient should move.
     */
    void BroadcastNavigationIntent(AEMRPatient* Patient, const FGameplayTag& MessageTag, AActor* TargetActor, const FVector& TargetLocation, const FRotator& TargetRotation, const FGameplayTag& MachineTypeTag = FGameplayTag());

    /** Convenience wrapper to resolve the closest machine matching MachineTypeTag when no explicit target is provided. */
    void BroadcastNavigationIntentToMachine(AEMRPatient* Patient, const FGameplayTag& MessageTag, const FGameplayTag& MachineTypeTag);

    /** Returns the gameplay message tag configured for a given machine type via the navigation intent mapping table. */
    FGameplayTag GetNavigationMessageTagForMachine(const FGameplayTag& MachineTypeTag) const;

	AEMRBaseMachine* FindClosestMachine(AActor* OriginActor, const FGameplayTag& MachineTypeTag) const;
private:
    void LoadSubsystemConfig();
    void OnLoadedSubsystemConfig();
    void BuildNavigationMappingsCache(UDataTable* MappingTable);
    void RegisterTagListeners(AEMRPatient* Patient, UAbilitySystemComponent* ASC);
    void HandlePatientTagChanged(const FGameplayTag Tag, int32 NewCount, AEMRPatient* Patient);
    void PublishNavigationIntent(AEMRPatient* Patient, const FEMRNavigationTargetData& Mapping);

    AActor* ResolveNavigationTarget(AEMRPatient* Patient, const FEMRNavigationTargetData& Mapping) const;

    /** trigger tag -> mapping data */
    TMap<FGameplayTag, FEMRNavigationTargetData> CachedNavigationMappings;

    /** Patient to subscribed tag delegates. */
    TMap<TWeakObjectPtr<AEMRPatient>, TArray<FEMRNavigationTagListener>> PatientTagListeners;
};

