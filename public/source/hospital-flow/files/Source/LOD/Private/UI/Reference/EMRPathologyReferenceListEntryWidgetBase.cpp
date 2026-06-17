#include "UI/Reference/EMRPathologyReferenceListEntryWidgetBase.h"

#include "Components/TextBlock.h"
#include "UI/Reference/EMRPathologyReferencePaperRowItem.h"

namespace
{
    FString BuildRequirementsText(const TArray<FText>& Labels)
    {
        if (Labels.Num() <= 0)
        {
            return FString(TEXT("-"));
        }

        TArray<FString> Parts;
        Parts.Reserve(Labels.Num());
        for (const FText& Label : Labels)
        {
            Parts.Add(Label.ToString());
        }

        return FString::Join(Parts, TEXT(", "));
    }
}

void UEMRPathologyReferenceListEntryWidgetBase::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

    UEMRPathologyReferencePaperRowItem* RowItem = Cast<UEMRPathologyReferencePaperRowItem>(ListItemObject);
    if (!RowItem)
    {
        return;
    }

    if (Text_Pathology)
    {
        Text_Pathology->SetText(RowItem->GetPathologyLabel());
    }

    if (Text_Requirements)
    {
        Text_Requirements->SetText(FText::FromString(BuildRequirementsText(RowItem->GetRequirementLabels())));
    }

    BP_OnReferenceRowSet(RowItem);
}

