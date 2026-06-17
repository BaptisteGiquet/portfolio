#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "EMRWaitingSeatComponent.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EEMRWaitingSeatState : uint8
{
    Free        UMETA(DisplayName = "Free"),
    Reserved    UMETA(DisplayName = "Reserved"),
    Occupied    UMETA(DisplayName = "Occupied")
};

/**
 * Scene component marking a usable waiting seat and tracking its occupancy state.
 */
UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRWaitingSeatComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UEMRWaitingSeatComponent();

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool ReserveSeat(AActor* InReservedFor);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void ReleaseSeat(AActor* ReleasingActor);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void OccupySeat(AActor* InOccupant);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void ClearSeat();

    /** Returns the preferred transform to approach this seat. */
    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    FTransform GetApproachTransform() const;

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool HasApproachPoint() const;

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    EEMRWaitingSeatState GetSeatState() const { return SeatState; }
	
    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool IsFree() const { return SeatState == EEMRWaitingSeatState::Free; }

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool IsReserved() const { return SeatState == EEMRWaitingSeatState::Reserved; }

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    bool IsOccupied() const { return SeatState == EEMRWaitingSeatState::Occupied; }

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    FTransform GetSeatTransform() const { return GetComponentTransform(); }

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    AActor* GetReservedActor() const { return ReservedActor.Get(); }

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    AActor* GetOccupant() const { return CurrentOccupant.Get(); }

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void SetWaitingSeatAnimationTag(FGameplayTag InSeatAnimationTag);

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    void ClearWaitingSeatAnimationTag(FGameplayTag ExpectedSeatAnimationTag);

    UFUNCTION(BlueprintPure, Category = "EMR|WaitingRoom")
    FGameplayTag GetWaitingSeatAnimationTag() const { return SeatAnimationTag; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    EEMRWaitingSeatState SeatState = EEMRWaitingSeatState::Free;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    TWeakObjectPtr<AActor> ReservedActor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    TWeakObjectPtr<AActor> CurrentOccupant;

    /** Offset (in local space) applied from the seat transform when no explicit approach point is provided. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    FVector ApproachOffset = FVector(0.f, 0.f, 0.f);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    FGameplayTag SeatAnimationTag;
};
