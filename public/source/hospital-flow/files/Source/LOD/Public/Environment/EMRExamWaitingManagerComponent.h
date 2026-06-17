#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EMRExamWaitingManagerComponent.generated.h"

class AActor;
class UEMRWaitingSeatComponent;

USTRUCT(BlueprintType)
struct LOD_API FEMRExamWaitingEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|ExamWaiting")
    TWeakObjectPtr<AActor> Patient;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|ExamWaiting")
    TWeakObjectPtr<UEMRWaitingSeatComponent> Seat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|ExamWaiting")
    float ArrivalTime = 0.f;

    bool IsValid() const { return Patient.IsValid(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEMRExamWaitingSeatAssignedSignature, UEMRWaitingSeatComponent*, Seat, AActor*, Patient);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEMRExamWaitingSeatReleasedSignature, UEMRWaitingSeatComponent*, Seat);

/**
 * Manager component responsible for registering waiting seats and tracking patient assignments within an exam waiting area.
 */
UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRExamWaitingManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRExamWaitingManagerComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    void RegisterSeat(UEMRWaitingSeatComponent* Seat);

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    void UnregisterSeat(UEMRWaitingSeatComponent* Seat);

    /** Attempts to reserve a free seat for the patient. If none are available, the patient is queued and OutSeat remains null. */
    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    bool RequestSeatForPatient(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat);

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    bool AssignSpecificSeatToPatient(UEMRWaitingSeatComponent* Seat, AActor* Patient);

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    void ReleaseSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    void ReleaseSeatForPatient(AActor* Patient);

    UFUNCTION(BlueprintPure, Category = "EMR|ExamWaiting")
    UEMRWaitingSeatComponent* GetSeatForPatient(const AActor* Patient) const;

    UFUNCTION(BlueprintPure, Category = "EMR|ExamWaiting")
    FGameplayTag GetMachineTypeTag() const { return MachineTypeTag; }

    UFUNCTION(BlueprintPure, Category = "EMR|ExamWaiting")
    bool HasFreeSeat() const;

    /** Returns an overflow wait target near this exam waiting area (typically the nearest seat approach). */
    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    bool GetOverflowWaitTarget(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat, FTransform& OutTransform) const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|ExamWaiting")
    bool bAutoRegisterSeats = true;

    /** If set, only patients queued for matching machines will use these seats. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|ExamWaiting", meta = (Categories = "EMR.Machine"))
    FGameplayTag MachineTypeTag;

    /** If true, also search for waiting seats within the owning area volume instead of only on the owner actor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|ExamWaiting")
    bool bSearchAreaVolumeForSeats = false;

    UPROPERTY(BlueprintAssignable, Category = "EMR|ExamWaiting")
    FEMRExamWaitingSeatAssignedSignature OnSeatAssigned;

    UPROPERTY(BlueprintAssignable, Category = "EMR|ExamWaiting")
    FEMRExamWaitingSeatReleasedSignature OnSeatReleased;

protected:
    void AutoRegisterSeats();
    FEMRExamWaitingEntry* FindEntryBySeat(const UEMRWaitingSeatComponent* Seat);
    FEMRExamWaitingEntry* FindEntryByPatient(const AActor* Patient);
    FEMRExamWaitingEntry* FindOldestQueuedEntry();
    bool AssignSeatToQueuedEntry(FEMRExamWaitingEntry& Entry, UEMRWaitingSeatComponent* Seat);
    void NotifyQueuedPatient(AActor* PatientActor, UEMRWaitingSeatComponent* Seat) const;

    void GatherSeatComponentsFromOwner(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    void GatherSeatComponentsFromAttachedActors(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    void GatherSeatComponentsFromAreaVolume(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    bool ValidateSeatApproach(UEMRWaitingSeatComponent* Seat) const;

protected:
    UPROPERTY(VisibleAnywhere, Replicated, Category = "EMR|ExamWaiting")
    TArray<TObjectPtr<UEMRWaitingSeatComponent>> RegisteredSeats;

    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WaitingPatients, Category = "EMR|ExamWaiting")
    TArray<FEMRExamWaitingEntry> WaitingPatients;

    UFUNCTION()
    void OnRep_WaitingPatients(const TArray<FEMRExamWaitingEntry>& PreviousPatients);
};
