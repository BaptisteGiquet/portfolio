#pragma once

#include "CoreMinimal.h"
#include "Widgets/GameSettingPanel.h"
#include "AGASSGameSettingPanelWidget.generated.h"

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingPanelWidget : public UGameSettingPanel
{
	GENERATED_BODY()

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void HandleNativeListSelectionChanged(UObject* Item);

	UPROPERTY(Transient)
	TObjectPtr<class UAGASSGameSettingListView> NativeListView;
};
