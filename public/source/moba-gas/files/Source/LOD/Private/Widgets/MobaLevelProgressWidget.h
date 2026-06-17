
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GAS/Attributes/MobaAttributeSet.h"
#include "MobaLevelProgressWidget.generated.h"


class UAbilitySystemComponent;
class UTextBlock;
class UImage;

UCLASS()
class LOD_API UMobaLevelProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override; 

private:
	void UpdateLevelProgress(const FOnAttributeChangeData& ExperienceData);
	
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName PercentMaterialParamName = TEXT("LevelPercent");
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_LevelProgress;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_LevelNumber;

	FNumberFormattingOptions NumberFormattingOptions;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> OwnerAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CachedDynamicMaterialInstance;
};
