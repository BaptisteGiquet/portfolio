

#include "Widgets/MobaAbilitySlotWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "MobaAbilityListViewWidget.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/MobaAbilitySystemStatics.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GAS/Attributes/MobaAttributeSet.h"
#include "GAS/Attributes/MobaAttributeSet_Hero.h"



void UMobaAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	WholeNumberFormattingOptions.MaximumFractionalDigits = 0;
	TwoDigitNumberFormattingOptions.MaximumFractionalDigits = 2;
	
	TextBlock_CooldownCounter->SetVisibility(ESlateVisibility::Hidden);

	CachedOwnerAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn());
	if (CachedOwnerAbilitySystemComponent)
	{
		CachedOwnerAbilitySystemComponent->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::OnAbilityCommitted);
		CachedOwnerAbilitySystemComponent->AbilitySpecDirtiedCallbacks.AddUObject(this, &ThisClass::OnAbilitySpecUpdated);

		CachedOwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetManaAttribute()).AddUObject(this,	&ThisClass::OnManaUpdated);
		CachedOwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet_Hero::GetUpgradePointAttribute()).AddUObject(this,	&ThisClass::OnUpgradePointUpdated);

		InitializeAbilityUpgradePoints();
	}
}



void UMobaAbilitySlotWidget::InitializeAbilityUpgradePoints()
{
	bool bFound = false;
	const float UpgradePoints = CachedOwnerAbilitySystemComponent->GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetUpgradePointAttribute(), bFound);
	if (bFound)
	{
		FOnAttributeChangeData UpgradePointData;
		UpgradePointData.NewValue = UpgradePoints;
		OnUpgradePointUpdated(UpgradePointData);
	}
}



void UMobaAbilitySlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	if (!ListItemObject) { return; }
	
	AbilityCDOForThisSlot = Cast<UGameplayAbility>(ListItemObject);

	const float AbilityCooldownDuration = UMobaAbilitySystemStatics::GetStaticCooldownDurationForAbility(AbilityCDOForThisSlot);
	const float AbilityCost = UMobaAbilitySystemStatics::GetStaticCostForAbility(AbilityCDOForThisSlot);
	
	TextBlock_CooldownDuration->SetText(FText::AsNumber(AbilityCooldownDuration));
	TextBlock_Cost->SetText(FText::AsNumber(AbilityCost));

	OnAbilitySpecUpdated(*GetAbilitySpec());
}



void UMobaAbilitySlotWidget::ChangeAbilityIcon(const FAbilitySlotWidgetData* AbilityWidgetData)
{
	if (!Image_Icon || !AbilityWidgetData) { return; }
	
	Image_Icon->GetDynamicMaterial()->SetTextureParameterValue(IconMaterialParamName, AbilityWidgetData->AbilityIcon.LoadSynchronous());
}



void UMobaAbilitySlotWidget::OnAbilityCommitted(UGameplayAbility* AbilityCommitted)
{
	if (AbilityCommitted->GetClass()->GetDefaultObject() == AbilityCDOForThisSlot)
	{
		float CooldownTimeRemaining = 0.f;
		float CooldownDuration = 0.f;
		AbilityCommitted->GetCooldownTimeRemainingAndDuration(AbilityCommitted->GetCurrentAbilitySpecHandle(), AbilityCommitted->GetCurrentActorInfo(), CooldownTimeRemaining, CooldownDuration);
		StartCooldown(CooldownTimeRemaining, CooldownDuration);
	}
}



