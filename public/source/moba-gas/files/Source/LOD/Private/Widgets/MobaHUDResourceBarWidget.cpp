
#include "Widgets/MobaHUDResourceBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UMobaHUDResourceBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (ProgressBar)
	{
		ProgressBar->SetFillColorAndOpacity(BarColor);
		ProgressBar->SetVisibility(bIsProgressBarVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);	
	}
	
	if (ValueText)
	{
		ValueText->SetFont(ValueTextFont);
		ValueText->SetVisibility(bIsValueTextVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);	
	}
}




void UMobaHUDResourceBarWidget::InitializeAndBindToAttributes(UAbilitySystemComponent* InAbilitySystemComponent, const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute)
{
	if (!InAbilitySystemComponent)
	{
		return;
	}
	
	bool bIsValueFound;
	bool bIsMaxValueFound;
	const float AttributeValue = InAbilitySystemComponent->GetGameplayAttributeValue(Attribute, bIsValueFound);
	const float AttributeMaxValue = InAbilitySystemComponent->GetGameplayAttributeValue(MaxAttribute, bIsMaxValueFound);

	if (bIsValueFound && bIsMaxValueFound)
	{
		SetResourceBarValues(AttributeValue, AttributeMaxValue);
		
		// Bind function callback that will be called when linked-attribute value changes
		InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UMobaHUDResourceBarWidget::OnAttributeValueChanged);
		InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxAttribute).AddUObject(this, &UMobaHUDResourceBarWidget::OnAttributeMaxValueChanged);
	}
}




void UMobaHUDResourceBarWidget::SetResourceBarValues(const float InAttributeValue, const float InAttributeMaxValue)
{
	if (!ProgressBar || !ValueText) { return; }

	
	CachedValue = InAttributeValue;
	CachedMaxValue = InAttributeMaxValue;
	
	if (InAttributeMaxValue <= KINDA_SMALL_NUMBER) { return; }
	
	
	const float ClampedValue = FMath::Clamp(InAttributeValue, 0.f, InAttributeMaxValue);
	const float NewPercent = ClampedValue / InAttributeMaxValue;
	ProgressBar->SetPercent(NewPercent);

	// Format values to remove decimals
	const int32 InAttributeValueFormatted = FMath::RoundToInt(InAttributeValue);
	const int32 InAttributeMaxValueFormatted = FMath::RoundToInt(InAttributeMaxValue);
	
	static const FText LabelFormat = INVTEXT("{0}/{1}");
	
	ValueText->SetText(FText::Format(LabelFormat, FText::AsNumber(InAttributeValueFormatted), FText::AsNumber(InAttributeMaxValueFormatted)));
}




void UMobaHUDResourceBarWidget::OnAttributeValueChanged(const FOnAttributeChangeData& AttributeData)
{
	SetResourceBarValues(AttributeData.NewValue, CachedMaxValue);
}




void UMobaHUDResourceBarWidget::OnAttributeMaxValueChanged(const FOnAttributeChangeData& AttributeData)
{
	SetResourceBarValues(CachedValue, AttributeData.NewValue);
}
