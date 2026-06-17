
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DEPRECATEDEMRNightShiftDebugWidget.generated.h"

class UTextBlock;
class UScrollBox;
class UEMRNightShiftSpawnSubsystem;
class UEMRPatientPoolSubsystem;
class AEMRNightShiftGameState;

/**
 * Debug widget that dumps the current NightShift definition and patient spawn runtime state.
 */
UCLASS()
class LOD_API UDEPRECATEDEMRNightShiftDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UDEPRECATEDEMRNightShiftDebugWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** Force an immediate refresh of the debug text. */
    UFUNCTION(BlueprintCallable, Category = "EMR|Debug")
    void RefreshDebugInfo();

protected:
    /** Interval in seconds between automatic refreshes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Debug")
    float RefreshInterval = 0.5f;

    /** Optional scroll box created at runtime. */
    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ScrollBoxContainer;

    /** Text block that receives the formatted debug content. */
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> DebugTextBlock;

private:
    void EnsureWidgetHierarchy();
    void AppendNightShiftSection(FStringBuilderBase& Builder, const AEMRNightShiftGameState* NightShiftGameState) const;
    void AppendSpawnSubsystemSection(FStringBuilderBase& Builder,const AEMRNightShiftGameState* NightShiftGameState, const UEMRNightShiftSpawnSubsystem* SpawnSubsystem) const;
    void AppendPatientPoolSection(FStringBuilderBase& Builder,const AEMRNightShiftGameState* NightShiftGameState, const UEMRPatientPoolSubsystem* PoolSubsystem) const;

    float RefreshAccumulator = 0.f;
};
