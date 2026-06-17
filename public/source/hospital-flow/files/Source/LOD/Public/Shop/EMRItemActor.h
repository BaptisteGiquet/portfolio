
#pragma once


#include "CoreMinimal.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRItemActor.generated.h"

class UCapsuleComponent;
class UEMRItemData;
class UEMRInteractableComponent;
class UEMRCarryableComponent;
class USoundBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;

/**
 * World representation of a clinic item that can be picked up / dropped.
 */
UCLASS()
class LOD_API AEMRItemActor : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRItemActor();

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Item")
    void SetItemData(const UEMRItemData* InItemData);

    UFUNCTION(BlueprintCallable, Category = "EMR|Item")
    void SetQuantity(const int32 NewQuantity);

    UFUNCTION(BlueprintCallable, Category = "EMR|Item")
    void SetDurability(const float NewDurability);

    UFUNCTION(BlueprintCallable, Category = "EMR|Item")
    void SetItemOwner(AActor* NewOwner);

    UFUNCTION()
    void OnRep_ItemData();

    UFUNCTION(BlueprintCallable, Category = "EMR|Item")
    bool Pickup(USkeletalMeshComponent* HandMesh);

    UFUNCTION(BlueprintCallable, Category = "EMR|Item")
    void DropAtLocation(const FVector& TargetLocation);

    UFUNCTION(BlueprintPure, Category = "EMR|Item")
    const UEMRItemData* GetItemData() const { return ItemData; }

    UFUNCTION(BlueprintPure, Category = "EMR|Item")
    int32 GetStackQuantity() const { return Quantity; }

    UFUNCTION(BlueprintPure, Category = "EMR|Item")
    float GetDurability() const { return Durability; }

    UFUNCTION(BlueprintPure, Category = "EMR|Item")
    UEMRCarryableComponent* GetCarryableComponent() const { return CarryableComponent; }

	UFUNCTION(BlueprintPure, Category = "EMR|Item")
	FName GetHandSocketName() const;

	UFUNCTION(BlueprintPure, Category = "EMR|Item")
	FName GetCarrySocketName() const { return GetHandSocketName(); }

    void ApplyFilledOverlayToFirstPersonPreview(
        UStaticMeshComponent* PreviewOverlayMeshComponent,
        UMaterialInstanceDynamic*& InOutPreviewOverlayMID,
        UStaticMesh*& InOutCachedPreviewOverlayMesh,
        UMaterialInterface*& InOutCachedPreviewOverlayMaterial) const;

    static void ClearFilledOverlayFirstPersonPreview(
        UStaticMeshComponent* PreviewOverlayMeshComponent,
        UMaterialInstanceDynamic*& InOutPreviewOverlayMID,
        UStaticMesh*& InOutCachedPreviewOverlayMesh,
        UMaterialInterface*& InOutCachedPreviewOverlayMaterial);

    virtual USoundBase* GetPickupInteractionSound() const;
    virtual USoundBase* GetPutDownInteractionSound() const;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    void SetFilledOverlayAmount(float NormalizedFillAmount);

    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_ItemData, BlueprintReadOnly, Category = "EMR|Item", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<const UEMRItemData> ItemData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Carry")
    FTransform HandOffset;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "EMR|Item")
    TObjectPtr<AActor> ItemOwner = nullptr;

    UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly, Category = "EMR|Item", meta = (ClampMin = 1))
    int32 Quantity = 1;
	
    UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly, Category = "EMR|Item", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
    float Durability = 1.0f;

private:
    bool GetFilledOverlayPreviewData(
        UStaticMesh*& OutOverlayMesh,
        UMaterialInterface*& OutOverlayMaterial,
        FTransform& OutOverlayRelativeTransform,
        FName& OutFillParameterName,
        float& OutNormalizedFillAmount) const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "EMR|Item")
	TObjectPtr<UStaticMeshComponent> FilledOverlayMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "EMR|Interaction")
	TObjectPtr<UEMRInteractableComponent> InteractableComponent;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Carry")
    TObjectPtr<UEMRCarryableComponent> CarryableComponent;

    void ApplyClientSafePawnCollision();
    void ApplyItemDataToMesh();
    void ApplyFilledOverlayVisual();
    void ApplyItemDataToActor();

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FilledOverlayMID = nullptr;

    UPROPERTY(Transient)
    float FilledOverlayAmount = 0.0f;
};
