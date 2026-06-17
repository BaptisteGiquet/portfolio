using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Foundation.Random;
using TheLastMage.Gameplay.AoE;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.StatusEffects;

namespace TheLastMage.Gameplay.Items;

public sealed class ProcSystem : IGameSystem
{
    private const int DefaultPerFrameProcBudget = 128;
    private const int DefaultRecursionLimit = 8;
    private const int MaxRememberedDeathSources = 64;
    private const string FaultyFocusEffectType = "faulty_focus_split_spell";
    private const int FaultyFocusChildCount = 3;
    private const int DefaultFaultyFocusMaxGenerations = 4;
    private const float DefaultFaultyFocusDamageMultiplier = 0.5f;
    private const float MinimumFaultyFocusDamageMultiplier = 0.00390625f;

    private readonly Dictionary<EntityId, ContentId> _deathSources = new();
    private int _frameProcCount;
    private int _procExecutionDepth;
    private RunContext? _context;
    private SubscriptionToken _damageToken;
    private SubscriptionToken _spellCastToken;
    private SubscriptionToken _enemyDiedToken;
    private SubscriptionToken _corpseToken;

    public int PerFrameProcBudget { get; set; } = DefaultPerFrameProcBudget;

    public int RecursionLimit { get; set; } = DefaultRecursionLimit;

    public void Initialize(RunContext context)
    {
        _context = context;
        _damageToken = context.Events.Subscribe<DamageAppliedEvent>(OnDamageApplied);
        _spellCastToken = context.Events.Subscribe<SpellCastEvent>(OnSpellCast);
        _enemyDiedToken = context.Events.Subscribe<EnemyDiedEvent>(OnEnemyDied);
        _corpseToken = context.Events.Subscribe<CorpseAvailableEvent>(OnCorpseAvailable);
    }

    public void Tick(float delta)
    {
    }

