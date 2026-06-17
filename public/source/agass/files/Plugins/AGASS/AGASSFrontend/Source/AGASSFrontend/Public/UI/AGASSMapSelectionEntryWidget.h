#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Types/AGASSModsTypes.h"
#include "AGASSMapSelectionEntryWidget.generated.h"

class UAGASSFrontendButtonBase;
class UCommonTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSMapSelectionEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void InitializeFromMapInfo(const FAGASSAvailableMapInfo& InMapInfo);
	UWidget* GetPrimaryFocusTarget() const;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void RefreshView();

	UFUNCTION()
	void HandleSelectClicked();

	FAGASSAvailableMapInfo MapInfo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Details;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Select;
};