void UMobaAbilitySlotWidget::StartCooldown(const float CooldownTimeRemaining, const float CooldownDuration)
{
	if (!GetWorld()) { return; }
	
	TextBlock_CooldownDuration->SetText(FText::AsNumber(CooldownDuration));
	CachedCooldownDuration = CooldownDuration;
	CachedCooldownTimeRemaining = CooldownTimeRemaining;

	TextBlock_CooldownCounter->SetVisibility(ESlateVisibility::Visible);
	
	GetWorld()->GetTimerManager().ClearTimer(AbilityCooldownTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(AbilityCooldownTimerHandle, this, &ThisClass::OnCooldownFinished, CooldownTimeRemaining);

	GetWorld()->GetTimerManager().ClearTimer(AbilityCooldownUpdateTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(AbilityCooldownUpdateTimerHandle, this, &ThisClass::OnUpdateCooldown, CooldownUpdateFrequency, true, 0.f);
}



void UMobaAbilitySlotWidget::OnUpdateCooldown()
{
	CachedCooldownTimeRemaining -= CooldownUpdateFrequency;

	const FNumberFormattingOptions* FormattingOptions = bIsCooldownTwoDigit ? &TwoDigitNumberFormattingOptions : &WholeNumberFormattingOptions;
	
	TextBlock_CooldownCounter->SetText(FText::AsNumber(CachedCooldownTimeRemaining, FormattingOptions));

	const float CooldownPercent = CachedCooldownTimeRemaining / CachedCooldownDuration;
	Image_Icon->GetDynamicMaterial()->SetScalarParameterValue(CooldownPercentParamName, CooldownPercent);
}



void UMobaAbilitySlotWidget::OnCooldownFinished()
{
	if (!GetWorld()) { return; }
	
	CachedCooldownDuration = 0.f;
	CachedCooldownTimeRemaining = 0.f;
	GetWorld()->GetTimerManager().ClearTimer(AbilityCooldownUpdateTimerHandle);
	TextBlock_CooldownCounter->SetVisibility(ESlateVisibility::Hidden);
}



FGameplayAbilitySpec* UMobaAbilitySlotWidget::GetAbilitySpec()
{
	if (!CachedOwnerAbilitySystemComponent || !AbilityCDOForThisSlot) { return nullptr; }
	
	if (!CachedAbilitySpecHandle.IsValid())
	{
		FGameplayAbilitySpec* AbilitySpec = CachedOwnerAbilitySystemComponent->FindAbilitySpecFromClass(AbilityCDOForThisSlot.GetClass());
		CachedAbilitySpecHandle = AbilitySpec->Handle;
		return AbilitySpec;
	}

	return CachedOwnerAbilitySystemComponent->FindAbilitySpecFromHandle(CachedAbilitySpecHandle);
}



void UMobaAbilitySlotWidget::OnAbilitySpecUpdated(const FGameplayAbilitySpec& AbilitySpec)
{
	if (!CachedOwnerAbilitySystemComponent || AbilitySpec.Ability.GetClass() != AbilityCDOForThisSlot.GetClass()) { return; }
	
	bIsAbilityLearned = AbilitySpec.Level > 0;
	Image_AbilityLevelBar->GetDynamicMaterial()->SetScalarParameterValue(AbilityLevelParamName, AbilitySpec.Level);
	UpdateCanCastParam();

	const float NewManaCost = UMobaAbilitySystemStatics::GetManaCostForAbility(AbilitySpec.Ability, *CachedOwnerAbilitySystemComponent, AbilitySpec.Level);
	const float NewCooldownDuration = UMobaAbilitySystemStatics::GetCooldownDurationForAbility(AbilitySpec.Ability, *CachedOwnerAbilitySystemComponent, AbilitySpec.Level);

	TextBlock_Cost->SetText(FText::AsNumber(NewManaCost));
	TextBlock_CooldownDuration->SetText(FText::AsNumber(NewCooldownDuration));
}



void UMobaAbilitySlotWidget::OnManaUpdated(const FOnAttributeChangeData& ManaData)
{
	UpdateCanCastParam();
}


void UMobaAbilitySlotWidget::OnUpgradePointUpdated(const FOnAttributeChangeData& UpgradePointData)
{
	const bool bHasUpgradePoint = UpgradePointData.NewValue > 0;
	const FGameplayAbilitySpec* AbilitySpec = GetAbilitySpec();
	if (AbilitySpec)
	{
		if (UMobaAbilitySystemStatics::IsAbilityAtMaxLevel(*AbilitySpec))
		{
			Image_Icon->GetDynamicMaterial()->SetScalarParameterValue(AbilityLevelParamName, 0);
			return;
		}

		Image_Icon->GetDynamicMaterial()->SetScalarParameterValue(UpgradeAvailableParamName, bHasUpgradePoint ? 1 : 0);
	}
}



void UMobaAbilitySlotWidget::UpdateCanCastParam()
{
	if (!CachedAbilitySpecHandle.IsValid() || !CachedOwnerAbilitySystemComponent) { return; }
	
	bool bCanCast = false;
	const bool HasEnoughResourceToCast = UMobaAbilitySystemStatics::HasEnoughResourceToCastAbility(*GetAbilitySpec(), *CachedOwnerAbilitySystemComponent);
	if (bIsAbilityLearned && HasEnoughResourceToCast)
	{
		bCanCast = true;
	}
	
	Image_Icon->GetDynamicMaterial()->SetScalarParameterValue(CanCastAbilityParamName, bCanCast ? 1 : 0);	
}
