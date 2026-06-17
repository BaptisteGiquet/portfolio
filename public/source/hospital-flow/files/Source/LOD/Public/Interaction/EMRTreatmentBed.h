// AEMRTreatmentBed.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "EMRTreatmentBed.generated.h"

class UEMRPatientUIController;
class UWidgetComponent;
class UArrowComponent;
class USphereComponent;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class AEMRPatient;
class UEMRTreatmentSubsystem;
class AEMRTreatmentBedFolder;
class UAnimMontage;
struct FHitResult;

UCLASS()
class LOD_API AEMRTreatmentBed : public AActor
{
	GENERATED_BODY()

public:
	AEMRTreatmentBed();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Bed state
	UFUNCTION(BlueprintPure, Category = "EMR|Treatment")
	bool IsOccupied() const { return GetCurrentPatient() != nullptr; }

	UFUNCTION(BlueprintPure, Category = "EMR|Treatment")
	AEMRPatient* GetCurrentPatient() const { return CurrentPatient; }

	
	// Bed operations
	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
	void AssignPatient(AEMRPatient* Patient);

	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
	void ReleasePatient();

	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment|Upgrade")
	void SetTreatmentBedEnabledByUpgrade(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "EMR|Treatment|Upgrade")
	int32 GetRequiredUpgradeStackCount() const { return FMath::Max(RequiredUpgradeStackCount, 1); }

	UFUNCTION(BlueprintPure, Category = "EMR|Treatment|Upgrade")
	FGameplayTag GetRequiredUpgradeTag() const { return RequiredUpgradeTag; }
	
	UFUNCTION(BlueprintCallable, Category = "EMR|Machine")
	FVector GetPatientWaitPointLocation() const;

	UFUNCTION(BlueprintCallable, Category = "EMR|Machine")
	FRotator GetPatientWaitPointRotation() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	TObjectPtr<USceneComponent> BedRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	TObjectPtr<UArrowComponent> PatientPositionMarker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	TObjectPtr<UArrowComponent> PatientAttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	TObjectPtr<USphereComponent> PatientAttachTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	TObjectPtr<UStaticMeshComponent> BedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	TObjectPtr<UArrowComponent> PatientFolderAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> TreatmentScreenComponent;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
	TSubclassOf<UUserWidget> TreatmentScreenWidgetClass;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "EMR|Treatment")
	TObjectPtr<AEMRTreatmentBedFolder> PatientFolder = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Treatment")
	TObjectPtr<UAnimMontage> PatientLayOnBedMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Treatment|Patient")
	FName PatientMeshComponentName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Treatment|Patient")
	FName PatientMeshComponentTag = NAME_None;

	UPROPERTY(EditAnywhere, Category = "EMR|Treatment|Upgrade", meta = (Categories = "EMR.RunUpgrade"))
	FGameplayTag RequiredUpgradeTag;

	UPROPERTY(EditAnywhere, Category = "EMR|Treatment|Upgrade", meta = (ClampMin = "1"))
	int32 RequiredUpgradeStackCount = 1;

	
private:
	void ApplyTreatmentBedEnabledState();
	void RegisterWithTreatmentSubsystemIfNeeded();
	void UnregisterFromTreatmentSubsystemIfNeeded();
	void ClearAssignedPatientWithoutFreeNotification();

	void UpdateScreenDisplay();
	void AttachPatientToBed(AEMRPatient* Patient);
	void DetachPatientFromBed(AEMRPatient* Patient);
	void PlayPatientLayMontageFor(AEMRPatient* Patient);
	void StopPatientLayMontageFor(AEMRPatient* Patient);
	void HandleAttachedPatientChanged();
	bool TryPlayAttachedPatientMontage();
	USkeletalMeshComponent* ResolvePatientMeshFor(const AEMRPatient* Patient) const;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPatientLayMontage(AEMRPatient* Patient);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopPatientLayMontage(AEMRPatient* Patient);

	UFUNCTION()
	void HandleAttachTriggerOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_CurrentPatient();

	UFUNCTION()
	void OnRep_AttachedPatient();

	UFUNCTION()
	void OnRep_TreatmentBedEnabledByUpgrade();
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPatient)
	TObjectPtr<AEMRPatient> CurrentPatient;

	UPROPERTY(ReplicatedUsing = OnRep_AttachedPatient)
	TObjectPtr<AEMRPatient> AttachedPatient;

	UPROPERTY()
	TObjectPtr<UEMRTreatmentSubsystem> TreatmentSubsystem;

	UPROPERTY(ReplicatedUsing = OnRep_TreatmentBedEnabledByUpgrade)
	bool bTreatmentBedEnabledByUpgrade = true;

	bool bTreatmentBedRegisteredWithSubsystem = false;

	UPROPERTY()
	TObjectPtr<UEMRPatientUIController> PatientUIController;

	UPROPERTY(Transient)
	TWeakObjectPtr<AEMRPatient> LastAttachedPatient;

	UPROPERTY(Transient)
	mutable TWeakObjectPtr<AEMRPatient> CachedPatientMeshOwner;

	UPROPERTY(Transient)
	mutable TWeakObjectPtr<USkeletalMeshComponent> CachedPatientMesh;

	UPROPERTY(Transient)
	TWeakObjectPtr<AEMRPatient> CachedPatientTransformOwner;

	UPROPERTY(Transient)
	FTransform CachedPatientTransform = FTransform::Identity;

	UPROPERTY(Transient)
	bool bHasCachedPatientTransform = false;

	FTimerHandle AttachedPatientMontageRetryTimer;
	
};
