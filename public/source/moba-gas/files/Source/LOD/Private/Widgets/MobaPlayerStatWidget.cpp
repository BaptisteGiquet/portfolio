

#include "MobaPlayerStatWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GAS/MobaAbilitySystemComponent.h"

void UMobaPlayerStatWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IconTexture)
	{
		Image_Icon->SetBrushFromTexture(IconTexture);
	}
}




void UMobaPlayerStatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	NumberFormatingOptions.MaximumFractionalDigits = 0;
	
	APawn* OwnerPawn = GetOwningPlayerPawn();
	if (!OwnerPawn) { return; }
	
	OwnerAbilitySystemComponent = Cast<UMobaAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn));
	if (!OwnerAbilitySystemComponent) { return; }

	bool bFound;
	const float AttributeValue = OwnerAbilitySystemComponent->GetGameplayAttributeValue(Attribute, bFound);
	if (!bFound) { return; }

	SetAttributeValue(AttributeValue);

	OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UMobaPlayerStatWidget::OnAttributeChanged);
}




void UMobaPlayerStatWidget::OnAttributeChanged(const FOnAttributeChangeData& AttributeData)
{
	SetAttributeValue(AttributeData.NewValue);
}




void UMobaPlayerStatWidget::SetAttributeValue(const float NewValue)
{
	TextBlock_AttributeValue->SetText(FText::AsNumber(NewValue, &NumberFormatingOptions));
}
