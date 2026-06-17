#include "UI/Settings/AGASSGameSettingPanelWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "UI/Settings/AGASSGameSettingListView.h"
#include "UObject/UnrealType.h"

namespace
{
	void AssignObjectWidgetProperty(UUserWidget* Owner, UClass* DeclaringClass, const TCHAR* PropertyName, UObject* Value)
	{
		if (Owner == nullptr || DeclaringClass == nullptr)
		{
			return;
		}

		if (FObjectPropertyBase* const Property = FindFProperty<FObjectPropertyBase>(DeclaringClass, PropertyName))
		{
			Property->SetObjectPropertyValue_InContainer(Owner, Value);
		}
	}

	void EnsurePanelWidgetTree(UAGASSGameSettingPanelWidget* Owner)
	{
		if (Owner == nullptr)
		{
			return;
		}

		if (Owner->WidgetTree == nullptr)
		{
			Owner->WidgetTree = NewObject<UWidgetTree>(Owner, TEXT("WidgetTree"), RF_Transient);
		}

		if (Owner->WidgetTree->RootWidget != nullptr)
		{
			return;
		}

		UBorder* const Border = Owner->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_SettingsPanel"));
		Border->SetPadding(FMargin(10.f));
		Border->SetBrushColor(FLinearColor(0.32f, 0.34f, 0.34f, 0.92f));

		UAGASSGameSettingListView* const ListView = Owner->WidgetTree->ConstructWidget<UAGASSGameSettingListView>(UAGASSGameSettingListView::StaticClass(), TEXT("ListView_Settings"));
		Border->SetContent(ListView);

		Owner->WidgetTree->RootWidget = Border;
	}
}

void UAGASSGameSettingPanelWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsurePanelWidgetTree(this);
	}
}

void UAGASSGameSettingPanelWidget::NativeOnInitialized()
{
	AssignObjectWidgetProperty(this, UGameSettingPanel::StaticClass(), TEXT("ListView_Settings"), GetWidgetFromName(TEXT("ListView_Settings")));
	AssignObjectWidgetProperty(this, UGameSettingPanel::StaticClass(), TEXT("Details_Settings"), GetWidgetFromName(TEXT("Details_Settings")));
	NativeListView = Cast<UAGASSGameSettingListView>(GetWidgetFromName(TEXT("ListView_Settings")));
	if (NativeListView != nullptr)
	{
		NativeListView->OnItemSelectionChanged().RemoveAll(this);
		NativeListView->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleNativeListSelectionChanged);
	}

	Super::NativeOnInitialized();
}

TSharedRef<SWidget> UAGASSGameSettingPanelWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsurePanelWidgetTree(this);
	}

	return Super::RebuildWidget();
}

void UAGASSGameSettingPanelWidget::HandleNativeListSelectionChanged(UObject* Item)
{
	if (NativeListView == nullptr || Item == nullptr)
	{
		return;
	}

	const int32 SelectedIndex = NativeListView->GetIndexForItem(Item);
	if (SelectedIndex != INDEX_NONE)
	{
		NativeListView->NavigateToIndex(SelectedIndex);
		NativeListView->ScrollIndexIntoView(SelectedIndex);
	}
}
