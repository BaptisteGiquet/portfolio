
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaItemWidget.generated.h"


class UMobaShopItem;
class UMobaItemTooltipWidget;
class UImage;

UCLASS()
class LOD_API UMobaItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void SetItemIconTexture(UTexture2D* ItemIconTexture);

	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UImage* GetItemIcon() const { return Image_ItemIcon; }
	
	virtual void RightButtonClicked();
	virtual void LeftButtonClicked();
	
	

	virtual UMobaItemTooltipWidget* SetupItemTooltipWidget(const UMobaShopItem* Item);
	
	
private:
	UPROPERTY(meta = (BindWidget))
	UImage* Image_ItemIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	TSubclassOf<UMobaItemTooltipWidget> ItemTooltipClass;
};