    public void FixedTick(float delta)
    {
        _frameProcCount = 0;
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_damageToken);
            _context.Events.Unsubscribe(_spellCastToken);
            _context.Events.Unsubscribe(_enemyDiedToken);
            _context.Events.Unsubscribe(_corpseToken);
        }

        _deathSources.Clear();
        _context = null;
    }

    public bool TryConsumeProcBudget(int recursionDepth, out string reason)
    {
        if (recursionDepth > RecursionLimit)
        {
            reason = "Proc recursion limit reached.";
            return false;
        }

        if (_frameProcCount >= PerFrameProcBudget)
        {
            reason = "Per-frame proc budget exhausted.";
            return false;
        }

        _frameProcCount++;
        if (_context != null)
        {
            _context.State.DebugMetrics.ProcBudgetUsedThisFrame = _frameProcCount;
        }

        reason = string.Empty;
        return true;
    }

    private void OnDamageApplied(DamageAppliedEvent gameEvent)
    {
        if (_context == null)
        {
            return;
        }

        if (gameEvent.TargetId.Equals(_context.State.Player.EntityId))
        {
            TriggerMatchingProcs(
                nameof(ProcTrigger.OnDamageTaken),
                gameEvent.SourceId,
                gameEvent.SourceTags,
                gameEvent.TargetId,
                _context.State.Player.Position,
                0);
            return;
        }

        if (IsEnemyTarget(gameEvent.TargetId, out var position))
        {
            if (IsItemSource(gameEvent.SourceId))
            {
                TriggerFireImmolateProcsFromItemDamage(gameEvent, position);
                return;
            }

            TriggerFaultyFocus(gameEvent, position);
            TriggerMatchingProcs(
                nameof(ProcTrigger.OnHit),
                gameEvent.SourceId,
                gameEvent.SourceTags,
                gameEvent.TargetId,
                position,
                0);
        }
    }

    private void TriggerFireImmolateProcsFromItemDamage(DamageAppliedEvent gameEvent, Vector3 position)
    {
        if (_context == null
            || gameEvent.Amount <= 0f
            || (_context.State.Build.Items.Count == 0 && _context.State.Build.ActivatableItem == null))
        {
            return;
        }

        var sourceTags = gameEvent.SourceTags ?? BuildSourceTags(gameEvent.SourceId);
        foreach (var (itemId, count) in EnumerateProcSources(_context.State.Build))
        {
            if (itemId.Equals(gameEvent.SourceId))
            {
                continue;
            }

            var item = _context.Content.GetItem(itemId);
            foreach (var proc in item.Procs)
            {
                if (!string.Equals(proc.Trigger, nameof(ProcTrigger.OnHit), StringComparison.OrdinalIgnoreCase)
                    || (!string.Equals(proc.EffectType, "apply_burn", StringComparison.OrdinalIgnoreCase)
                        && !string.Equals(proc.EffectType, "apply_immolate", StringComparison.OrdinalIgnoreCase))
                    || !TargetMatches(proc, sourceTags))
                {
                    continue;
                }

                for (var stackIndex = 0; stackIndex < count; stackIndex++)
                {
                    TryTriggerProc(proc, gameEvent.TargetId, position, Math.Max(0, _procExecutionDepth));
                }
            }
        }
    }

    private void TriggerFaultyFocus(DamageAppliedEvent gameEvent, Vector3 targetPosition)
    {
        if (_context == null
            || gameEvent.Amount <= 0f
            || !_context.Content.TryGetSpell(gameEvent.SourceId, out var spell)
            || spell == null
            || (_context.State.Build.Items.Count == 0 && _context.State.Build.ActivatableItem == null))
        {
            return;
        }

        var sourceTags = gameEvent.SourceTags ?? BuildSourceTags(gameEvent.SourceId);
        foreach (var (itemId, count) in EnumerateProcSources(_context.State.Build))
        {
            var item = _context.Content.GetItem(itemId);
            for (var effectIndex = 0; effectIndex < item.Effects.Count; effectIndex++)
            {
                var effect = item.Effects[effectIndex];
                if (!string.Equals(effect.Kind, nameof(ItemEffectKind.FaultyFocus), StringComparison.Ordinal)
                    || !GameplayTagPath.MatchesAny(effect.TargetTag, sourceTags))
                {
                    continue;
                }

                for (var stackIndex = 0; stackIndex < count; stackIndex++)
                {
                    TryTriggerFaultyFocus(itemId, effect, gameEvent, targetPosition);
                }
            }
        }
    }

    private void TryTriggerFaultyFocus(
        ContentId itemId,
        ItemEffectRuntimeSpec effect,
        DamageAppliedEvent gameEvent,
        Vector3 targetPosition)
    {
        if (_context == null)
        {
            return;
        }

        var maxGenerations = Math.Clamp(
            (int)MathF.Floor(effect.SecondaryValue <= 0f ? DefaultFaultyFocusMaxGenerations : effect.SecondaryValue),
            1,
            RecursionLimit);
        var nextGeneration = Math.Max(0, gameEvent.SpellChainGeneration) + 1;
        var damageMultiplier = effect.Value <= 0f ? DefaultFaultyFocusDamageMultiplier : effect.Value;
        var childDamageMultiplier = MathF.Max(0f, gameEvent.DamageMultiplier) * damageMultiplier;
        var syntheticProc = new ProcRuntimeSpec(
            itemId,
            nameof(ProcTrigger.OnHit),
            FaultyFocusEffectType,
            effect.Chance <= 0f ? 1f : effect.Chance,
            damageMultiplier,
            effect.TargetTag);

        if (nextGeneration > maxGenerations)
        {
            RecordBlocked(syntheticProc, $"Faulty Focus generation cap reached ({maxGenerations}).");
            return;
        }

        if (childDamageMultiplier < MinimumFaultyFocusDamageMultiplier)
        {
            RecordBlocked(syntheticProc, "Faulty Focus damage multiplier floor reached.");
            return;
        }

        if (!TryConsumeProcBudget(nextGeneration, out var reason))
        {
            RecordBlocked(syntheticProc, reason);
            return;
        }

        var random = _context.Random.Stream(RandomStreamId.Combat);
        if (random.NextDouble() > syntheticProc.Chance)
        {
            return;
        }

        var baseDirection = targetPosition - _context.State.Player.Position;
        baseDirection.Y = 0f;
        if (baseDirection.LengthSquared() <= 0.0001f)
        {
            baseDirection = Vector3.Forward;
        }

        _context.State.DebugMetrics.LastProcSummary =
            $"{itemId}:OnHit->{FaultyFocusEffectType} spell={gameEvent.SourceId} gen={nextGeneration} x{childDamageMultiplier:0.###}";
        _context.Events.Publish(new ItemProcTriggeredEvent(itemId, nameof(ProcTrigger.OnHit), FaultyFocusEffectType, gameEvent.TargetId));
        _context.GetSystem<SpellSystem>().CastChildSpells(
            gameEvent.SourceId,
            _context.State.Player.EntityId,
            targetPosition + new Vector3(0f, 0.2f, 0f),
            baseDirection,
            FaultyFocusChildCount,
            childDamageMultiplier,
            nextGeneration);
    }

    private void OnSpellCast(SpellCastEvent gameEvent)
    {
        if (_context == null || !gameEvent.CasterId.Equals(_context.State.Player.EntityId))
        {
            return;
        }

        TriggerMatchingProcs(
            nameof(ProcTrigger.OnCast),
            gameEvent.SpellId,
            null,
            EntityId.None,
            _context.State.Player.Position,
            0);
    }

    private void OnEnemyDied(EnemyDiedEvent gameEvent)
    {
        if (_deathSources.Count >= MaxRememberedDeathSources)
        {
            _deathSources.Clear();
        }

        _deathSources[gameEvent.EnemyId] = gameEvent.SourceId;
    }

    private void OnCorpseAvailable(CorpseAvailableEvent gameEvent)
    {
        if (_context == null)
        {
            return;
        }

        var sourceId = _deathSources.TryGetValue(gameEvent.EnemyId, out var rememberedSourceId)
            ? rememberedSourceId
            : ContentId.From(string.Empty);
        _deathSources.Remove(gameEvent.EnemyId);

        TriggerMatchingProcs(
            nameof(ProcTrigger.OnKill),
            sourceId,
            null,
            gameEvent.EnemyId,
            gameEvent.Position,
            0);
    }

    private void TriggerMatchingProcs(
        string trigger,
        ContentId sourceId,
        IReadOnlyList<TagId>? sourceTagsOverride,
        EntityId targetId,
        Vector3 position,
        int recursionDepth)
    {
        if (_context == null
            || (_context.State.Build.Items.Count == 0 && _context.State.Build.ActivatableItem == null))
        {
            return;
        }

        var sourceTags = sourceTagsOverride ?? BuildSourceTags(sourceId);
        foreach (var (itemId, count) in EnumerateProcSources(_context.State.Build))
        {
            var item = _context.Content.GetItem(itemId);
            foreach (var proc in item.Procs)
            {
                if (!string.Equals(proc.Trigger, trigger, StringComparison.OrdinalIgnoreCase)
                    || !TargetMatches(proc, sourceTags))
                {
                    continue;
                }

                for (var stackIndex = 0; stackIndex < count; stackIndex++)
                {
                    TryTriggerProc(proc, targetId, position, Math.Max(recursionDepth, _procExecutionDepth));
                }
            }
        }
    }

    private static IEnumerable<(ContentId ItemId, int Count)> EnumerateProcSources(BuildState build)
    {
        foreach (var stack in build.Items)
        {
            yield return (stack.ItemId, stack.Count);
        }

        if (build.ActivatableItem != null)
        {
            yield return (build.ActivatableItem.ItemId, 1);
        }
    }

    private void TryTriggerProc(ProcRuntimeSpec proc, EntityId targetId, Vector3 position, int recursionDepth)
    {
        if (_context == null)
        {
            return;
        }

        if (!TryConsumeProcBudget(recursionDepth, out var reason))
        {
            RecordBlocked(proc, reason);
            return;
        }

        var random = _context.Random.Stream(RandomStreamId.Combat);
        if (random.NextDouble() > proc.Chance)
        {
            return;
        }

        _context.State.DebugMetrics.LastProcSummary =
            $"{proc.SourceId}:{proc.Trigger}->{proc.EffectType} target={targetId}";
        _context.Events.Publish(new ItemProcTriggeredEvent(proc.SourceId, proc.Trigger, proc.EffectType, targetId));
        _procExecutionDepth++;
        try
        {
            ExecuteProcEffect(proc, targetId, position);
        }
        finally
        {
            _procExecutionDepth--;
        }
    }

    private void ExecuteProcEffect(ProcRuntimeSpec proc, EntityId targetId, Vector3 position)
    {
        if (_context == null)
        {
            return;
        }

        switch (proc.EffectType)
        {
            case "apply_burn":
                ApplyStatus(proc, targetId, StatusEffectRules.Burn);
                break;
            case "apply_immolate":
                ApplyStatus(proc, targetId, StatusEffectRules.Immolate);
                break;
            case "apply_slow":
                ApplyStatus(proc, targetId, StatusEffectRules.Slow);
                break;
            case "apply_freeze":
                ApplyStatus(proc, targetId, StatusEffectRules.Freeze);
                break;
            case "bonus_damage":
            case "shatter_damage":
            case "chain_damage":
                if (targetId.IsValid)
                {
                    ApplyBonusDamage(proc, targetId, position);
                }

                break;
            case "kill_explosion":
            case "burning_ground":
            case "explosive_seal":
                SpawnProcArea(proc, position);
                break;
            case "cooldown_tick":
                _context.State.Spellbook.TickCooldowns(MathF.Max(0f, proc.Value));
                break;
        }
    }

    private void ApplyStatus(ProcRuntimeSpec proc, EntityId targetId, string statusKind)
    {
        if (_context == null
            || !targetId.IsValid
            || !_context.TryGetSystem<StatusEffectSystem>(out var statusSystem)
            || statusSystem == null)
        {
            return;
        }

        statusSystem.Apply(
            targetId,
            statusKind,
            proc.SourceId,
            _context.State.Player.EntityId,
            baseDurationOverrideSeconds: proc.Value);
    }

    private void ApplyBonusDamage(ProcRuntimeSpec proc, EntityId targetId, Vector3 position)
    {
        if (_context == null)
        {
            return;
        }

        var damageType = ResolveDamageType(proc.EffectType);
        _context.GetSystem<CombatSystem>().ApplyDamage(new DamageRequest(
            _context.State.Player.EntityId,
            targetId,
            proc.SourceId,
            MathF.Max(0f, proc.Value),
            damageType,
            new HitContext(position, Vector3.Up, _context.State.Player.EntityId)));
    }

    private void SpawnProcArea(ProcRuntimeSpec proc, Vector3 position)
    {
        if (_context == null || !_context.TryGetSystem<AoESystem>(out var aoeSystem) || aoeSystem == null)
        {
            return;
        }

        var damageType = ResolveDamageType(proc.EffectType);
        var isGround = string.Equals(proc.EffectType, "burning_ground", StringComparison.OrdinalIgnoreCase);
        var radius = string.Equals(proc.EffectType, "explosive_seal", StringComparison.OrdinalIgnoreCase) ? 3.2f : 2.4f;
        var duration = isGround ? 2.5f : 0.08f;
        var tick = isGround ? 0.5f : 0.05f;
        aoeSystem.Spawn(
            proc.SourceId,
            _context.State.Player.EntityId,
            position,
            Vector3.Zero,
            radius,
            duration,
            tick,
            MathF.Max(0f, proc.Value),
            damageType,
            string.Empty);
    }

    private void RecordBlocked(ProcRuntimeSpec proc, string reason)
    {
        if (_context == null)
        {
            return;
        }

        _context.State.DebugMetrics.ProcBudgetBlockedThisFrame++;
        _context.State.DebugMetrics.LastProcSummary =
            $"{proc.SourceId}:{proc.Trigger}->{proc.EffectType} blocked: {reason}";
        _context.Events.Publish(new ItemProcBlockedEvent(proc.SourceId, proc.Trigger, proc.EffectType, reason));
    }

    private bool IsEnemyTarget(EntityId entityId, out Vector3 position)
    {
        position = Vector3.Zero;
        if (_context == null || !_context.TryGetSystem<EnemySystem>(out var enemySystem) || enemySystem == null)
        {
            return false;
        }

        foreach (var enemy in enemySystem.ActiveEnemies)
        {
            if (enemy.EntityId.Equals(entityId))
            {
                position = enemy.Position;
                return true;
            }
        }

        return false;
    }

    private bool IsItemSource(ContentId sourceId)
    {
        return _context?.Content.Items.ContainsKey(sourceId) == true;
    }

    private IReadOnlyList<TagId> BuildSourceTags(ContentId sourceId)
    {
        var tags = new List<TagId>(8);
        if (_context?.Content.TryGetSpell(sourceId, out var spell) == true && spell != null)
        {
            tags.AddRange(spell.Tags);
        }
        else if (_context?.Content.TryGetDefense(sourceId, out var defense) == true && defense != null)
        {
            tags.Add(TagId.From("combat.source.defense"));
            tags.AddRange(defense.Tags);
        }
        else if (_context?.Content.Items.TryGetValue(sourceId, out var item) == true)
        {
            tags.AddRange(item.Tags);
        }

        return GameplayTagSet.From(tags);
    }

    private static bool TargetMatches(ProcRuntimeSpec proc, IReadOnlyList<TagId> sourceTags)
    {
        return GameplayTagPath.MatchesAny(proc.TargetTag, sourceTags);
    }

    private static DamageType ResolveDamageType(string effectType)
    {
        if (string.Equals(effectType, "apply_burn", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effectType, "apply_immolate", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effectType, "burning_ground", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effectType, "kill_explosion", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effectType, "explosive_seal", StringComparison.OrdinalIgnoreCase))
        {
            return DamageType.Fire;
        }

        if (string.Equals(effectType, "apply_freeze", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effectType, "apply_slow", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effectType, "shatter_damage", StringComparison.OrdinalIgnoreCase))
        {
            return DamageType.Frost;
        }

        if (string.Equals(effectType, "chain_damage", StringComparison.OrdinalIgnoreCase))
        {
            return DamageType.Arcane;
        }

        return DamageType.Physical;
    }
}
