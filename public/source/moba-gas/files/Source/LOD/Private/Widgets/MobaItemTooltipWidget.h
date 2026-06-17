
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaItemTooltipWidget.generated.h"


class UMobaShopItem;
class UTextBlock;
class UImage;

UCLASS()
class LOD_API UMobaItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItemToolTip(const UMobaShopItem* Item);
	void SetPrice(const float NewPrice);
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ItemIcon;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemTitle;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemPrice;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemDescription;
};
