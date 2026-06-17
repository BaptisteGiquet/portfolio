#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EMRCarryableComponent.generated.h"

class UPrimitiveComponent;
class USkeletalMeshComponent;
class UMeshComponent;
class AActor;
struct FGameplayEventData;

/**
 * Component that handles carrying an actor in a character's hand.
 */
UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRCarryableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRCarryableComponent();

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    void SetSimulatedComponent(UPrimitiveComponent* InComponent);

    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    void AttachToHand(USkeletalMeshComponent* HandMesh, const FName& SocketName);

    /** Attach the carryable to any provided skeletal mesh socket (not limited to a hand). */
    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    void AttachToMesh(USkeletalMeshComponent* TargetMesh, const FName& SocketName);

    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    void DropAtLocation(const FVector& TargetLocation);

    /** Lock the carryable in place (disable physics/collision) while attached to a non-carrier anchor. */
    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    void SetLockedInPlace(bool bLocked);

    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    bool BuildPlaceObjectEventData(const AActor* InstigatorActor, FGameplayEventData& OutEventData) const;

    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    bool BuildUseObjectEventData(const AActor* InstigatorActor, FGameplayEventData& OutEventData, const AActor* TargetActor = nullptr) const;
	
    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    void SetPlaceObjectEventTag(const FGameplayTag& InPlacementEventTag) { PlaceObjectEventTag = InPlacementEventTag; }

	UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
	void SetUseObjectEventTag(const FGameplayTag& InPlacementEventTag) { UseObjectEventTag = InPlacementEventTag; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Carry")
    bool IsCarried() const { return bIsCarried; }

    /**
     * Find a skeletal mesh component on the given carrier that supports the provided socket.
     * This allows characters with nested skeletal meshes (e.g., MetaHumans) to be used as carriers.
     */
    static USkeletalMeshComponent* FindCarrierMesh(const AActor& CarrierActor, const FName& SocketName);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Carry")
    TObjectPtr<UPrimitiveComponent> SimulatedComponent;

    /** Gameplay event tag to broadcast when this carryable is placed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Carry")
    FGameplayTag PlaceObjectEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Carry")
	FGameplayTag UseObjectEventTag;

    UPROPERTY(ReplicatedUsing = OnRep_Carried, BlueprintReadOnly, Category = "EMR|Carry")
    bool bIsCarried = false;

private:
    void NotifyCarrierLocalItemInteractionSound(bool bPickup) const;

    void ApplyCarryState();
    void UpdateCollisionResponses() const;
    void ApplyLocalCarrierMeshVisibility();
    void RestoreLocalCarrierMeshVisibility();

    UFUNCTION()
    void OnRep_Carried();

    UFUNCTION()
    void OnRep_LockedInPlace();

    UPROPERTY(Replicated)
    TObjectPtr<AActor> CurrentCarrierActor = nullptr;

    FTimerHandle DropLocationTimerHandle;

    UPROPERTY(Replicated)
    FName CurrentCarrierSocketName = NAME_None;

    UPROPERTY(ReplicatedUsing = OnRep_LockedInPlace)
    bool bLockedInPlace = false;

    TArray<TWeakObjectPtr<UMeshComponent>> LocallyHiddenMeshComponents;
    TArray<uint8> LocallyHiddenMeshPreviousHiddenState;
};
