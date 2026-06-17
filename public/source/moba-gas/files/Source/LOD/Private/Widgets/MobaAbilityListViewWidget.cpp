

#include "MobaAbilityListViewWidget.h"

#include "MobaAbilitySlotWidget.h"
#include "Abilities/GameplayAbility.h"


void UMobaAbilityListViewWidget::ConfigureAbilities(const TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>>& MainAbilities)
{
	OnEntryWidgetGenerated().AddUObject(this, &ThisClass::OnAbilitySlotWidgetCreated);
	
	for (const TPair<EAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair: MainAbilities)
	{
		AddItem(AbilityPair.Value.GetDefaultObject());
	}
}




void UMobaAbilityListViewWidget::OnAbilitySlotWidgetCreated(UUserWidget& Widget)
{
	if (UMobaAbilitySlotWidget* AbilitySlotWidget = Cast<UMobaAbilitySlotWidget>(&Widget))
	{
		AbilitySlotWidget->ChangeAbilityIcon(FindWidgetDataForAbility(AbilitySlotWidget->GetListItem<UGameplayAbility>()->GetClass()));
	}
}




const struct FAbilitySlotWidgetData* UMobaAbilityListViewWidget::FindWidgetDataForAbility(const TSubclassOf<UGameplayAbility> AbilityClass) const
{
	if (!AbilitySlotWidgetDataTable || !AbilityClass) { return nullptr; }

	
	const TArray<FName> DataTableRowNames = AbilitySlotWidgetDataTable->GetRowNames();
	for (const FName DataTableRowName : DataTableRowNames)
	{
		const FAbilitySlotWidgetData* RowWidgetData = AbilitySlotWidgetDataTable->FindRow<FAbilitySlotWidgetData>(DataTableRowName, TEXT("FindWidgetDataForAbility"));
		if (RowWidgetData && RowWidgetData->AbilityClass == AbilityClass)
		{
			return RowWidgetData;
		}
	}

	return nullptr;
}


