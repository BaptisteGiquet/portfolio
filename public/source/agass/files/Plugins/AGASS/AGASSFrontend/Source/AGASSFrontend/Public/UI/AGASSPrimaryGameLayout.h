#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "AGASSPrimaryGameLayout.generated.h"

class UCommonActivatableWidget;

UCLASS(Abstract)
class AGASSFRONTEND_API UAGASSPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UAGASSPrimaryGameLayout(const FObjectInitializer& ObjectInitializer);

	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(FGameplayTag LayerName, TSubclassOf<UCommonActivatableWidget> ActivatableWidgetClass)
	{
		if (UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerName))
		{
			return Layer->AddWidget<ActivatableWidgetT>(ActivatableWidgetClass);
		}

		return nullptr;
	}

	UCommonActivatableWidgetContainerBase* GetLayerWidget(FGameplayTag LayerName) const;
	UCommonActivatableWidget* GetActiveWidgetInLayer(FGameplayTag LayerName) const;
	UCommonActivatableWidget* GetLayerRootWidget(FGameplayTag LayerName) const;
	void ResetLayerToRoot(FGameplayTag LayerName);
	void ClearLayer(FGameplayTag LayerName);
	void FindAndRemoveWidgetFromLayers(UCommonActivatableWidget* ActivatableWidget);

protected:
	void RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget);
	void ResetRegisteredLayers();

private:
	void OnWidgetStackTransitioning(UCommonActivatableWidgetContainerBase* Widget, bool bIsTransitioning);

	UPROPERTY(Transient, meta = (Categories = "UI.Layer"))
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Layers;
};
