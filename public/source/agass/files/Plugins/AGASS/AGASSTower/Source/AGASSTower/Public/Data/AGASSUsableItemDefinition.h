#pragma once

#include "CoreMinimal.h"
#include "Data/AGASSItemDefinition.h"
#include "AGASSUsableItemDefinition.generated.h"

class UAnimMontage;

UCLASS(BlueprintType, Blueprintable, HideCategories = ("AGASS|Placement", "Placement"))
class AGASSTOWER_API UAGASSUsableItemDefinition : public UAGASSItemDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Usable")
	FName CarryAttachSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Usable")
	FTransform CarryAttachRelativeTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Usable")
	FName CarryAnimationId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Usable")
	TSoftObjectPtr<UAnimMontage> UseMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Usable")
	bool bConsumeOnSuccessfulUse = true;
};
