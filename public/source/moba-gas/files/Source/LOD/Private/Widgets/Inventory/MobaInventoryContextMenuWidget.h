
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MobaInventoryContextMenuWidget.generated.h"


class UButton;

UCLASS()
class LOD_API UMobaInventoryContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnButtonClickedEvent& GetSellButtonClickedEvent() const;
	FOnButtonClickedEvent& GetUseButtonClickedEvent() const;
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_UseButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SellButton;
};
