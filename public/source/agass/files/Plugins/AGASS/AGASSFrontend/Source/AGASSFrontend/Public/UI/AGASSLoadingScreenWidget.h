#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "AGASSLoadingScreenWidget.generated.h"

class UAGASSFrontendButtonBase;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSLoadingScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

public:
	UAGASSLoadingScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual FText GetTitleText() const override;
	virtual void NativeConstruct() override;
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) override;
	virtual bool NativeOnHandleBackAction() override;
	virtual UWidget* ResolveInitialFocusTarget() const override;

private:
	UFUNCTION()
	void HandleCancelClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_ReturnToFrontend;
};
