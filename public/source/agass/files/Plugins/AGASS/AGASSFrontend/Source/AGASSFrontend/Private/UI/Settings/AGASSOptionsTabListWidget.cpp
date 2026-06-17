#include "UI/Settings/AGASSOptionsTabListWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/DataTable.h"
#include "UI/AGASSFrontendButtonBase.h"
#include "UObject/ConstructorHelpers.h"

UAGASSOptionsTabListWidget::UAGASSOptionsTabListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoListenForInput = true;
	bShouldWrapNavigation = false;

	static ConstructorHelpers::FObjectFinder<UDataTable> GenericInputActions(TEXT("/CommonUI/GenericInputActionDataTable.GenericInputActionDataTable"));
	if (GenericInputActions.Succeeded())
	{
		NextTabInputActionData.DataTable = GenericInputActions.Object;
		NextTabInputActionData.RowName = TEXT("GenericRightShoulder");
		PreviousTabInputActionData.DataTable = GenericInputActions.Object;
		PreviousTabInputActionData.RowName = TEXT("GenericLeftShoulder");
	}
}

bool UAGASSOptionsTabListWidget::RegisterTextTab(FName TabId, const FText& TabLabel)
{
	PendingTabLabels.Add(TabId, TabLabel);
	return RegisterTab(TabId, UAGASSFrontendButtonBase::StaticClass(), nullptr);
}

void UAGASSOptionsTabListWidget::ClearTextTabs()
{
	PendingTabLabels.Reset();
	RemoveAllTabs();
}

TSharedRef<SWidget> UAGASSOptionsTabListWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureWidgetTree();
	}

	return Super::RebuildWidget();
}

void UAGASSOptionsTabListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Tabs_Row == nullptr)
	{
		Tabs_Row = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("Tabs_Row")));
	}
}

void UAGASSOptionsTabListWidget::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);

	if (Tabs_Row == nullptr)
	{
		Tabs_Row = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("Tabs_Row")));
	}

	if (Tabs_Row == nullptr || TabButton == nullptr)
	{
		return;
	}

	if (UAGASSFrontendButtonBase* const AGASSButton = Cast<UAGASSFrontendButtonBase>(TabButton))
	{
		const FText* const TabLabel = PendingTabLabels.Find(TabNameID);
		AGASSButton->SetButtonText(TabLabel != nullptr ? *TabLabel : FText::FromName(TabNameID));
	}

	if (UHorizontalBoxSlot* const ButtonSlot = Tabs_Row->AddChildToHorizontalBox(TabButton))
	{
		ButtonSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}

void UAGASSOptionsTabListWidget::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabRemoval_Implementation(TabNameID, TabButton);

	PendingTabLabels.Remove(TabNameID);
}

void UAGASSOptionsTabListWidget::EnsureWidgetTree()
{
	if (WidgetTree == nullptr)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	Tabs_Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Tabs_Row"));
	WidgetTree->RootWidget = Tabs_Row;
}
