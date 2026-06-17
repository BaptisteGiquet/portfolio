#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "AGASSBenchmarkScreenWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSBenchmarkScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

public:
	UAGASSBenchmarkScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual FText GetTitleText() const override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) override;
	virtual void RefreshFromSessionState() override;
	virtual UWidget* ResolveInitialFocusTarget() const override;
#if WITH_EDITOR
	virtual void GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const override;
#endif

private:
	void SetBodyText(const FText& InText);
	void StartBenchmark();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BenchmarkBody;

	FText CurrentBodyText;
	bool bBenchmarkStarted = false;
};
