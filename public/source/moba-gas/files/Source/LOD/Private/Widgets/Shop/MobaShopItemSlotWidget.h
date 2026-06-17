
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Interfaces/MobaTreeNodeInterface.h"
#include "Widgets/MobaItemWidget.h"
#include "MobaShopItemSlotWidget.generated.h"


class UListView;
class UMobaShopItemSlotWidget;
class UMobaShopItem;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemPurchasedIssued, const UMobaShopItem*)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemSelected, const UMobaShopItemSlotWidget*)

UCLASS()
class LOD_API UMobaShopItemSlotWidget : public UMobaItemWidget, public IUserObjectListEntry, public IMobaTreeNodeInterface
{
	GENERATED_BODY()

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	// IMobaTreeNodeInterface
	virtual UUserWidget* CreateTreeNodeWidget() const override;
	virtual TArray<const IMobaTreeNodeInterface*> GetParentNodes() const override;
	virtual TArray<const IMobaTreeNodeInterface*> GetChildNodes() const override;
	virtual UObject* GetNodeObject() const override;

	
	TObjectPtr<UMobaShopItem> GetShopItem() const;
	
	FOnItemPurchasedIssued OnItemPurchasedIssued;
	FOnItemSelected OnItemSelected;
	
private:
	void CopyFromSource(const UMobaShopItemSlotWidget* SourceWidget);
	void InitWithShopItem(UMobaShopItem* InShopItem);
	TArray<const IMobaTreeNodeInterface*> ConvertItemsToInterfaces(const TArray<const UMobaShopItem*>& Items) const;
	
	virtual void LeftButtonClicked() override;
	virtual void RightButtonClicked() override;
	
	UPROPERTY()
	TObjectPtr<UMobaShopItem> ShopItem;

	UPROPERTY()
	TObjectPtr<UListView> ParentNodeListView;
};

