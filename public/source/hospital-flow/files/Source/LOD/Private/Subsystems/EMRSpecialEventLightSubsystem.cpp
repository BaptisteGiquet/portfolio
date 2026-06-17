#include "Subsystems/EMRSpecialEventLightSubsystem.h"

#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "Framework/EMRNightShiftGameState.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpecialEventLights, Log, All);

void UEMRSpecialEventLightSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BindToRunState();
}

void UEMRSpecialEventLightSubsystem::Deinitialize()
{
    StopFlicker();
    UnbindFromRunState();
    Super::Deinitialize();
}

void UEMRSpecialEventLightSubsystem::Tick(const float DeltaTime)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!CachedRunState.IsValid())
    {
        BindToRunState();
    }

    EvaluateActiveSpecialEventState(DeltaTime);

    if (!bFlickerActive)
    {
        return;
    }

    FlickerElapsedSeconds += DeltaTime;
    const float Pulse = 0.5f + (0.5f * FMath::Sin(FlickerElapsedSeconds * FlickerRateHz * UE_TWO_PI));
    const float IntensityScalar = FMath::Lerp(0.45f, 1.0f, Pulse);

    for (int32 Index = TrackedLights.Num() - 1; Index >= 0; --Index)
    {
        FTrackedLight& TrackedLight = TrackedLights[Index];
        ULightComponent* LightComponent = TrackedLight.Light.Get();
        if (!LightComponent)
        {
            TrackedLights.RemoveAtSwap(Index);
            continue;
        }

        LightComponent->SetLightColor(ActiveFlickerColor);
        LightComponent->SetIntensity(TrackedLight.OriginalIntensity * IntensityScalar);
    }
}

TStatId UEMRSpecialEventLightSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UEMRSpecialEventLightSubsystem, STATGROUP_Tickables);
}

bool UEMRSpecialEventLightSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

void UEMRSpecialEventLightSubsystem::BindToRunState()
{
    if (!GetWorld())
    {
        return;
    }

    AEMRNightShiftGameState* RunState = GetWorld()->GetGameState<AEMRNightShiftGameState>();
    if (!RunState)
    {
        return;
    }

    if (CachedRunState.Get() == RunState && SpecialEventPhaseHandle.IsValid())
    {
        return;
    }

    UnbindFromRunState();
    CachedRunState = RunState;
    SpecialEventPhaseHandle = RunState->OnSpecialEventPhaseChanged().AddUObject(
        this,
        &ThisClass::HandleSpecialEventPhaseChanged);

    RefreshFromRunState();
}

void UEMRSpecialEventLightSubsystem::UnbindFromRunState()
{
    if (AEMRNightShiftGameState* RunState = CachedRunState.Get())
    {
        if (SpecialEventPhaseHandle.IsValid())
        {
            RunState->OnSpecialEventPhaseChanged().Remove(SpecialEventPhaseHandle);
            SpecialEventPhaseHandle.Reset();
        }
    }

    CachedRunState.Reset();
}

void UEMRSpecialEventLightSubsystem::RefreshFromRunState()
{
    EvaluateActiveSpecialEventState(0.0f);
}

