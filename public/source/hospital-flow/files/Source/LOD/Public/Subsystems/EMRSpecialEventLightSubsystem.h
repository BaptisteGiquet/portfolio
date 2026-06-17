#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EMRSpecialEventLightSubsystem.generated.h"

class AEMRNightShiftGameState;
class ULightComponent;
enum class EEMRSpecialEventPhase : uint8;

UCLASS()
class LOD_API UEMRSpecialEventLightSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    struct FTrackedLight
    {
        TWeakObjectPtr<ULightComponent> Light;
        FLinearColor OriginalColor = FLinearColor::White;
        float OriginalIntensity = 0.0f;
    };

    struct FLightDiscoveryStats
    {
        int32 ActorsScanned = 0;
        int32 LightComponentsScanned = 0;
        int32 TaggedComponentsFound = 0;
        int32 DynamicMutableCount = 0;
        int32 DynamicBlockedCount = 0;
        TArray<FString> SampleDynamicBlockedComponents;
    };

    void BindToRunState();
    void UnbindFromRunState();
    void RefreshFromRunState();
    void EvaluateActiveSpecialEventState(float DeltaTime);
    void StartFlicker(EEMRSpecialEventPhase Phase, const FName& LightTag, const FLinearColor& FlickerColor, float FlickerRateHz);
    void StopFlicker(bool bResetDiagnostics = true);
    FLightDiscoveryStats RebuildTrackedLights(const FName& LightTag);
    void HandleSpecialEventPhaseChanged(EEMRSpecialEventPhase NewPhase, FName EventId, float PhaseServerTimestamp);

    TArray<FTrackedLight> TrackedLights;
    TWeakObjectPtr<AEMRNightShiftGameState> CachedRunState;
    FDelegateHandle SpecialEventPhaseHandle;

    bool bFlickerActive = false;
    bool bWarnedMissingLights = false;
    bool bWarnedInvalidTagForActivePhase = false;
    bool bWarnedDynamicBlockedLights = false;
    float FlickerElapsedSeconds = 0.0f;
    float FlickerRateHz = 1.0f;
    float NoLightRetryIntervalSeconds = 1.0f;
    float NoLightRetryElapsedSeconds = 0.0f;
    FLinearColor ActiveFlickerColor = FLinearColor::Red;
    FName ActiveLightTag = NAME_None;
};
