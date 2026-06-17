#pragma once

#include "CoreMinimal.h"
#include "Shop/EMRItemActor.h"
#include "Characters/Patient/EMRPatient.h"
#include "Interfaces/EMRAnchoredCarryItemInterface.h"
#include "Interfaces/EMRFirstPersonCarryViewInterface.h"

#include "EMRTreatmentBedFolder.generated.h"

class UWidgetComponent;
class UEMRTriagePatientCard;
class UEMRPatientUIController;
class UEMRFixedPlacementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UUserWidget;

UCLASS()
class LOD_API AEMRTreatmentBedFolder : public AEMRItemActor, public IEMRAnchoredCarryItemInterface, public IEMRFirstPersonCarryViewInterface
{
	GENERATED_BODY()

public:
	AEMRTreatmentBedFolder();
	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetAssignedPatient(AEMRPatient* Patient);
	void ClearAssignedPatient();

	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
	void SetAnchorTransform(const FTransform& InTransform);

	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
	void ReturnToAnchor();

	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
	void SetReturnTraceComponent(UPrimitiveComponent* InComponent);

	bool TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance);
	bool IsCarriedBy(const AActor* Actor) const;
	virtual bool EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance) override;
	virtual void EMRReturnToAnchor_Implementation() override;
	virtual bool EMRIsCarriedBy_Implementation(const AActor* Actor) const override;
	virtual bool IsInteractableEnabled_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	void SyncAnchorTraceToFixedAnchor(bool bForce);
	void UpdateAnchorTraceCollision(bool bEnableBlocking);
	void RefreshPatientCardWorldWidgetVisibility();
	void RefreshPatientCardWidget();
	UEMRTriagePatientCard* ResolvePatientCardWidget();
	void ConfigurePatientCard(UEMRTriagePatientCard* CardWidget) const;
	void SetFolderEnabled(bool bEnabled);

	virtual bool GetFirstPersonCarryWidgetSettings_Implementation(FEMRFirstPersonCarryWidgetSettings& OutSettings) const override;
	virtual void UpdateFirstPersonCarryWidget_Implementation(UUserWidget* Widget) override;
	virtual void ResetFirstPersonCarryWidget_Implementation(UUserWidget* Widget) override;


	UFUNCTION()
	void OnRep_AssignedPatient();

	UPROPERTY(VisibleAnywhere, Category = "EMR|Treatment")
	TObjectPtr<UWidgetComponent> PatientCardWidgetComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Treatment")
	TSubclassOf<UUserWidget> PatientCardWidgetClass;

	UPROPERTY(EditAnywhere, Category = "EMR|Treatment|PatientCard")
	bool bEnableLabAnalyzerWaitingPathologyOverride = true;

	UPROPERTY(EditAnywhere, Category = "EMR|Treatment|PatientCard")
	FText LabAnalyzerWaitingPathologyText;

	UPROPERTY(VisibleAnywhere, Category = "EMR|Treatment")
	TObjectPtr<UEMRFixedPlacementComponent> FixedPlacementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "EMR|Treatment")
	TObjectPtr<USphereComponent> AnchorTraceComponent = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_AssignedPatient)
	TObjectPtr<AEMRPatient> AssignedPatient = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UEMRPatientUIController> PatientCardUIController = nullptr;

	TWeakObjectPtr<UEMRTriagePatientCard> CachedPatientCardWidget;
	TWeakObjectPtr<AEMRPatient> CachedPatientCardPatient;

	UPROPERTY(Transient)
	TObjectPtr<UEMRPatientUIController> FirstPersonPatientCardUIController = nullptr;

	TWeakObjectPtr<AEMRPatient> FirstPersonCachedPatient;
	TWeakObjectPtr<UPrimitiveComponent> ReturnTraceComponent;
	bool bWasCarried = false;

};
