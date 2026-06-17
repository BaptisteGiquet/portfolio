#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "EMRWaitingRoomManagerComponent.generated.h"

class AActor;
class UEMRWaitingSeatComponent;
class AEMRPatient;

USTRUCT(BlueprintType)
struct LOD_API FEMRWaitingPatientEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    TWeakObjectPtr<AActor> Patient;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    TWeakObjectPtr<UEMRWaitingSeatComponent> Seat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    float ArrivalTime = 0.f;

    bool IsValid() const { return Patient.IsValid(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEMRWaitingSeatAssignedSignature, UEMRWaitingSeatComponent*, Seat, AActor*, Patient);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEMRWaitingSeatReleasedSignature, UEMRWaitingSeatComponent*, Seat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEMRWaitingPatientCalledSignature, AActor*, Patient);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEMRWaitingPatientsChangedSignature);

/**
 * Manager component responsible for registering waiting seats and tracking patient assignments within a waiting room area.
 */
UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRWaitingRoomManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRWaitingRoomManagerComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void RegisterSeat(UEMRWaitingSeatComponent* Seat);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void UnregisterSeat(UEMRWaitingSeatComponent* Seat);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool AssignSeatToPatient(AActor* Patient);

    /** Attempts to reserve a free seat for the patient. If none are available, the patient is queued and OutSeat remains null. */
    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool RequestSeatForPatient(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool AssignSpecificSeatToPatient(UEMRWaitingSeatComponent* Seat, AActor* Patient);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void ReleaseSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void CallPatient(AActor* Patient);

    /** Removes a patient from the queue, releasing any reserved seat and notifying listeners. */
    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool RemovePatientFromQueue(AActor* Patient);

    UFUNCTION(BlueprintPure, Category = "EMR|WaitingRoom")
    TArray<FEMRWaitingPatientEntry> GetWaitingPatients() const { return WaitingPatients; }

    UFUNCTION(BlueprintPure, Category = "EMR|WaitingRoom")
    UEMRWaitingSeatComponent* GetSeatForPatient(const AActor* Patient) const;

    UFUNCTION(BlueprintPure, Category = "EMR|WaitingRoom")
    TArray<FEMRWaitingPatientEntry> GetNextCallablePatients() const;

    UFUNCTION(BlueprintPure, Category = "EMR|WaitingRoom")
    bool HasFreeSeat() const;

    UFUNCTION(BlueprintPure, Category = "EMR|WaitingRoom")
    UEMRWaitingSeatComponent* FindFirstFreeSeat() const;

    UFUNCTION(BlueprintPure, Category = "EMR|WaitingRoom")
    UEMRWaitingSeatComponent* FindRandomFreeSeat() const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    bool bAutoRegisterSeats = true;

    /** If true, also search for waiting seats within the owning room volume instead of only on the owner actor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    bool bSearchAreaVolumeForSeats = false;

    UPROPERTY(BlueprintAssignable, Category = "EMR|WaitingRoom")
    FEMRWaitingSeatAssignedSignature OnSeatAssigned;

    UPROPERTY(BlueprintAssignable, Category = "EMR|WaitingRoom")
    FEMRWaitingSeatReleasedSignature OnSeatReleased;

    UPROPERTY(BlueprintAssignable, Category = "EMR|WaitingRoom")
    FEMRWaitingPatientCalledSignature OnPatientCalled;

    UPROPERTY(BlueprintAssignable, Category = "EMR|WaitingRoom")
    FEMRWaitingPatientsChangedSignature OnWaitingPatientsChanged;

protected:
    void AutoRegisterSeats();
    FEMRWaitingPatientEntry* FindEntryBySeat(const UEMRWaitingSeatComponent* Seat);
    FEMRWaitingPatientEntry* FindEntryByPatient(const AActor* Patient);
    FEMRWaitingPatientEntry* FindOldestQueuedEntry();
    bool AssignSeatToQueuedEntry(FEMRWaitingPatientEntry& Entry, UEMRWaitingSeatComponent* Seat);
    void TryAssignQueuedPatientsToFreeSeats();
    void NotifyQueuedPatient(AActor* PatientActor, UEMRWaitingSeatComponent* Seat) const;
    void RemoveInvalidEntries();

	
    void GatherSeatComponentsFromOwner(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    void GatherSeatComponentsFromAttachedActors(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    void GatherSeatComponentsFromAreaVolume(TArray<UEMRWaitingSeatComponent*>& OutSeats) const;
    bool ValidateSeatApproach(UEMRWaitingSeatComponent* Seat) const;
protected:
    UPROPERTY(VisibleAnywhere, Replicated, Category = "EMR|WaitingRoom")
    TArray<TObjectPtr<UEMRWaitingSeatComponent>> RegisteredSeats;

    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WaitingPatients, Category = "EMR|WaitingRoom")
    TArray<FEMRWaitingPatientEntry> WaitingPatients;

    UFUNCTION()
    void OnRep_WaitingPatients(const TArray<FEMRWaitingPatientEntry>& PreviousPatients);
};
