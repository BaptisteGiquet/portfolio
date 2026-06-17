#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "Interfaces/EMRSeatedAnimationInterface.h"
#include "GameplayEffectTypes.h"

#include "EMRPatient.generated.h"


class UEMRPatientAttributeSet;
class UEMRAbilitySystemComponent;
class UEMRInteractableComponent;
class UEMRPatientData;
class UEMRDifficultyTuningData;
class UStaticMesh;
class UStaticMeshComponent;
class AEMRBaseMachine;

UENUM()
enum class EEMRPatienceDrainSuppressor : uint8
{
    Legacy = 0,
    MagicBox,
    OxygenTreatment,
    IntravenousTreatment,
};

UCLASS()
class AEMRPatient : public ACharacter, public IAbilitySystemInterface, public IEMRInteractableInterface, public IEMRSeatedAnimationInterface
{
	GENERATED_BODY()

public:
    AEMRPatient();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // IEMRInteractableInterface
    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Movement")
    void SetIsSeated(bool bInIsSeated);

    UFUNCTION(BlueprintPure, Category = "EMR|Movement")
    bool IsSeated() const { return bIsSeated; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Movement")
    void SetSeatedAnimationState(bool bInIsSeated, const FGameplayTag& InSeatedAnimationTag);

    UFUNCTION(BlueprintCallable, Category = "EMR|Movement")
    void ClearSeatedAnimationState();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EMR|Movement")
    void SetSeatedAnimationTag(const FGameplayTag& InTag);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EMR|Movement")
    FGameplayTag GetSeatedAnimationTag() const;

    virtual void SetSeatedAnimationTag_Implementation(const FGameplayTag& InTag) override;
    virtual FGameplayTag GetSeatedAnimationTag_Implementation() const override;

    void InitializeFromPatientData(const UEMRPatientData* InPatientData);

    /** Reset all patient-specific state so the actor can safely return to the pool. */
    void ResetForPooling();

    /** (Server) Applies or refreshes the patience drain effect with the provided overtime multiplier. */
    void RefreshPatienceDrainEffect(float OvertimeMultiplier);
    void SetPatienceDrainSuppressed(bool bSuppressed);
    void SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor Source, bool bSuppressed);
    bool IsPatienceDrainSuppressed() const { return bPatienceDrainSuppressed; }

    /** Prepares skeletal mesh/animation state while the patient is still hidden for staged reveal. */
    void PrimeMeshAndAnimationForSpawn();

