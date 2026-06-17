
#pragma once

#include "CoreMinimal.h"
#include "UI/Common/EMRPreferredFocusWidgetInterface.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRCommonHubNightShiftSelectionWidget.generated.h"

class UCommonTextBlock;
class UEMRFrontendCommonButtonBase;
class UEMRFrontendCommonListView;
class UEMRNightShiftDefinition;
class UEMRCommonHubNightShiftListDataObject;
class AEMRNightShiftGameState;
class UWidget;

UCLASS()
class LOD_API UEMRCommonHubNightShiftSelectionWidget : public UEMRFrontendCommonActivatableWidgetBase, public IEMRPreferredFocusWidgetInterface
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	virtual UWidget* GetPreferredFocusWidget_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "EMR|NightShift")
	void RefreshNightShifts();

protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UFUNCTION(BlueprintImplementableEvent, Category = "EMR|NightShift|UI", meta = (DisplayName = "On Confirm Availability Changed"))
	void BP_OnConfirmAvailabilityChanged(bool bCanConfirm, bool bInSelectionCommitted);

private:
	void BuildEntriesFromSubsystem(TArray<UEMRCommonHubNightShiftListDataObject*>& OutEntries, bool& bOutHasProgression);
	void BuildEntriesFromGameState(TArray<UEMRCommonHubNightShiftListDataObject*>& OutEntries, bool bSubsystemHasProgression);

	void UpdateSelectionFromItem(UObject* SelectedItem);
	void UpdateConfirmState(bool bHasUnlockedSelection);

		
	void OnListViewItemHoveredChanged(UObject* InHoveredItem, bool bWasHovered);
	void OnListViewItemSelectionChanged(UObject* InSelectedItem);
	
	UFUNCTION()
	void HandleSelectionChanged(UObject* SelectedItem);

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	void BindToGameStateUpdates();
	void UnbindFromGameStateUpdates();
	AEMRNightShiftGameState* ResolveNightShiftGameState() const;
	void OnGameStateNightShiftsUpdated();
	void OnGameStateSelectionCommittedChanged();

	//***** Bound Widgets *****//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonListView> CommonListView_NightShifts = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Confirm = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Cancel = nullptr;
	//***** Bound Widgets *****//

	UPROPERTY()
	TObjectPtr<UEMRNightShiftDefinition> SelectedNightShift = nullptr;

	UPROPERTY()
	int32 SelectedOfferIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEMRCommonHubNightShiftListDataObject>> NightShiftEntries;

	TWeakObjectPtr<AEMRNightShiftGameState> CachedNightShiftGameState;
	FDelegateHandle GameStateDelegateHandle;
	FTimerHandle GameStateBindingRetryHandle;

	bool bSelectionCommitted = false;
};
