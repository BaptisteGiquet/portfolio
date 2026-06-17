#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMRItemDispenserSlotWidget.generated.h"

class UTextBlock;

UCLASS()
class LOD_API UEMRItemDispenserSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EMR|Dispenser")
    void SetProductCodeText(const FText& InText);

    UFUNCTION(BlueprintCallable, Category = "EMR|Dispenser")
    void SetCostText(const FText& InText);

    UFUNCTION(BlueprintCallable, Category = "EMR|Dispenser")
    void SetCostColor(const FLinearColor& InColor);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ProductCodeText = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CostText = nullptr;
};
