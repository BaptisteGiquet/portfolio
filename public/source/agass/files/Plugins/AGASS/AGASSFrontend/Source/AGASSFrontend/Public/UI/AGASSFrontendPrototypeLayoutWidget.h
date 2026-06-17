#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AGASSFrontendPrototypeLayoutWidget.generated.h"

class UCommonActivatableWidget;
class UCommonActivatableWidgetStack;

UCLASS(Abstract)
class AGASSFRONTEND_API UAGASSFrontendPrototypeLayoutWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidget* PushPrototypeWidget(TSubclassOf<UCommonActivatableWidget> InWidgetClass);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ScreenStack;
};
