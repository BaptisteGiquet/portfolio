#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "EMRHubPlayerSlot.generated.h"

class APlayerState;
class AEMRPlayerState;
class APawn;
class USceneComponent;
class UArrowComponent;

UCLASS()
class LOD_API AEMRHubPlayerSlot : public AActor
{
    GENERATED_BODY()

public:
    AEMRHubPlayerSlot();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    bool AssignPlayerState(APlayerState* NewPlayerState);

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    bool ClearPlayerState();

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    bool IsOccupied() const;

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    APlayerState* GetAssignedPlayerState() const;

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    AEMRPlayerState* GetAssignedEMRPlayerState() const;

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    int32 GetSlotIndex() const;

    UFUNCTION(BlueprintNativeEvent, Category = "Hub|Seating")
    void HandleSeatAssigned(APlayerState* NewPlayerState);
    virtual void HandleSeatAssigned_Implementation(APlayerState* NewPlayerState);

    UFUNCTION(BlueprintNativeEvent, Category = "Hub|Seating")
    void HandleSeatCleared(APlayerState* OldPlayerState);
    virtual void HandleSeatCleared_Implementation(APlayerState* OldPlayerState);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Hub")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Hub")
    TObjectPtr<UArrowComponent> SeatPoint = nullptr;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "EMR|Hub")
    TObjectPtr<USceneComponent> SeatAttachComponent = nullptr;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "EMR|Hub")
    int32 SlotIndex = 0;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "EMR|Hub")
    FGameplayTag SeatAnimationTag;



    UPROPERTY(ReplicatedUsing = OnRep_AssignedPlayerState)
    APlayerState* AssignedPlayerState = nullptr;

    UFUNCTION()
    void OnRep_AssignedPlayerState(APlayerState* PreviousPlayerState);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void SetAssignedPlayerState(APlayerState* NewPlayerState);
    void NotifyAssignmentChanged(APlayerState* PreviousPlayerState, APlayerState* NewPlayerState);
    void TrySeatAssignedPawn();
    void SeatPawn(APawn* Pawn);
    void UnseatPawn(APawn* Pawn);

    UFUNCTION()
    void HandleAssignedPawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn);

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerState> BoundPlayerState;
};