    /** Enforces hidden capsule rendering while preserving capsule collision. */
    void EnsureCapsuleRenderingHidden();


    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    const UEMRPatientData* GetPatientData() const { return PatientData; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    FText GetFullName() const { return FullName; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    int32 GetAge() const { return Age; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    FGameplayTag GetBloodType() const { return BloodType; }

	UFUNCTION(BlueprintPure, Category = "EMR|Patient")
	bool CanReceiveTreatment() const { return bCanReceiveTreatment; }

	UFUNCTION(BlueprintCallable, Category = "EMR|Patient")
	void SetCanReceiveTreatment(bool CanReceiveTreatment) { bCanReceiveTreatment = CanReceiveTreatment; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient|UI")
    bool IsFolderDisplayRegistered() const { return bFolderDisplayRegistered; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Patient|UI")
    void SetFolderDisplayRegistered(bool bInRegistered);

    UFUNCTION(BlueprintCallable, Category = "EMR|Ultrasound")
    UStaticMeshComponent* GetOrCreateUltrasoundSkinProxyComponent();

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound")
    UStaticMeshComponent* GetUltrasoundSkinProxyComponent() const { return UltrasoundSkinProxyComponent; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient|Routing")
    AEMRBaseMachine* GetCurrentAssignedMachine() const { return CurrentAssignedMachine; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient|Routing")
    AEMRBaseMachine* GetNextMachineOnList() const { return NextMachineOnList; }

    void SetCurrentAssignedMachine(AEMRBaseMachine* InMachine);
    void SetNextMachineOnList(AEMRBaseMachine* InMachine);

    float GetCachedMoneyReward() const { return CachedMoneyReward; }
    float GetCachedReputationReward() const { return CachedReputationReward; }
    bool HasCachedRewards() const { return bHasCachedRewards; }



protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Abilities")
	TObjectPtr<UEMRAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Abilities")
	TObjectPtr<UEMRPatientAttributeSet> AttributeSet = nullptr ;

private:
    bool ShouldShowUltrasoundSkinProxy() const;
    void ApplyUltrasoundSkinProxyVisibilityPolicy();

    void ApplyPatientAttributes();
    void GrantStartupAbilities();
    void GrantStartupEffects();
    void ApplyPathologyTags();
    void LoadPatientDataFromID();
    void GenerateIdentityFromData();
    void CacheRewardValues();

    void BuildAttributeToTagMap();
    const UEMRDifficultyTuningData* GetDifficultyTuning() const;
    float GetInitialPatienceForSeverity() const;
    FGameplayTag GetAttributeTag(const FGameplayAttribute& Attribute) const;

    void RegisterPatienceChangeDelegate();
    void UnregisterPatienceChangeDelegate();
    void HandlePatienceChanged(const FOnAttributeChangeData& ChangeData);

	
	UFUNCTION()
	void OnRep_PatientDataID();

    UFUNCTION()
    void OnRep_IsSeated();

    UFUNCTION()
    void OnRep_SeatedAnimationTag();


    UPROPERTY(VisibleAnywhere, Category = "EMR|Patient")
    TObjectPtr<const UEMRPatientData> PatientData;

    UPROPERTY(VisibleAnywhere, Replicated, Category = "EMR|Identity")
    FText FullName;

    UPROPERTY(VisibleAnywhere, Replicated, Category = "EMR|Identity")
    int32 Age = 0;

    UPROPERTY(VisibleAnywhere, Replicated, Category = "EMR|Identity")
    FGameplayTag BloodType;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Interaction")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "EMR|Patient|Routing", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<AEMRBaseMachine> CurrentAssignedMachine = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "EMR|Patient|Routing", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<AEMRBaseMachine> NextMachineOnList = nullptr;

	UPROPERTY(Replicated)
	bool bCanReceiveTreatment = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "EMR|Patient|UI", meta = (AllowPrivateAccess = "true"))
    bool bFolderDisplayRegistered = false;

    UPROPERTY(ReplicatedUsing = OnRep_IsSeated, VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Movement", meta = (AllowPrivateAccess = "true"))
    bool bIsSeated = false;

    UPROPERTY(ReplicatedUsing = OnRep_SeatedAnimationTag, VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Movement", meta = (AllowPrivateAccess = "true"))
    FGameplayTag SeatedAnimationTag;

    UPROPERTY(EditAnywhere, Category = "EMR|Ultrasound")
    TObjectPtr<UStaticMesh> UltrasoundSkinProxyMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound")
    FName UltrasoundSkinProxyParentMeshTag = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound")
    FName UltrasoundSkinProxyParentMeshName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "EMR|Ultrasound")
    FTransform UltrasoundSkinProxyOffset = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Debug")
    bool bShowUltrasoundSkinProxy = false;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> UltrasoundSkinProxyComponent = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_PatientDataID)
	FPrimaryAssetId PatientDataID;
	

    TMap<FGameplayAttribute, FGameplayTag> CachedAttributeToTagMap;

    FActiveGameplayEffectHandle ActivePatienceDrainEffectHandle;
    bool bPatienceDrainSuppressed = false;
    uint8 PatienceDrainSuppressionSourceMask = 0;
    float CachedPatienceDrainMultiplier = 1.0f;

    FDelegateHandle PatienceChangeDelegateHandle;

    bool bHasCachedRewards = false;
    float CachedMoneyReward = 0.0f;
    float CachedReputationReward = 0.0f;

    mutable TWeakObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning;
};
