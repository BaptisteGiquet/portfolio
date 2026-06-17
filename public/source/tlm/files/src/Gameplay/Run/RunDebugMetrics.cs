namespace TheLastMage.Gameplay.Run;

public sealed class RunDebugMetrics
{
    public int ActiveEnemies { get; set; }

    public int ActiveDefenses { get; set; }

    public int ActiveAoEs { get; set; }

    public int ActiveStatuses { get; set; }

    public int ActiveSummons { get; set; }

    public int ActiveProjectiles { get; set; }

    public int DamageEventsThisFrame { get; set; }

    public string LastDamageSummary { get; set; } = "-";

    public int DamagePreventionsThisFrame { get; set; }

    public string LastDamagePreventionSummary { get; set; } = "-";

    public int ProcBudgetUsedThisFrame { get; set; }

    public int ProcBudgetBlockedThisFrame { get; set; }

    public string LastProcSummary { get; set; } = "-";

    public string LastActivatableItemSummary { get; set; } = "-";

    public string LastCastFlowSummary { get; set; } = "-";

    public string LastSpellRadiusSummary { get; set; } = "-";

    public int PathQueriesThisFrame { get; set; }

    public int AttackChecksThisFrame { get; set; }

    public int SeparationChecksThisFrame { get; set; }

    public int SlotAssignmentsThisFrame { get; set; }

    public float EnemyAiMilliseconds { get; set; }

    public float FrameMilliseconds { get; set; }

    public long AllocatedBytesThisFrame { get; set; }

    public int WaveTargetActiveEnemies { get; set; }

    public int WaveSpawnedThisDay { get; set; }

    public string ActiveWaveId { get; set; } = "-";

    public float WaveElapsedSeconds { get; set; }

    public string LastSpawnLaneId { get; set; } = "-";

    public string SpawnLaneSummary { get; set; } = "-";

    public string TowerRouteState { get; set; } = "Unknown";

    public string TowerEntranceState { get; set; } = "Unknown";

    public string TowerRouteSummary { get; set; } = "-";

    public string TowerRouteStageSummary { get; set; } = "-";

    public string LastDefensePlacementValidation { get; set; } = "-";

    public string DefensePreparationSummary { get; set; } = "-";

    public string BenchmarkReportPath { get; set; } = "-";

    public string RunReportPath { get; set; } = "-";

    public string BalanceReportPath { get; set; } = "-";

    public string ReproductionSummary { get; set; } = "-";

    public string ProfilingSummary { get; set; } = "-";

    public string LastMageDamageSummary { get; set; } = "-";

    public string DeathCauseSummary { get; set; } = "-";

    public string LastRunModeSummary { get; set; } = "normal run; player stats counted";

    public string LastRunSummary { get; set; } = "-";

    public int FeedbackRequestsThisFrame { get; set; }

    public int FeedbackBudgetDropsThisFrame { get; set; }

    public string ActiveFeedbackBudgetSummary { get; set; } = "-";

    public string AudioMixSummary { get; set; } = "-";

    public string ReadabilitySummary { get; set; } = "-";

    public void ResetFrameCounters()
    {
        PathQueriesThisFrame = 0;
        AttackChecksThisFrame = 0;
        SeparationChecksThisFrame = 0;
        SlotAssignmentsThisFrame = 0;
        DamageEventsThisFrame = 0;
        DamagePreventionsThisFrame = 0;
        ProcBudgetUsedThisFrame = 0;
        ProcBudgetBlockedThisFrame = 0;
        EnemyAiMilliseconds = 0f;
        AllocatedBytesThisFrame = 0;
        FeedbackRequestsThisFrame = 0;
        FeedbackBudgetDropsThisFrame = 0;
    }
}
