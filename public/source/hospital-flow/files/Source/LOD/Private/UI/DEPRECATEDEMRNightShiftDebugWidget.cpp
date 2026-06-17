#include "UI/DEPRECATEDEMRNightShiftDebugWidget.h"

#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Subsystems/EMRRunRulesSubsystem.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "Subsystems/EMRPatientPoolSubsystem.h"
#include "Blueprint/WidgetTree.h"

UDEPRECATEDEMRNightShiftDebugWidget::UDEPRECATEDEMRNightShiftDebugWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UDEPRECATEDEMRNightShiftDebugWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshAccumulator = 0.f;
    EnsureWidgetHierarchy();
    RefreshDebugInfo();
}

void UDEPRECATEDEMRNightShiftDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    RefreshAccumulator += InDeltaTime;
    if (RefreshAccumulator >= RefreshInterval)
    {
        RefreshAccumulator = 0.f;
        RefreshDebugInfo();
    }
}

void UDEPRECATEDEMRNightShiftDebugWidget::RefreshDebugInfo()
{
    TStringBuilder<2048> Builder;

    const UWorld* World = GetWorld();
    const AEMRNightShiftGameState* NightShiftGameState = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
    const UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World ? World->GetSubsystem<UEMRNightShiftSpawnSubsystem>() : nullptr;
    const UEMRPatientPoolSubsystem* PoolSubsystem = World ? World->GetSubsystem<UEMRPatientPoolSubsystem>() : nullptr;

    AppendNightShiftSection(Builder, NightShiftGameState);
    Builder.Append(TEXT("\n"));
    AppendSpawnSubsystemSection(Builder, NightShiftGameState, SpawnSubsystem);
    Builder.Append(TEXT("\n"));
    AppendPatientPoolSection(Builder, NightShiftGameState, PoolSubsystem);

    if (DebugTextBlock)
    {
        DebugTextBlock->SetText(FText::FromString(Builder.ToString()));
    }
}

void UDEPRECATEDEMRNightShiftDebugWidget::EnsureWidgetHierarchy()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!DebugTextBlock)
    {
        DebugTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        DebugTextBlock->SetAutoWrapText(true);
    }

    if (!ScrollBoxContainer)
    {
        ScrollBoxContainer = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
        ScrollBoxContainer->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
        ScrollBoxContainer->AddChild(DebugTextBlock);
    }

    if (!WidgetTree->RootWidget)
    {
        WidgetTree->RootWidget = ScrollBoxContainer;
    }
}

void UDEPRECATEDEMRNightShiftDebugWidget::AppendNightShiftSection(FStringBuilderBase& Builder, const AEMRNightShiftGameState* NightShiftGameState) const
{
    Builder.Append(TEXT("=== NightShift ===\n"));

    if (!NightShiftGameState)
    {
        Builder.Append(TEXT("No NightShiftGameState available.\n"));
        return;
    }

    Builder.Appendf(TEXT("RunPhase: %d | RemainingTime: %.2fs\n"), static_cast<int32>(NightShiftGameState->GetRunPhase()), NightShiftGameState->GetRemainingTimeInNightShift());

    if (const UEMRNightShiftDefinition* Definition = NightShiftGameState->GetCurrentNightShiftDefinition())
    {
        Builder.Appendf(TEXT("Definition: %s (%s)\n"), *Definition->GetName(), *Definition->GetEffectiveNightShiftId().ToString());
        float DurationSeconds = 600.f;
        if (const UWorld* World = GetWorld())
        {
            if (const UGameInstance* GameInstance = World->GetGameInstance())
            {
                if (const UEMRRunRulesSubsystem* RunRulesSubsystem = GameInstance->GetSubsystem<UEMRRunRulesSubsystem>())
                {
                    DurationSeconds = RunRulesSubsystem->GetNightShiftDurationSeconds();
                }
            }
        }
        DurationSeconds = FMath::Max(DurationSeconds, 1.0f);

        Builder.Appendf(TEXT("Difficulty: %s | Revenue: %s | Duration: %.0fs\n"),
            *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(Definition->DifficultyTier),
            *StaticEnum<EEMRNightShiftRevenuePotential>()->GetValueAsString(Definition->RevenuePotential),
            DurationSeconds);
        Builder.Appendf(TEXT("Tags: %s\n"), *Definition->NightShiftTags.ToStringSimple());
        Builder.Appendf(TEXT("HospitalLevel: %s | ReceptionLayout: %s\n"), *Definition->HospitalLevel.ToString(), *Definition->ReceptionLayoutTag.ToString());
    }
    else
    {
        Builder.Append(TEXT("No active NightShift definition.\n"));
    }
}