void UEMRSpecialEventLightSubsystem::EvaluateActiveSpecialEventState(const float DeltaTime)
{
    const AEMRNightShiftGameState* RunState = CachedRunState.Get();
    if (!RunState)
    {
        StopFlicker();
        return;
    }

    const EEMRSpecialEventPhase Phase = RunState->GetSpecialEventPhase();
    const bool bShouldFlicker = Phase == EEMRSpecialEventPhase::Alert;
    if (!bShouldFlicker)
    {
        StopFlicker();
        return;
    }

    const FName LightTag = RunState->GetSpecialEventFlickerLightTag();
    if (LightTag.IsNone())
    {
        if (!bWarnedInvalidTagForActivePhase)
        {
            bWarnedInvalidTagForActivePhase = true;
            UE_LOG(
                LogSpecialEventLights,
                Warning,
                TEXT("[SpecialEventLights] Alert phase but FlickerLightTag is None (EventId=%s, Phase=%d)."),
                *RunState->GetActiveSpecialEventId().ToString(),
                static_cast<int32>(Phase));
        }

        StopFlicker(false);
        return;
    }

    const FLinearColor DesiredFlickerColor = RunState->GetSpecialEventFlickerColor();
    const float DesiredFlickerRateHz = FMath::Max(RunState->GetSpecialEventLightFlickerRateHz(), 0.01f);
    const bool bNeedsRestart =
    !bFlickerActive ||
    ActiveLightTag != LightTag ||
    !ActiveFlickerColor.Equals(DesiredFlickerColor) ||
    !FMath::IsNearlyEqual(FlickerRateHz, DesiredFlickerRateHz);

    if (bNeedsRestart)
    {
        StartFlicker(Phase, LightTag, DesiredFlickerColor, DesiredFlickerRateHz);
        return;
    }

    if (TrackedLights.IsEmpty())
    {
        NoLightRetryElapsedSeconds += FMath::Max(DeltaTime, 0.0f);
        if (NoLightRetryElapsedSeconds >= NoLightRetryIntervalSeconds)
        {
            NoLightRetryElapsedSeconds = 0.0f;
            StartFlicker(Phase, LightTag, DesiredFlickerColor, DesiredFlickerRateHz);
        }

        return;
    }

    NoLightRetryElapsedSeconds = 0.0f;
}

void UEMRSpecialEventLightSubsystem::StartFlicker(const EEMRSpecialEventPhase Phase, const FName& LightTag, const FLinearColor& FlickerColor, const float InFlickerRateHz)
{
    if (LightTag.IsNone())
    {
        StopFlicker(false);
        return;
    }

    const float TargetRateHz = FMath::Max(InFlickerRateHz, 0.01f);
    const bool bTagChanged = ActiveLightTag != LightTag;
    const bool bNeedsRefresh =
    !bFlickerActive ||
    bTagChanged ||
    !ActiveFlickerColor.Equals(FlickerColor) ||
    !FMath::IsNearlyEqual(FlickerRateHz, TargetRateHz) ||
    TrackedLights.IsEmpty();

    if (bNeedsRefresh)
    {
        StopFlicker(false);
        if (bTagChanged)
        {
            bWarnedMissingLights = false;
            bWarnedDynamicBlockedLights = false;
            bWarnedInvalidTagForActivePhase = false;
        }

        const FLightDiscoveryStats Stats = RebuildTrackedLights(LightTag);
        if (TrackedLights.IsEmpty())
        {
            if (Stats.TaggedComponentsFound <= 0)
            {
                if (!bWarnedMissingLights)
                {
                    bWarnedMissingLights = true;
                    UE_LOG(
                        LogSpecialEventLights,
                        Warning,
                        TEXT("[SpecialEventLights] No tagged lights found (Phase=%d, Tag=%s, ActorsScanned=%d, LightComponents=%d)."),
                        static_cast<int32>(Phase),
                        *LightTag.ToString(),
                        Stats.ActorsScanned,
                        Stats.LightComponentsScanned);
                }
            }
            else if (Stats.DynamicMutableCount <= 0 && Stats.DynamicBlockedCount > 0 && !bWarnedDynamicBlockedLights)
            {
                bWarnedDynamicBlockedLights = true;
                const FString BlockedSamples = Stats.SampleDynamicBlockedComponents.IsEmpty()
                    ? TEXT("<none>")
                    : FString::Join(Stats.SampleDynamicBlockedComponents, TEXT(", "));

                UE_LOG(
                    LogSpecialEventLights,
                    Warning,
                    TEXT("[SpecialEventLights] Tagged lights are static/dynamic-blocked (Phase=%d, Tag=%s, Tagged=%d, DynamicBlocked=%d, Examples=%s)."),
                    static_cast<int32>(Phase),
                    *LightTag.ToString(),
                    Stats.TaggedComponentsFound,
                    Stats.DynamicBlockedCount,
                    *BlockedSamples);
            }
        }
    }

    bFlickerActive = true;
    ActiveLightTag = LightTag;
    ActiveFlickerColor = FlickerColor;
    FlickerRateHz = TargetRateHz;
    NoLightRetryElapsedSeconds = 0.0f;
}

