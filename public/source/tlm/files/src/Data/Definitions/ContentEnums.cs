namespace TheLastMage.Data.Definitions;

public enum ItemKind
{
    Passive,
    Activatable
}

public enum ItemEffectKind
{
    AddMultiplyStat = 1,
    AddProjectilePierce = 2,
    FreezeWithFrostDamage = 3,
    ImmolateWithFireDamage = 4,
    EnemiesExplodeOnDeath = 5,
    BurningGroundOnEnemyDeath = 6,
    AddFlatStat = 7,
    RemoveMultiplyStat = 8,
    RemoveFlatStat = 9,
    AddSpellCount = 15,
    SplitProjectile = 16,
    KeepItIn = 17,
    AbyssalRing = 18,
    FaultyFocus = 19,
    PreventNextPlayerHealthLoss = 20,
    HomingSpells = 21,
    OrbitingSpells = 22,
    PersistentSummons = 23
}

public enum DataModifierOp
{
    FlatAdd,
    AdditivePercent,
    Multiplicative,
    MinClamp,
    MaxClamp,
    Override
}

public enum ProcTrigger
{
    None,
    OnKill,
    OnCast,
    OnHit,
    OnDamageTaken
}

public enum EnemyFamily
{
    Human,
    Undead,
    Beast,
    SiegeMachine,
    Other
}

public enum EnemyRole
{
    Grunt,
    Soldier,
    Ranged,
    Brute,
    Specialist,
    Boss
}

public enum DefensePlacementRule
{
    None,
    Ground,
    TowerEntrance,
    Wall,
    ExplicitSocket
}

public enum DurationScalingMode
{
    None,
    PlayerDuration
}
