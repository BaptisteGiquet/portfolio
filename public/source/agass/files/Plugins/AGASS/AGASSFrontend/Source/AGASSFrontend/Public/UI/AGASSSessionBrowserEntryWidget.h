#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "AGASSSessionBrowserEntryWidget.generated.h"

class UAGASSFrontendButtonBase;
class UCommonTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSSessionBrowserEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void InitializeFromResult(int32 InSearchResultIndex, const FAGASSSessionSearchResultData& InSessionData);
	UWidget* GetPrimaryFocusTarget() const;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void RefreshView();
	FText GetTransportText() const;

	UFUNCTION()
	void HandleJoinClicked();

	int32 SearchResultIndex = INDEX_NONE;
	FAGASSSessionSearchResultData SessionData;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Details;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Reason;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Join;
};
