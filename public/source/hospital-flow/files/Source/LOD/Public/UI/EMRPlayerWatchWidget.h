#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMRPlayerWatchWidget.generated.h"

struct FOnAttributeChangeData;
class UTextBlock;
class UAbilitySystemComponent;
class AEMRNightShiftGameState;
enum class EEMRSpecialEventPhase : uint8;

UCLASS()
class LOD_API UEMRPlayerWatchWidget : public UUserWidget
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

    void UpdateWatchTime(float RemainingSeconds, float TotalDurationSeconds);
    void UpdateTotalRevenue(float NewRevenue);
    void UpdateCycleQuota(float NewQuota);
    void UpdateReputation(float NewReputation);
    void UpdateOvertimeState(bool bInOvertime);

    void HandleCycleInfoUpdated(int32 CycleIndex, float CycleQuota, float CycleStartRevenue);
    void OnTotalRevenueChanged(const FOnAttributeChangeData& ChangeData);
    void OnReputationChanged(const FOnAttributeChangeData& ChangeData);
    void HandleOvertimeStarted();
    void HandleSpecialEventPhaseChanged(EEMRSpecialEventPhase NewPhase, FName EventId, float PhaseServerTimestamp);
    void UpdateSpecialEventAlertState();
    void SetDefaultWatchDataVisible(bool bVisible);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_TimeHours;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_TimeMinutes;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_TimePeriod;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_TotalRevenue;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CycleQuota;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_Reputation;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_AlertTitle;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_AlertDescription;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Box_DefaultWatchData;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch")
    FSlateColor OvertimeHourColor = FSlateColor(FLinearColor::Red);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch")
    FSlateColor ReputationZeroColor = FSlateColor(FLinearColor::Red);

    FSlateColor DefaultHourColor;
    FSlateColor DefaultReputationColor;

    TWeakObjectPtr<AEMRNightShiftGameState> CachedNightShiftGameState;
    TWeakObjectPtr<UAbilitySystemComponent> CachedTeamASC;

    FDelegateHandle RevenueDelegateHandle;
    FDelegateHandle ReputationDelegateHandle;
    FDelegateHandle CycleInfoDelegateHandle;
    FDelegateHandle OvertimeDelegateHandle;
    FDelegateHandle SpecialEventPhaseDelegateHandle;

    float CachedNightShiftDuration = 0.f;
    float CachedRemainingTime = -1.f;
    int32 CachedHour = INDEX_NONE;
    int32 CachedMinute = INDEX_NONE;
    bool bCachedIsPM = false;
    bool bOvertimeActive = false;
    bool bSpecialEventAlertActive = false;
};
