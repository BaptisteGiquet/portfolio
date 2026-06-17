#include "UI/AGASSFrontendPrototypeLayoutWidget.h"

#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UCommonActivatableWidget* UAGASSFrontendPrototypeLayoutWidget::PushPrototypeWidget(const TSubclassOf<UCommonActivatableWidget> InWidgetClass)
{
	return ScreenStack != nullptr && InWidgetClass != nullptr
		? ScreenStack->AddWidget(InWidgetClass)
		: nullptr;
}
