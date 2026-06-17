#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSActivatableWidget.h"
#include "AGASSFrontendScreenWidget.generated.h"

class UAGASSFrontendButtonBase;
class UAGASSInviteCodeSubsystem;
class UAGASSModsSubsystem;
class UAGASSPlatformMenuSubsystem;
class UAGASSSessionSubsystem;
class UCommonTextBlock;
class UEditableTextBox;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetTree;
class IWidgetCompilerLog;

UCLASS(Abstract)
class AGASSFRONTEND_API UAGASSFrontendScreenWidget : public UAGASSActivatableWidget
{
	GENERATED_BODY()

public:
	UAGASSFrontendScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual UWidget* ResolveInitialFocusTarget() const;
	virtual FText GetTitleText() const PURE_VIRTUAL(UAGASSFrontendScreenWidget::GetTitleText, return FText::GetEmpty(););
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) PURE_VIRTUAL(UAGASSFrontendScreenWidget::BuildFallbackScreenContent, );
	virtual void RefreshFromSessionState();
#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const override;
	virtual void GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const;
	void ValidateRequiredNamedWidgets(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog, TConstArrayView<FName> RequiredWidgetNames) const;
#endif

	UAGASSSessionSubsystem* GetSessionSubsystem() const;
	UAGASSInviteCodeSubsystem* GetInviteCodeSubsystem() const;
	UAGASSModsSubsystem* GetModsSubsystem() const;
	UAGASSPlatformMenuSubsystem* GetPlatformMenuSubsystem() const;

	UAGASSFrontendButtonBase* CreateMenuButton(const FText& Label, const FName& WidgetName = NAME_None);
	UAGASSFrontendButtonBase* AddMenuButton(UVerticalBox* Parent, const FText& Label, const FName& WidgetName = NAME_None);
	UEditableTextBox* AddEditableTextBox(UVerticalBox* Parent, const FText& HintText, const FName& WidgetName = NAME_None);
	UTextBlock* AddBodyText(UVerticalBox* Parent, const FText& Text, int32 FontSize = 16, const FName& WidgetName = NAME_None);
	UHorizontalBox* AddHorizontalBox(UVerticalBox* Parent, const FName& WidgetName = NAME_None);
	UScrollBox* AddScrollBox(UVerticalBox* Parent, float HeightOverride = 0.f, const FName& WidgetName = NAME_None);
	void AddSpacer(UVerticalBox* Parent, float Height);

	template <typename WidgetT>
	WidgetT* ResolveWidgetByName(TObjectPtr<WidgetT>& WidgetPtr, const TCHAR* WidgetName)
	{
		if (WidgetPtr == nullptr)
		{
			WidgetPtr = Cast<WidgetT>(GetWidgetFromName(FName(WidgetName)));
		}

		return WidgetPtr.Get();
	}

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Status;

private:
	void HandleSessionStateChanged();

	FDelegateHandle SessionStateChangedHandle;
};
