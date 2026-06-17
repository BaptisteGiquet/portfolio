#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "AGASSCreditsScreenWidget.generated.h"

class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSCreditsScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

public:
	UAGASSCreditsScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual FText GetTitleText() const override;
	virtual void NativeConstruct() override;
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) override;
	virtual UWidget* ResolveInitialFocusTarget() const override;
};
