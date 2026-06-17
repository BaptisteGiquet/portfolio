#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EMRPathologyReferencePaperRowItem.generated.h"

UCLASS(BlueprintType)
class LOD_API UEMRPathologyReferencePaperRowItem : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(const FText& InPathologyLabel, const TArray<FText>& InRequirementLabels);

    UFUNCTION(BlueprintPure, Category = "EMR|Reference")
    const FText& GetPathologyLabel() const { return PathologyLabel; }

    UFUNCTION(BlueprintPure, Category = "EMR|Reference")
    const TArray<FText>& GetRequirementLabels() const { return RequirementLabels; }

private:
    UPROPERTY(BlueprintReadOnly, Category = "EMR|Reference", meta = (AllowPrivateAccess = "true"))
    FText PathologyLabel;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Reference", meta = (AllowPrivateAccess = "true"))
    TArray<FText> RequirementLabels;
};

