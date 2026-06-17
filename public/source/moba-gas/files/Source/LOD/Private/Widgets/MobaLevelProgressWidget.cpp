

#include "MobaLevelProgressWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GAS/Attributes/MobaAttributeSet_Hero.h"


void UMobaLevelProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	NumberFormattingOptions.SetMaximumFractionalDigits(0);

	APawn* OwnerPawn = GetOwningPlayerPawn();
	if (!OwnerPawn) { return; }
	
	OwnerAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn);
	if (!OwnerAbilitySystemComponent) { return; }

	if (Image_LevelProgress)
	{
		CachedDynamicMaterialInstance = Image_LevelProgress->GetDynamicMaterial();	
	}
	
	UpdateLevelProgress(FOnAttributeChangeData());
	
	OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet_Hero::GetExperienceAttribute()).AddUObject(this, &ThisClass::UpdateLevelProgress);
	OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet_Hero::GetNextLevelExperienceAttribute()).AddUObject(this, &ThisClass::UpdateLevelProgress);
	OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet_Hero::GetPreviousLevelExperienceAttribute()).AddUObject(this, &ThisClass::UpdateLevelProgress);
	OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet_Hero::GetLevelAttribute()).AddUObject(this, &ThisClass::UpdateLevelProgress);
}



void UMobaLevelProgressWidget::UpdateLevelProgress(const FOnAttributeChangeData& ExperienceData)
{
	bool bFound = false;
	const float CurrentExperience = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetExperienceAttribute(), bFound);
	if (!bFound) { return; }

	const float NextLevelExperience = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetNextLevelExperienceAttribute(), bFound);
	if (!bFound) { return; }

	const float PreviousLevelExperience = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetPreviousLevelExperienceAttribute(), bFound);
	if (!bFound) { return; }

	const float CurrentLevel = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetLevelAttribute(), bFound);
	if (!bFound) { return; }
	
	Text_LevelNumber->SetText(FText::AsNumber(CurrentLevel, &NumberFormattingOptions));

	
	const float CurrentLevelProgress = CurrentExperience - PreviousLevelExperience;
	const float CurrentLevelTotalExperience = NextLevelExperience - PreviousLevelExperience;
	const float ProgressPercent = FMath::Clamp(CurrentLevelProgress / CurrentLevelTotalExperience, 0.f, 1.f);
	
	
	if (CachedDynamicMaterialInstance)
	{
		CachedDynamicMaterialInstance->SetScalarParameterValue(PercentMaterialParamName, ProgressPercent);
	}
}
