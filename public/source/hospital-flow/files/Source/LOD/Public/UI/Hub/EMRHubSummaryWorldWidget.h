#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "EMRHubSummaryWorldWidget.generated.h"

class AEMRNightShiftGameState;
class UAbilitySystemComponent;
class UTextBlock;
class UBorder;
struct FOnAttributeChangeData;
enum class EER_RunPhase : uint8;

UCLASS()
class LOD_API UEMRHubSummaryWorldWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void InitializeFromRunState();
    void BindToRunState();
    void UnbindFromRunState();
    void TryBindToTeamASC();
    void UnbindFromTeamASC();
    void RefreshSummary();
    int32 ResolveMaxNightShiftsInCycle() const;
    bool ShouldUseQuotaWarningColor(int32 NextNightShiftDisplay, int32 MaxNightShiftsInCycle, EER_RunPhase RunPhase, bool bHasReachedCycleQuota) const;
    void SetOptionalIntegerText(UTextBlock* TextBlock, int32 Value) const;
    void SetOptionalBorderVisibility(UBorder* Border, ESlateVisibility NewVisibility) const;

    void OnTotalRevenueChanged(const FOnAttributeChangeData& ChangeData);

    void HandleCycleInfoUpdated(int32 CycleIndex, float CycleQuota, float CycleStartRevenue);
    void HandleRunPhaseChanged(EER_RunPhase NewPhase, EER_RunPhase PreviousPhase);
    void SetOptionalTextVisibility(UTextBlock* TextBlock, ESlateVisibility NewVisibility) const;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CurrentCycleNumber = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CurrentNightShiftNumber = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_MaxNightShiftInCycle = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CurrentCycleRevenue = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CurrentCycleQuota = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_EndOfSessionResult = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UBorder> Border_EndOfSessionResult = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Summary")
    FText EndOfSessionWonText;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Summary")
    FText EndOfSessionLostText;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Summary")
    FSlateColor QuotaWarningColor = FSlateColor(FLinearColor::Red);

    FSlateColor DefaultQuotaColor;

    TWeakObjectPtr<AEMRNightShiftGameState> CachedNightShiftGameState;
    TWeakObjectPtr<UAbilitySystemComponent> CachedTeamASC;

    FDelegateHandle RevenueDelegateHandle;
    FDelegateHandle CycleInfoDelegateHandle;
    FDelegateHandle RunPhaseChangedHandle;

    int32 CachedDisplayedCycle = INDEX_NONE;
    int32 CachedDisplayedNightShift = INDEX_NONE;
    int32 CachedDisplayedMaxNightShift = INDEX_NONE;
    int32 CachedDisplayedCycleRevenue = INDEX_NONE;
    int32 CachedDisplayedCycleQuota = INDEX_NONE;
    bool bCachedQuotaWarning = false;
};
