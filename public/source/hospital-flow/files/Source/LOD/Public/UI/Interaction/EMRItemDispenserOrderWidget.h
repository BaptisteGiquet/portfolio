#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMRItemDispenserOrderWidget.generated.h"

class UTextBlock;

UCLASS()
class LOD_API UEMRItemDispenserOrderWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EMR|Dispenser")
    void SetQuantityText(const FText& InText);

    UFUNCTION(BlueprintCallable, Category = "EMR|Dispenser")
    void SetTotalCostText(const FText& InText);

    UFUNCTION(BlueprintCallable, Category = "EMR|Dispenser")
    void SetTotalCostColor(const FLinearColor& InColor);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TextBlock_OrderQuantity = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TextBlock_OrderTotalCost = nullptr;
};
