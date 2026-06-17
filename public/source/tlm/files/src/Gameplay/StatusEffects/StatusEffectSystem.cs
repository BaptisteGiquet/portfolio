using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.StatusEffects;

public sealed class StatusEffectSystem : IGameSystem
{
    private readonly List<StatusEffectState> _effects = new();
    private RunContext? _context;

    public int ActiveCount => _effects.Count;

    public IReadOnlyList<StatusEffectState> ActiveEffects => _effects;

    public void Initialize(RunContext context)
    {
        _context = context;
    }

    public void Tick(float delta)
    {
        if (_context != null)
        {
            _context.State.DebugMetrics.ActiveStatuses = _effects.Count;
        }
    }

    public void FixedTick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var combat = _context.GetSystem<CombatSystem>();
        for (var i = _effects.Count - 1; i >= 0; i--)
        {
            var effect = _effects[i];
            if (!combat.IsAlive(effect.TargetId))
            {
                _effects.RemoveAt(i);
                continue;
            }

            effect.RemainingSeconds -= delta;
            if (effect.RemainingSeconds <= 0f)
            {
                _effects.RemoveAt(i);
                continue;
            }

            var rule = StatusEffectRules.GetOrDefault(effect.Kind);
            if (rule.PeriodicDamageType == null || rule.PeriodicTickSeconds <= 0f)
            {
                continue;
            }

            effect.PeriodicTickRemainingSeconds -= delta;
            while (effect.PeriodicTickRemainingSeconds <= 0f && effect.RemainingSeconds > 0f)
            {
                effect.PeriodicTickRemainingSeconds += rule.PeriodicTickSeconds;
                combat.ApplyDamage(new DamageRequest(
                    effect.SourceEntityId,
                    effect.TargetId,
                    effect.SourceId,
                    MathF.Max(0f, effect.Magnitude * rule.PeriodicTickSeconds),
                    rule.PeriodicDamageType.Value,
                    new HitContext(Godot.Vector3.Zero, Godot.Vector3.Up, effect.SourceEntityId),
                    DamageMultiplier: effect.DamageMultiplier,
                    SpellChainGeneration: effect.SpellChainGeneration));
            }
        }
    }

    public void Shutdown()
    {
        _effects.Clear();
        _context = null;
    }

    public void Apply(
        EntityId targetId,
        string kind,
        ContentId sourceId,
        EntityId sourceEntityId,
        float? baseDurationOverrideSeconds = null,
        float? basePowerOverride = null,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0)
    {
        if (_context == null || !targetId.IsValid || string.IsNullOrWhiteSpace(kind))
        {
            return;
        }

        var normalizedKind = StatusEffectRules.NormalizeKind(kind);
        var rule = StatusEffectRules.GetOrDefault(normalizedKind);
        var tags = BuildStatusTags(normalizedKind, sourceId);
        var basePower = basePowerOverride.HasValue && basePowerOverride.Value > 0f ? basePowerOverride.Value : rule.BasePower;
        var baseDuration = baseDurationOverrideSeconds.HasValue && baseDurationOverrideSeconds.Value > 0f
            ? baseDurationOverrideSeconds.Value
            : rule.BaseDurationSeconds;
        var power = MathF.Max(0f, PlayerAttributeResolver.ResolveValue(_context, PlayerAttributeResolver.StatusPower, basePower, tags));
        var durationSeconds = PlayerAttributeResolver.ResolveDuration(_context, baseDuration, tags);
        if (durationSeconds <= 0f)
        {
            return;
        }

        for (var i = 0; i < _effects.Count; i++)
        {
            var existing = _effects[i];
            if (!existing.TargetId.Equals(targetId) || !string.Equals(existing.Kind, normalizedKind, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            existing.Magnitude = MathF.Max(existing.Magnitude, power);
            if (durationSeconds > existing.RemainingSeconds)
            {
                existing.RemainingSeconds = durationSeconds;
            }

            return;
        }

        _effects.Add(new StatusEffectState
        {
            TargetId = targetId,
            Kind = normalizedKind,
            SourceEntityId = sourceEntityId,
            SourceId = sourceId,
            DamageMultiplier = MathF.Max(0f, damageMultiplier),
            SpellChainGeneration = Math.Max(0, spellChainGeneration),
            Magnitude = power,
            RemainingSeconds = durationSeconds,
            PeriodicTickRemainingSeconds = StatusEffectRules.GetOrDefault(normalizedKind).PeriodicTickSeconds
        });
        _context.Events.Publish(new StatusAppliedEvent(targetId, normalizedKind, power, durationSeconds, sourceId));
    }

    public float GetMoveSpeedMultiplier(EntityId targetId)
    {
        var multiplier = 1f;
        for (var i = 0; i < _effects.Count; i++)
        {
            var effect = _effects[i];
            if (!effect.TargetId.Equals(targetId))
            {
                continue;
            }

            var rule = StatusEffectRules.GetOrDefault(effect.Kind);
            if (string.Equals(effect.Kind, StatusEffectRules.Sleep, StringComparison.OrdinalIgnoreCase))
            {
                multiplier = 0f;
                continue;
            }

            if (string.Equals(effect.Kind, StatusEffectRules.Slow, StringComparison.OrdinalIgnoreCase)
                || string.Equals(effect.Kind, StatusEffectRules.Freeze, StringComparison.OrdinalIgnoreCase))
            {
                multiplier = MathF.Min(multiplier, Math.Clamp(effect.Magnitude, 0.1f, 1f));
            }
        }

        return multiplier;
    }

    public bool BlocksActions(EntityId targetId)
    {
        for (var i = 0; i < _effects.Count; i++)
        {
            var effect = _effects[i];
            if (effect.TargetId.Equals(targetId)
                && string.Equals(effect.Kind, StatusEffectRules.Sleep, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
        }

        return false;
    }

    public bool TryGetPrimaryStatusKind(EntityId targetId, out string statusKind)
    {
        statusKind = string.Empty;
        var priority = 0;
        for (var i = 0; i < _effects.Count; i++)
        {
            var effect = _effects[i];
            if (!effect.TargetId.Equals(targetId))
            {
                continue;
            }

            var candidatePriority = StatusEffectRules.GetOrDefault(effect.Kind).DisplayPriority;
            if (candidatePriority <= priority)
            {
                continue;
            }

            statusKind = effect.Kind;
            priority = candidatePriority;
        }

        return priority > 0;
    }

    private IReadOnlyList<TagId> BuildStatusTags(string statusKind, ContentId sourceId)
    {
        var rule = StatusEffectRules.GetOrDefault(statusKind);
        var tags = new List<TagId>(8)
        {
            TagId.From("status"),
            TagId.From($"status.{statusKind}")
        };

        if (rule.PeriodicDamageType != null)
        {
            tags.Add(TagId.From("combat.damage"));
            tags.Add(TagId.From($"combat.damage.{rule.PeriodicDamageType.Value.ToString().ToLowerInvariant()}"));
        }

        if (string.Equals(statusKind, StatusEffectRules.Slow, StringComparison.OrdinalIgnoreCase)
            || string.Equals(statusKind, StatusEffectRules.Freeze, StringComparison.OrdinalIgnoreCase))
        {
            tags.Add(TagId.From("combat.damage"));
            tags.Add(TagId.From("combat.damage.frost"));
            tags.Add(TagId.From("combat.control"));
            tags.Add(TagId.From($"combat.control.{statusKind}"));
        }

        if (string.Equals(statusKind, StatusEffectRules.Sleep, StringComparison.OrdinalIgnoreCase))
        {
            tags.Add(TagId.From("combat.control"));
            tags.Add(TagId.From("combat.control.sleep"));
        }

        if (_context?.Content.TryGetSpell(sourceId, out var spell) == true && spell != null)
        {
            tags.AddRange(spell.Tags);
        }

        return GameplayTagSet.From(tags);
    }
}