void UEMRSpecialEventLightSubsystem::StopFlicker(const bool bResetDiagnostics)
{
    int32 LostReferencesCount = 0;
    for (FTrackedLight& TrackedLight : TrackedLights)
    {
        ULightComponent* LightComponent = TrackedLight.Light.Get();
        if (!LightComponent)
        {
            ++LostReferencesCount;
            continue;
        }

        LightComponent->SetLightColor(TrackedLight.OriginalColor);
        LightComponent->SetIntensity(TrackedLight.OriginalIntensity);
    }

    if (LostReferencesCount > 0)
    {
        UE_LOG(
            LogSpecialEventLights,
            Error,
            TEXT("[SpecialEventLights] Failed to restore %d tracked lights (invalid references)."),
            LostReferencesCount);
    }

    TrackedLights.Reset();
    bFlickerActive = false;
    FlickerElapsedSeconds = 0.0f;
    FlickerRateHz = 1.0f;
    NoLightRetryElapsedSeconds = 0.0f;
    ActiveLightTag = NAME_None;

    if (bResetDiagnostics)
    {
        bWarnedMissingLights = false;
        bWarnedInvalidTagForActivePhase = false;
        bWarnedDynamicBlockedLights = false;
    }
}

UEMRSpecialEventLightSubsystem::FLightDiscoveryStats UEMRSpecialEventLightSubsystem::RebuildTrackedLights(const FName& LightTag)
{
    TrackedLights.Reset();
    FLightDiscoveryStats Stats;

    UWorld* World = GetWorld();
    if (!World || LightTag.IsNone())
    {
        return Stats;
    }

    constexpr int32 MaxSampleCount = 3;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor)
        {
            continue;
        }

        ++Stats.ActorsScanned;
        const bool bActorTagged = Actor->ActorHasTag(LightTag);

        TArray<ULightComponent*> LightComponents;
        Actor->GetComponents<ULightComponent>(LightComponents);
        for (ULightComponent* LightComponent : LightComponents)
        {
            ++Stats.LightComponentsScanned;

            if (!LightComponent)
            {
                continue;
            }

            if (!bActorTagged && !LightComponent->ComponentHasTag(LightTag))
            {
                continue;
            }

            ++Stats.TaggedComponentsFound;

            // Mirror runtime light mutation constraints without calling protected API:
            // ULightComponent::SetLightColor/SetIntensity effectively no-op on registered static components.
            const bool bDynamicChangeBlocked =
                LightComponent->IsRegistered() &&
                LightComponent->GetMobility() == EComponentMobility::Static;

            if (bDynamicChangeBlocked)
            {
                ++Stats.DynamicBlockedCount;
                if (Stats.SampleDynamicBlockedComponents.Num() < MaxSampleCount)
                {
                    Stats.SampleDynamicBlockedComponents.Add(GetPathNameSafe(LightComponent));
                }
                continue;
            }

            ++Stats.DynamicMutableCount;
            FTrackedLight& TrackedLight = TrackedLights.AddDefaulted_GetRef();
            TrackedLight.Light = LightComponent;
            TrackedLight.OriginalColor = LightComponent->GetLightColor();
            TrackedLight.OriginalIntensity = LightComponent->Intensity;
        }
    }

    return Stats;
}

void UEMRSpecialEventLightSubsystem::HandleSpecialEventPhaseChanged(
    EEMRSpecialEventPhase NewPhase,
    FName EventId,
    const float PhaseServerTimestamp)
{
    (void)NewPhase;
    (void)EventId;
    (void)PhaseServerTimestamp;
    RefreshFromRunState();
}
