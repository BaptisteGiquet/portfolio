#include "UI/AGASSBoundActionButtonWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActionWidget.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"

UAGASSBoundActionButtonWidget::UAGASSBoundActionButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsSelectable(false);
}

TSharedRef<SWidget> UAGASSBoundActionButtonWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		WidgetTree->RootWidget = nullptr;

		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		RootSizeBox->SetWidthOverride(152.f);
		RootSizeBox->SetHeightOverride(40.f);
		WidgetTree->RootWidget = RootSizeBox;

		UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Root"));
		RootBorder->SetPadding(FMargin(14.f, 8.f));
		RootBorder->SetBrushColor(FLinearColor(0.10f, 0.11f, 0.12f, 0.96f));
		RootSizeBox->AddChild(RootBorder);

		UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));
		RootBorder->SetContent(ActionRow);

		InputActionWidget = WidgetTree->ConstructWidget<UCommonActionWidget>(UCommonActionWidget::StaticClass(), TEXT("InputActionWidget"));
		if (UHorizontalBoxSlot* InputSlot = ActionRow->AddChildToHorizontalBox(InputActionWidget))
		{
			InputSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			InputSlot->SetVerticalAlignment(VAlign_Center);
		}

		Text_ActionName = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_ActionName"));
		Text_ActionName->SetAutoWrapText(false);
		Text_ActionName->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.95f, 0.96f, 1.0f)));
		if (UHorizontalBoxSlot* TextSlot = ActionRow->AddChildToHorizontalBox(Text_ActionName))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}
