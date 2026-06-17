
#include "Widgets/MobaOverheadResourceBarWidget.h"

#include "MobaHUDResourceBarWidget.h"
#include "GAS/Attributes/MobaAttributeSet.h"



void UMobaOverheadResourceBarWidget::BindToAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (!InAbilitySystemComponent || !HealthBar || !ManaBar )
	{
		return;
	}
	
	const FGameplayAttribute& HealthAttribute = UMobaAttributeSet::GetHealthAttribute();
	const FGameplayAttribute& HealthMaxAttribute = UMobaAttributeSet::GetMaxHealthAttribute();
	HealthBar->InitializeAndBindToAttributes(InAbilitySystemComponent, HealthAttribute, HealthMaxAttribute);
	
	const FGameplayAttribute& ManaAttribute = UMobaAttributeSet::GetManaAttribute();
	const FGameplayAttribute& ManaMaxAttribute = UMobaAttributeSet::GetMaxManaAttribute();
	ManaBar->InitializeAndBindToAttributes(InAbilitySystemComponent, ManaAttribute, ManaMaxAttribute);
}
