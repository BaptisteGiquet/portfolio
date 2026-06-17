#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMRInteractableNameWidget.generated.h"

class UTextBlock;

/** Simple HUD widget that displays the currently focused interactable name. */
UCLASS()
class LOD_API UEMRInteractableNameWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void UpdateInteractableInfo(const FText& InteractableName);

    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void ClearInteractableInfo();

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_InteractableName;
};
