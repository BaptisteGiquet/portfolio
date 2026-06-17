#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DEPRECATEDEMRNightShiftStatusWidget.generated.h"

struct FOnAttributeChangeData;
class UTextBlock;
class UAbilitySystemComponent;
class AEMRNightShiftGameState;

/**
 * Displays the key information for the active NightShift (timer, quota, revenue, reputation).
 */
UCLASS()
class LOD_API UDEPRECATEDEMRNightShiftStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void InitializeFromRunState();
    void TryBindToTeamASC();
    void UnbindFromTeamASC();
    void BindToRunState();
    void UnbindFromRunState();

    void UpdateTimeDisplay(float RemainingSeconds);
    void UpdateCycleQuota(float NewQuota);
    void UpdateCycleRevenue(float NewRevenue);
    void UpdateReputation(float NewReputation);
    void UpdateCycleIndexDisplay(int32 CycleIndex);
    void UpdateNightShiftIndexDisplay(int32 NightShiftIndex);


    void HandleCycleInfoUpdated(int32 CycleIndex, float CycleQuota, float CycleStartRevenue);
    void OnRevenueChanged(const FOnAttributeChangeData& ChangeData);

    void OnReputationChanged(const FOnAttributeChangeData& ChangeData);

    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_TimeRemaining;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CycleQuota;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CycleRevenue;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_Reputation;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_CycleIndex;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Text_NightShiftIndex;

    TWeakObjectPtr<AEMRNightShiftGameState> CachedNightShiftGameState;
    TWeakObjectPtr<UAbilitySystemComponent> CachedTeamASC;

    FDelegateHandle RevenueDelegateHandle;
    FDelegateHandle ReputationDelegateHandle;
    FDelegateHandle CycleInfoDelegateHandle;

    float CachedQuota = 0.f;
    float CachedRemainingTime = -1.f;
    int32 CachedCycleIndex = INDEX_NONE;
    int32 CachedNightShiftIndex = INDEX_NONE;
};
