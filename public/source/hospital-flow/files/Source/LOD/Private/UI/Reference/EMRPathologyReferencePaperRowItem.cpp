#include "UI/Reference/EMRPathologyReferencePaperRowItem.h"

void UEMRPathologyReferencePaperRowItem::Initialize(
    const FText& InPathologyLabel,
    const TArray<FText>& InRequirementLabels)
{
    PathologyLabel = InPathologyLabel;
    RequirementLabels = InRequirementLabels;
}

