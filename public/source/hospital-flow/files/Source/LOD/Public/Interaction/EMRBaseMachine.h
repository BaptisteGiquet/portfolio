#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRBaseMachine.generated.h"

class UEMRInteractableComponent;
class UArrowComponent;
class AEMRPatient;
class USphereComponent;

UCLASS()
class LOD_API AEMRBaseMachine : public AActor, public IEMRInteractableInterface
{
	GENERATED_BODY()

public:
	AEMRBaseMachine();

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "EMR|Machine")
	FVector GetPatientWaitPointLocation() const;

    UFUNCTION(BlueprintCallable, Category = "EMR|Machine")
    FRotator GetPatientWaitPointRotation() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Machine")
    FGameplayTag GetMachineTypeTag() const { return MachineTypeTag; }

    UFUNCTION(BlueprintPure, Category = "EMR|Machine")
    bool IsOccupied() const { return bOccupied; }

    UFUNCTION(BlueprintPure, Category = "EMR|Machine")
    AEMRPatient* GetAssignedPatient() const { return CurrentPatient; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Machine")
    void SetOccupiedByPatient(AEMRPatient* Patient);

    UFUNCTION(BlueprintCallable, Category = "EMR|Machine")
    void ClearOccupiedPatient(AEMRPatient* Patient);

	
protected:
	virtual void BeginPlay() override;
	
    virtual AEMRPatient* FindNearestPatient() const;

    virtual bool ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Machine", meta = (Categories = "EMR.Machine"))
    FGameplayTag MachineTypeTag; // EMR.Machine.Reception

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Machine")
    TSubclassOf<UUserWidget> MachineWidgetClass;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MachineMesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> DetectionRadius;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEMRInteractableComponent> InteractableComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Machine")
	TObjectPtr<UArrowComponent> PatientWaitPoint;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "EMR|Machine")
	TObjectPtr<AEMRPatient> CurrentPatient;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "EMR|Machine")
	bool bOccupied = false;

protected:
	UFUNCTION()
	void OnPatientAddedToQueue(FGameplayTag ExamTag);

	UFUNCTION()
	void OnExamCompleted(FGameplayTag ExamTag);

	void TryAssignNextPatient();

	void AssignPatient(AEMRPatient* NextPatientInQueue);

	UFUNCTION(BlueprintCallable, Category = "EMR|Machine")
	void ReleasePatient();
};
