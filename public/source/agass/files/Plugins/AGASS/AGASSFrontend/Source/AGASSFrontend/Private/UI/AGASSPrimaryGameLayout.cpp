#include "UI/AGASSPrimaryGameLayout.h"

#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSFrontendLayers, Log, All);

UAGASSPrimaryGameLayout::UAGASSPrimaryGameLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UCommonActivatableWidgetContainerBase* UAGASSPrimaryGameLayout::GetLayerWidget(const FGameplayTag LayerName) const
{
	return Layers.FindRef(LayerName);
}

UCommonActivatableWidget* UAGASSPrimaryGameLayout::GetActiveWidgetInLayer(const FGameplayTag LayerName) const
{
	if (const UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerName))
	{
		return Layer->GetActiveWidget();
	}

	return nullptr;
}

UCommonActivatableWidget* UAGASSPrimaryGameLayout::GetLayerRootWidget(const FGameplayTag LayerName) const
{
	if (const UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerName))
	{
		if (const UCommonActivatableWidgetStack* Stack = Cast<UCommonActivatableWidgetStack>(Layer))
		{
			if (UCommonActivatableWidget* RootContent = Stack->GetRootContent())
			{
				return RootContent;
			}
		}

		const TArray<UCommonActivatableWidget*>& WidgetList = Layer->GetWidgetList();
		return WidgetList.IsEmpty() ? nullptr : WidgetList[0];
	}

	return nullptr;
}

void UAGASSPrimaryGameLayout::ResetLayerToRoot(const FGameplayTag LayerName)
{
	if (UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerName))
	{
		if (GetLayerRootWidget(LayerName) == nullptr)
		{
			Layer->ClearWidgets();
			return;
		}

		while (Layer->GetNumWidgets() > 1)
		{
			if (UCommonActivatableWidget* ActiveWidget = Layer->GetActiveWidget())
			{
				ActiveWidget->DeactivateWidget();
			}
			else
			{
				break;
			}
		}
	}
}

void UAGASSPrimaryGameLayout::ClearLayer(const FGameplayTag LayerName)
{
	if (UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerName))
	{
		Layer->ClearWidgets();
	}
}

void UAGASSPrimaryGameLayout::FindAndRemoveWidgetFromLayers(UCommonActivatableWidget* ActivatableWidget)
{
	if (ActivatableWidget == nullptr)
	{
		return;
	}

	for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& LayerPair : Layers)
	{
		if (LayerPair.Value != nullptr)
		{
			LayerPair.Value->RemoveWidget(*ActivatableWidget);
		}
	}
}

void UAGASSPrimaryGameLayout::RegisterLayer(const FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget)
{
	if (LayerWidget == nullptr)
	{
		UE_LOG(LogAGASSFrontendLayers, Warning, TEXT("RegisterLayer failed for %s on %s because LayerWidget is null."),
			*LayerTag.ToString(),
			*GetNameSafe(this));
		return;
	}

	LayerWidget->OnTransitioningChanged.RemoveAll(this);
	LayerWidget->OnTransitioningChanged.AddUObject(this, &ThisClass::OnWidgetStackTransitioning);
	LayerWidget->SetTransitionDuration(0.0f);
	Layers.Add(LayerTag, LayerWidget);

	UE_LOG(LogAGASSFrontendLayers, Log, TEXT("Registered layer %s on %s using widget %s (%s)."),
		*LayerTag.ToString(),
		*GetNameSafe(this),
		*GetNameSafe(LayerWidget),
		*GetNameSafe(LayerWidget->GetClass()));
}

void UAGASSPrimaryGameLayout::ResetRegisteredLayers()
{
	UE_LOG(LogAGASSFrontendLayers, Log, TEXT("ResetRegisteredLayers on %s. Previous layer count: %d."),
		*GetNameSafe(this),
		Layers.Num());
	Layers.Reset();
}

void UAGASSPrimaryGameLayout::OnWidgetStackTransitioning(UCommonActivatableWidgetContainerBase* Widget, const bool bIsTransitioning)
{
}
