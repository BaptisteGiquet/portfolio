#pragma once

#include "CoreMinimal.h"
#include "Developer/EMRDeveloperToolsTypes.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRDeveloperToolsWidget.generated.h"

class AEMRPlayerController;
class UButton;
class UComboBoxString;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class LOD_API UEMRDeveloperToolsWidget : public UEMRFrontendCommonActivatableWidgetBase
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|DevTools")
    void RequestDeveloperToolAction(EEMRDeveloperToolAction Action);

    UFUNCTION(BlueprintCallable, Category = "EMR|DevTools")
    void RequestDeveloperToolUpgradeList();

    UFUNCTION(BlueprintCallable, Category = "EMR|DevTools")
    void RequestGiveSpecificUpgradeByIndex(int32 UpgradeIndex);

    UFUNCTION(BlueprintPure, Category = "EMR|DevTools")
    const TArray<FEMRDeveloperToolUpgradeOption>& GetDeveloperToolUpgradeOptions() const { return UpgradeOptions; }

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|DevTools")
    void BP_OnDeveloperToolActionResultReceived(const FEMRDeveloperToolActionResult& Result);

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|DevTools")
    void BP_OnDeveloperToolUpgradeOptionsUpdated();

    void InitializeForController(AEMRPlayerController* InOwningController);
    void ApplyActionResult(const FEMRDeveloperToolActionResult& Result);
    void ApplyUpgradeOptions(const TArray<FEMRDeveloperToolUpgradeOption>& InOptions);

private:
    void EnsureWidgetHierarchy();
    void BindNativeWidgetEvents();
    void UnbindNativeWidgetEvents();
    void RebuildUpgradeCombo();
    void RequestAction(EEMRDeveloperToolAction Action, FGameplayTag OptionalUpgradeTag = FGameplayTag());
    void SetStatusMessage(const FString& InMessage, bool bWasSuccess);

    UFUNCTION()
    void HandleSpawnPatientClicked();

    UFUNCTION()
    void HandleWinNightShiftClicked();

    UFUNCTION()
    void HandleLoseNightShiftClicked();

    UFUNCTION()
    void HandleEarnRevenueClicked();

    UFUNCTION()
    void HandleEarnReputationClicked();

    UFUNCTION()
    void HandleLoseReputationClicked();

    UFUNCTION()
    void HandleSpeedUpClicked();

    UFUNCTION()
    void HandleSlowDownClicked();

    UFUNCTION()
    void HandleRerollUpgradesClicked();

    UFUNCTION()
    void HandleGiveSpecificUpgradeClicked();

    UFUNCTION()
    void HandleRefreshUpgradeListClicked();

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPlayerController> CachedOwningEMRPlayerController;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UScrollBox> RootScrollBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> RootVerticalBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StatusText = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UComboBoxString> UpgradeComboBox = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_SpawnPatient = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_WinNightShift = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_LoseNightShift = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_EarnRevenue1000 = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_EarnReputation10 = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_LoseReputation10 = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_SpeedUpX2 = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_SlowDownX2 = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_RerollHubUpgrades = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_RefreshUpgradeList = nullptr;

    UPROPERTY(Transient, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_GiveSpecificUpgrade = nullptr;

    TArray<FEMRDeveloperToolUpgradeOption> UpgradeOptions;
};
