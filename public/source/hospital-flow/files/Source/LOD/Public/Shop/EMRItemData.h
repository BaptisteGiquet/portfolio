
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EMRItemData.generated.h"

class UEMRItemData;
class UStaticMesh;
class UGameplayEffect;
class USoundBase;
class AActor;
class UMaterialInterface;

USTRUCT()
struct FItemCollection
{
	GENERATED_BODY()
public:
	FItemCollection();
	FItemCollection(const TArray<const UEMRItemData*>& InItem);
	void AddItem(const UEMRItemData* NewItem, const bool bAddUnique = false);
	bool Contains(const UEMRItemData* Item) const;
	const TArray<const UEMRItemData*>& GetItems() const;
	
private:
	UPROPERTY()
	TArray<const UEMRItemData*> Items;
};


UENUM(BlueprintType)
enum class EShopItemCategory : uint8
{
	PhysicalCare,       // Bandage, Suture, Cast, Splint, Sling
	InjectableMeds,     // IV : Thrombolysis, Antibiotic, Insulin, Adrenaline
	OralMeds,           // Pills : Analgesic, Aspirin, Corticosteroid
	LifeSupport,        // Oxygen, Hydration, Observation
};


UCLASS()
class LOD_API UEMRItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static FPrimaryAssetType GetShopItemAssetType();

    UTexture2D* GetItemIcon() const;
    FText GetItemName() const { return ItemName; }
    FText GetItemDescription() const { return ItemDescription; }
    bool IsItemConsumable() const { return bItemConsumable; }
    TSubclassOf<UGameplayEffect> GetItemConsumedEffect() const { return ItemConsumedEffect; }
    int32 GetItemPrice() const { return ItemPrice; }
    FGameplayTag GetItemType() const { return ItemType; }
    FGameplayTag GetUseItemTag() const { return UseItemTag; }
    TSubclassOf<AActor> GetWorldItemClass() const { return WorldItemClass; }
    UStaticMesh* GetWorldItemMesh() const;
    UStaticMesh* GetFilledOverlayMesh() const;
    UMaterialInterface* GetFilledOverlayMaterial() const;
    USoundBase* GetInteractionSound() const;
    USoundBase* GetUseInteractionSound() const;
    bool ShouldUseFilledOverlayVisual() const { return bUseFilledOverlayVisual; }
    FName GetFilledOverlayParameterName() const { return FilledOverlayParameterName; }
    float GetFilledOverlayAmount() const { return FilledOverlayAmount; }
    FTransform GetFilledOverlayRelativeTransform() const { return FilledOverlayRelativeTransform; }
    FName GetCarrySocketName() const { return CarrySocketName; }
    FVector GetFirstPersonMeshRelativeLocation() const { return FirstPersonMeshRelativeLocation; }
    FRotator GetFirstPersonMeshRelativeRotation() const { return FirstPersonMeshRelativeRotation; }
    FVector GetFirstPersonMeshRelativeScale() const { return FirstPersonMeshRelativeScale; }
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
	TSoftObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
	FText ItemName = FText();

	UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
	FText ItemDescription = FText();

	UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem", meta = (DisplayName = "Consume On Patient Use", ToolTip = "If true, this ItemActor is consumed when used on a patient who can receive treatment."))
	bool bItemConsumable = false;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
    int32 ItemPrice = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem", meta = (EditCondition = "bItemConsumable", EditConditionHides, DisplayName = "Item Consumed Effect (Legacy Prototype)", ToolTip = "Legacy prototype field; current consumption behavior is driven by Consume On Patient Use + ability logic."))
    TSubclassOf<UGameplayEffect> ItemConsumedEffect = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
    FGameplayTag ItemType;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
	FGameplayTag UseItemTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
    TSubclassOf<AActor> WorldItemClass = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem")
    TSoftObjectPtr<UStaticMesh> WorldItemMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Visual")
    bool bUseFilledOverlayVisual = false;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Visual", meta = (EditCondition = "bUseFilledOverlayVisual", EditConditionHides))
    TSoftObjectPtr<UStaticMesh> FilledOverlayMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Visual", meta = (EditCondition = "bUseFilledOverlayVisual", EditConditionHides))
    TSoftObjectPtr<UMaterialInterface> FilledOverlayMaterial = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Visual", meta = (EditCondition = "bUseFilledOverlayVisual", EditConditionHides))
    FName FilledOverlayParameterName = TEXT("FillAmount");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Visual", meta = (EditCondition = "bUseFilledOverlayVisual", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0"))
    float FilledOverlayAmount = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Visual", meta = (EditCondition = "bUseFilledOverlayVisual", EditConditionHides))
    FTransform FilledOverlayRelativeTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Audio")
    TSoftObjectPtr<USoundBase> InteractionSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|ShopItem|Audio")
    TSoftObjectPtr<USoundBase> UseInteractionSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry")
    FName CarrySocketName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry|FirstPerson")
    FVector FirstPersonMeshRelativeLocation = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry|FirstPerson")
    FRotator FirstPersonMeshRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry|FirstPerson")
    FVector FirstPersonMeshRelativeScale = FVector::OneVector;
};
