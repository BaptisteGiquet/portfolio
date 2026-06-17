#pragma once

#include "CoreMinimal.h"
#include "UI/Common/EMRPreferredFocusWidgetInterface.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRCommonHubUpgradeVoteWidget.generated.h"

class UCommonTextBlock;
class UEMRFrontendCommonListView;
class UEMRCommonHubUpgradeListDataObject;
class UEMRFrontendCommonButtonBase;
class AEMRNightShiftGameState;
class UWidget;

UCLASS()
class LOD_API UEMRCommonHubUpgradeVoteWidget : public UEMRFrontendCommonActivatableWidgetBase, public IEMRPreferredFocusWidgetInterface
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

    virtual UWidget* GetPreferredFocusWidget_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Hub|UpgradeVote")
    void RefreshUpgradeOffers();

protected:
    virtual UWidget* NativeGetDesiredFocusTarget() const override;

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|Hub|UpgradeVote|UI", meta = (DisplayName = "On Vote State Changed"))
    void BP_OnVoteStateChanged(bool bVoteActive, float RemainingSeconds, int32 WinningOfferIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|Hub|UpgradeVote|UI", meta = (DisplayName = "On Start Availability Changed"))
    void BP_OnStartAvailabilityChanged(bool bCanStartNow, int32 SubmittedVoterCount, int32 RequiredVoterCount);

private:
    void BindToGameStateUpdates();
    void UnbindFromGameStateUpdates();
    AEMRNightShiftGameState* ResolveNightShiftGameState() const;
    void HandleSelectionChanged(UObject* SelectedItem);
    void HandleStartClicked();
    void OnListViewItemHoveredChanged(UObject* InHoveredItem, bool bWasHovered);
    void OnGameStateVoteStateChanged();
    void TickCountdown();
    void UpdateCountdownUI();
    void UpdateStartButtonState();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEMRFrontendCommonListView> CommonListView_Upgrades = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_Countdown = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_Status = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Start = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEMRCommonHubUpgradeListDataObject>> UpgradeEntries;

    TWeakObjectPtr<AEMRNightShiftGameState> CachedNightShiftGameState;
    FDelegateHandle VoteStateChangedHandle;
    FTimerHandle GameStateBindingRetryHandle;
    FTimerHandle CountdownRefreshHandle;

    int32 LocalSelectedOfferIndex = INDEX_NONE;
};

