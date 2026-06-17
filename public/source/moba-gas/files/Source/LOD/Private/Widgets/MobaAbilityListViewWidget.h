
#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "MobaAbilityListViewWidget.generated.h"


class UGameplayAbility;

UCLASS()
class LOD_API UMobaAbilityListViewWidget : public UListView
{
	GENERATED_BODY()
	
public:
	
	
	void ConfigureAbilities(const TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>>& MainAbilities);

	const struct FAbilitySlotWidgetData* FindWidgetDataForAbility(const TSubclassOf<UGameplayAbility> AbilityClass) const;


private:
	void OnAbilitySlotWidgetCreated(UUserWidget& Widget);
	
	UPROPERTY(EditAnywhere, Category = "Data")
	TObjectPtr<UDataTable> AbilitySlotWidgetDataTable = nullptr;
};
