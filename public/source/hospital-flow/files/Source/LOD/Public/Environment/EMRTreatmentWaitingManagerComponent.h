#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EMRTreatmentWaitingManagerComponent.generated.h"

class AActor;
class UEMRWaitingSeatComponent;

USTRUCT(BlueprintType)
struct LOD_API FEMRTreatmentWaitingEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|TreatmentWaiting")
    TWeakObjectPtr<AActor> Patient;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|TreatmentWaiting")
    TWeakObjectPtr<UEMRWaitingSeatComponent> Seat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|TreatmentWaiting")
    float ArrivalTime = 0.f;

    bool IsValid() const { return Patient.IsValid(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEMRTreatmentWaitingSeatAssignedSignature, UEMRWaitingSeatComponent*, Seat, AActor*, Patient);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEMRTreatmentWaitingSeatReleasedSignature, UEMRWaitingSeatComponent*, Seat);

/**
 * Manager component responsible for registering waiting seats and tracking patient assignments within a treatment waiting area.
 */
UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRTreatmentWaitingManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRTreatmentWaitingManagerComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    void RegisterSeat(UEMRWaitingSeatComponent* Seat);

    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    void UnregisterSeat(UEMRWaitingSeatComponent* Seat);

    /** Attempts to reserve a free seat for the patient. If none are available, the patient is queued and OutSeat remains null. */
    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    bool RequestSeatForPatient(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat);

    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    bool AssignSpecificSeatToPatient(UEMRWaitingSeatComponent* Seat, AActor* Patient);

    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    void ReleaseSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    void ReleaseSeatForPatient(AActor* Patient);

    UFUNCTION(BlueprintPure, Category = "EMR|TreatmentWaiting")
    UEMRWaitingSeatComponent* GetSeatForPatient(const AActor* Patient) const;

    UFUNCTION(BlueprintPure, Category = "EMR|TreatmentWaiting")
    bool HasFreeSeat() const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|TreatmentWaiting")
    bool bAutoRegisterSeats = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|TreatmentWaiting")
    bool bSearchAreaVolumeForSeats = false;

    UPROPERTY(BlueprintAssignable, Category = "EMR|TreatmentWaiting")
    FEMRTreatmentWaitingSeatAssignedSignature OnSeatAssigned;

    UPROPERTY(BlueprintAssignable, Category = "EMR|TreatmentWaiting")
    FEMRTreatmentWaitingSeatReleasedSignature OnSeatReleased;

protected:
    void AutoRegisterSeats();
    FEMRTreatmentWaitingEntry* FindEntryBySeat(const UEMRWaitingSeatComponent* Seat);
    FEMRTreatmentWaitingEntry* FindEntryByPatient(const AActor* Patient);
    FEMRTreatmentWaitingEntry* FindOldestQueuedEntry();
    bool AssignSeatToQueuedEntry(FEMRTreatmentWaitingEntry& Entry, UEMRWaitingSeatComponent* Seat);
    void NotifyQueuedPatient(AActor* PatientActor, UEMRWaitingSeatComponent* Seat) const;

    void GatherSeatComponentsFromOwner(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    void GatherSeatComponentsFromAttachedActors(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    void GatherSeatComponentsFromAreaVolume(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    bool ValidateSeatApproach(UEMRWaitingSeatComponent* Seat) const;

protected:
    UPROPERTY(VisibleAnywhere, Replicated, Category = "EMR|TreatmentWaiting")
    TArray<TObjectPtr<UEMRWaitingSeatComponent>> RegisteredSeats;

    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WaitingPatients, Category = "EMR|TreatmentWaiting")
    TArray<FEMRTreatmentWaitingEntry> WaitingPatients;

    UFUNCTION()
    void OnRep_WaitingPatients(const TArray<FEMRTreatmentWaitingEntry>& PreviousPatients);
};
