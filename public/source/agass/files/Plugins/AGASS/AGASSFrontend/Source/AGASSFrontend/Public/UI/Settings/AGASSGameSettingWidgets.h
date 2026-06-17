#pragma once

#include "CoreMinimal.h"
#include "Widgets/GameSettingListEntry.h"
#include "Widgets/Misc/GameSettingRotator.h"
#include "AGASSGameSettingWidgets.generated.h"

struct FFocusEvent;
struct FGeometry;
struct FPointerEvent;

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingRotatorWidget : public UGameSettingRotator
{
	GENERATED_BODY()

public:
	UAGASSGameSettingRotatorWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void InitializeNativeClassData() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingScalarEntryWidget : public UGameSettingListEntrySetting_Scalar
{
	GENERATED_BODY()

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
};

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingDiscreteEntryWidget : public UGameSettingListEntrySetting_Discrete
{
	GENERATED_BODY()

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingActionEntryWidget : public UGameSettingListEntrySetting_Action
{
	GENERATED_BODY()

public:
	virtual void SetSetting(UGameSetting* InSetting) override;

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingKeyBindEntryWidget : public UGameSettingListEntry_Setting
{
	GENERATED_BODY()

public:
	virtual void SetSetting(UGameSetting* InSetting) override;

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeOnEntryReleased() override;
	virtual void OnSettingChanged() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void HandleRebindClicked();
	void HandleResetClicked();
	void HandleKeySelected(FKey InKey);
	void HandleKeySelectionCanceled();
	void HandleDuplicateConfirmed();
	void HandleDuplicateCanceled();
	void Refresh();

	UPROPERTY()
	TObjectPtr<class UAGASSGameSettingKeyBind> KeyBindSetting;

	FKey PendingSelectedKey = EKeys::Invalid;
};

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingSectionHeaderEntryWidget : public UGameSettingListEntry_Setting
{
	GENERATED_BODY()

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingNavigationEntryWidget : public UGameSettingListEntrySetting_Navigation
{
	GENERATED_BODY()

public:
	virtual void SetSetting(UGameSetting* InSetting) override;

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
