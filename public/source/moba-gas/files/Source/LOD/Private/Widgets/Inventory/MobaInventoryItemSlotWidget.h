
#pragma once

#include "CoreMinimal.h"
#include "Inventory/MobaInventoryItem.h"
#include "Widgets/MobaItemWidget.h"
#include "MobaInventoryItemSlotWidget.generated.h"



class UTextBlock;
class UMobaInventoryItem;
class UMobaInventoryItemSlotWidget;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemDropped, UMobaInventoryItemSlotWidget* /* Destination Widget */, UMobaInventoryItemSlotWidget* /* Source Widget */)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnButtonClick, const FInventoryItemHandle& /* ItemHandle */)

UCLASS()
class LOD_API UMobaInventoryItemSlotWidget : public UMobaItemWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	virtual void LeftButtonClicked() override;
	virtual void RightButtonClicked() override;

	FOnInventoryItemDropped OnInventoryItemDropped;
	FOnButtonClick OnLeftButtonClicked;
	FOnButtonClick OnRightButtonClicked;
	
	void UpdateInventoryItem(const UMobaInventoryItem* NewItem);
	
	bool IsSlotEmpty() const;
	void EmptySlot();

	void SetSlotNumber(const int32 NewSlotNumber);
	int32 GetSlotNumber() const { return SlotNumber; }

	UTexture2D* GetIconTexture() const;
	void UpdateStackCount();

	const UMobaInventoryItem* GetInventoryItem() const { return InventoryItem; }
	FInventoryItemHandle GetInventoryItemHandle() const;

private:
	UPROPERTY()
	TObjectPtr<UMobaInventoryItem> InventoryItem = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ManaCost = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CooldownCounter = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CooldownDuration = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UTexture2D> EmptyTexture = nullptr;

	int32 SlotNumber = 0;

	/************************************************/
	/*                   DRAG & DROP                */
	/************************************************/

private:
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditDefaultsOnly, Category = "DragDrop")
	TSubclassOf<UDragDropOperation> DragDropOperationClass;

	/************************************************/
	/*                       GAS                    */
	/************************************************/

public:
	void StartCooldown(float TimeRemaining, float CooldownDuration);
	
	virtual void SetItemIconTexture(UTexture2D* ItemIconTexture) override;
	
private:
	void UpdateCanCastDisplay(const bool bCanCast);

	void BindCanCastGrantedAbilityDelegate();
	void UnbindCanCastGrantedAbilityDelegate();
	
	void CooldownFinished();
	void UpdateCooldown();
	void ClearCooldown();

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FName IconMaterialParamName = TEXT("Icon");

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FName CooldownPercentParamName = TEXT("CooldownPercent");
	
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FName CanCastAbilityParamName = TEXT("CanCastAbility");

	
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	float CooldownUpdateFrequency = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Cooldown")
	bool bIsCooldownTwoDigit = true;
	
	FTimerHandle CooldownDurationTimerHandle;
	FTimerHandle CooldownUpdateTimerHandle;
	
	float CachedCooldownTimeRemaining = 0.f;
	float CachedCooldownDuration = 0.f;


	FNumberFormattingOptions WholeNumberFormattingOptions;
	FNumberFormattingOptions TwoDigitNumberFormattingOptions;
};
