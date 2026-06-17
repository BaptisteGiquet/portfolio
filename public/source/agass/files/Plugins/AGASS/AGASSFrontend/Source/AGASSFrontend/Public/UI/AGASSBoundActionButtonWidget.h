#pragma once

#include "CoreMinimal.h"
#include "Input/CommonBoundActionButton.h"
#include "AGASSBoundActionButtonWidget.generated.h"

UCLASS()
class AGASSFRONTEND_API UAGASSBoundActionButtonWidget : public UCommonBoundActionButton
{
	GENERATED_BODY()

public:
	UAGASSBoundActionButtonWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
