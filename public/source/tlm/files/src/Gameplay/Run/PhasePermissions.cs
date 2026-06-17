namespace TheLastMage.Gameplay.Run;

public readonly record struct PhasePermissions(
    bool CanCastSpells,
    bool CanChooseMarketOffer,
    bool CanRerollOffers,
    bool CanPlaceDefense,
    bool CanRepairDefense,
    bool CanStartNextDay,
    bool EnemySpawningActive)
{
    public static PhasePermissions For(RunPhase phase)
    {
        return phase switch
        {
            RunPhase.RunSetup => new PhasePermissions(false, false, false, false, false, true, false),
            RunPhase.DayCombat => new PhasePermissions(true, false, false, false, false, false, true),
            RunPhase.DayPeak => new PhasePermissions(true, false, false, false, false, false, true),
            RunPhase.DayCleanup => new PhasePermissions(true, false, false, false, false, false, false),
            RunPhase.NightMarket => new PhasePermissions(false, true, true, false, false, false, false),
            RunPhase.NightDefense => new PhasePermissions(false, false, false, true, true, true, false),
            RunPhase.RunVictory => new PhasePermissions(false, false, false, false, false, false, false),
            RunPhase.RunDefeat => new PhasePermissions(false, false, false, false, false, false, false),
            _ => new PhasePermissions(false, false, false, false, false, false, false)
        };
    }
}