void UDEPRECATEDEMRNightShiftDebugWidget::AppendSpawnSubsystemSection(FStringBuilderBase& Builder, const AEMRNightShiftGameState* NightShiftGameState, const UEMRNightShiftSpawnSubsystem* SpawnSubsystem) const
{
    Builder.Append(TEXT("=== Spawn Subsystem ===\n"));

    FEMRNightShiftSpawnDebugInfo DebugInfo;
    bool bHasInfo = false;

    if (NightShiftGameState && NightShiftGameState->HasReplicatedSpawnDebugInfo())
    {
        DebugInfo = NightShiftGameState->GetReplicatedSpawnDebugInfo();
        bHasInfo = true;
    }
    else if (SpawnSubsystem)
    {
        DebugInfo = SpawnSubsystem->GetDebugInfo();
        bHasInfo = true;
    }

    if (!bHasInfo)
    {
        Builder.Append(TEXT("No debug info available.\n"));
        return;
    }

    Builder.Appendf(TEXT("Spawning: %s | Active: %d | Pending: %d | SeqCounter: %d\n"),
        DebugInfo.bSpawningEnabled ? TEXT("ON") : TEXT("OFF"),
        DebugInfo.ActivePatientCount,
        DebugInfo.PendingRequestCount,
        DebugInfo.SpawnSequenceCounter);

    Builder.Appendf(TEXT("Elapsed: %.2fs | BaseAcc: %.2f | Immediate: %s\n"),
        DebugInfo.ElapsedSeconds,
        DebugInfo.BaseAccumulator,
        DebugInfo.bImmediateRequestPending ? TEXT("Yes") : TEXT("No"));

    Builder.Appendf(TEXT("BaseRate: %.2f/s | DifficultySpawnMult: %.2f | PlayerCountMult: %.2f\n"),
        DebugInfo.BaseSpawnRatePerSecond,
        DebugInfo.DifficultySpawnRateMultiplier,
        DebugInfo.PlayerCountMultiplier);

    Builder.Appendf(TEXT("DeferredAuto: %d | OldestDeferred: %.2fs | WarmReady: %d\n"),
        DebugInfo.DeferredAutomaticSpawnCount,
        DebugInfo.OldestDeferredAutomaticSpawnSeconds,
        DebugInfo.WarmReadyPatientCount);

    if (DebugInfo.ActiveProfile.SpawnModifiers.Num() > 0)
    {
        Builder.Append(TEXT("Modifiers:\n"));
        for (const FEMRNightShiftSpawnModifierDebug& ModifierDebug : DebugInfo.Modifiers)
        {
            Builder.Appendf(TEXT(" - %s | Enabled=%s | Curve=%s | Acc=%.2f\n"),
                *ModifierDebug.ModifierDefinition.ModifierTag.ToString(),
                ModifierDebug.bEnabled ? TEXT("Yes") : TEXT("No"),
                *ModifierDebug.ModifierDefinition.SpawnRateCurve.ToString(),
                ModifierDebug.Accumulator);
        }
    }
    else
    {
        Builder.Append(TEXT("Modifiers: none\n"));
    }

    if (DebugInfo.PendingRequests.Num() > 0)
    {
        Builder.Append(TEXT("Pending Requests (heap order):\n"));
        for (const FEMRNightShiftSpawnRequest& Request : DebugInfo.PendingRequests)
        {
            Builder.Appendf(TEXT(" - Seq=%d | Time=%.2f | Difficulty=%s | Tags=%s\n"),
                Request.SequenceId,
                Request.RequestTimestamp,
                *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(Request.RequestedDifficulty),
                *Request.SourceTags.ToStringSimple());
        }
    }
    else
    {
        Builder.Append(TEXT("Pending Requests: none\n"));
    }
}

void UDEPRECATEDEMRNightShiftDebugWidget::AppendPatientPoolSection(FStringBuilderBase& Builder, const AEMRNightShiftGameState* NightShiftGameState, const UEMRPatientPoolSubsystem* PoolSubsystem) const
{
    Builder.Append(TEXT("=== Patient Pool ===\n"));

    FEMRPatientPoolDebugInfo DebugInfo;
    bool bHasInfo = false;

    if (NightShiftGameState && NightShiftGameState->HasReplicatedPatientPoolDebugInfo())
    {
        DebugInfo = NightShiftGameState->GetReplicatedPatientPoolDebugInfo();
        bHasInfo = true;
    }
    else if (PoolSubsystem)
    {
        DebugInfo = PoolSubsystem->GetDebugInfo();
        bHasInfo = true;
    }

    if (!bHasInfo)
    {
        Builder.Append(TEXT("No debug info available.\n"));
        return;
    }

    Builder.Appendf(TEXT("TargetSize: %d | Inactive: %d | ActiveTracked: %d\n"), DebugInfo.TargetPoolSize, DebugInfo.InactivePatients, DebugInfo.ActiveTrackedPatients);
    Builder.Appendf(TEXT("CachedPatientData: %d | PatientClass: %s\n"), DebugInfo.CachedPatientData, *DebugInfo.PatientClassName);
    Builder.Appendf(TEXT("NightShift: %s\n"), *DebugInfo.CurrentNightShiftName);
}
