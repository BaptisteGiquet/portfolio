#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "AGASSOptionsTabListWidget.generated.h"

class UCommonButtonBase;
class UHorizontalBox;

UCLASS()
class AGASSFRONTEND_API UAGASSOptionsTabListWidget : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	UAGASSOptionsTabListWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "AGASS Frontend|Tabs")
	bool RegisterTextTab(FName TabId, const FText& TabLabel);

	UFUNCTION(BlueprintCallable, Category = "AGASS Frontend|Tabs")
	void ClearTextTabs();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

private:
	void EnsureWidgetTree();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> Tabs_Row;

	UPROPERTY(Transient)
	TMap<FName, FText> PendingTabLabels;
};
