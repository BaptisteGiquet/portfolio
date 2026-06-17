#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "EMRPathologyReferenceListEntryWidgetBase.generated.h"

class UEMRPathologyReferencePaperRowItem;
class UTextBlock;

UCLASS(Abstract, BlueprintType)
class LOD_API UEMRPathologyReferenceListEntryWidgetBase : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Pathology = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Requirements = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|Reference", meta = (DisplayName = "On Reference Row Set"))
    void BP_OnReferenceRowSet(UEMRPathologyReferencePaperRowItem* RowItem);
};

UCLASS(BlueprintType)
class LOD_API UEMRPathologyReferenceExamEntryWidget : public UEMRPathologyReferenceListEntryWidgetBase
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class LOD_API UEMRPathologyReferenceTreatmentEntryWidget : public UEMRPathologyReferenceListEntryWidgetBase
{
    GENERATED_BODY()
};

