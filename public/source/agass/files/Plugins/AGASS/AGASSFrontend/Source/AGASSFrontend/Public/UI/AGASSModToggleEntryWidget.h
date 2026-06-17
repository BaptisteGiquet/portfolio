#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Types/AGASSModsTypes.h"
#include "AGASSModToggleEntryWidget.generated.h"

class UAGASSFrontendButtonBase;
class UCommonTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSModToggleEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void InitializeFromModInfo(const FAGASSDiscoveredModInfo& InModInfo);
	UWidget* GetPrimaryFocusTarget() const;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void RefreshView();

	UFUNCTION()
	void HandleToggleClicked();

	FAGASSDiscoveredModInfo ModInfo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Details;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Toggle;
};
