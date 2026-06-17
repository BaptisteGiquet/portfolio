using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.AoE;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Projectiles;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public sealed class SpellSystem : IGameSystem
{
    private static readonly TagId[] AbyssalRingTags =
    {
        TagId.From("spell.delivery.area"),
        TagId.From("combat.damage.arcane")
    };

    private readonly EffectResolver _effectResolver = new(new IEffectExecutor[]
    {
        new SpawnProjectileEffectExecutor(),
        new FrostProjectileEffectExecutor(),
        new HitscanBeamEffectExecutor(),
        new FirewallHazardEffectExecutor(),
        new BlizzardAoEEffectExecutor(),
        new TornadoVortexEffectExecutor(),
        new SummonEffectExecutor()
    });

    private RunContext? _context;

    public void Initialize(RunContext context)
    {
        _context = context;
        ApplyMageLoadout();
    }

    public void Tick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        _context.State.Spellbook.TickCooldowns(delta);
        TickCastFlow(delta);
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        _context = null;
    }

    public bool Cast(EntityId casterId, int slotIndex, Vector3 origin, Vector3 direction)
    {
        if (_context == null || !_context.State.Spellbook.TryGetSlot(slotIndex, out var slot) || slot == null || !slot.HasSpell)
        {
            return false;
        }

        if (!_context.Content.TryGetSpell(slot.SpellId, out var spell) || spell == null)
        {
            GD.PushWarning($"Cannot cast missing spell '{slot.SpellId}'.");
            return false;
        }

        ExecuteCast(casterId, slot, spell, origin, direction, bonusSpellInstances: 0);
        return true;
    }

    public bool TryFireTemporaryWeapon(EntityId casterId, Vector3 origin, Vector3 direction)
    {
        if (_context?.State.TemporaryWeaponMode is not { CanFire: true } weapon)
        {
            return false;
        }

        var tags = new[]
        {
            TagId.From("spell.delivery.projectile"),
            TagId.From("combat.damage.physical")
        };
        var damage = PlayerAttributeResolver.ResolveDamage(_context, 3f, tags);
        const float speed = 58f;
        const float range = 42f;
        const float radius = 0.08f;
        _context.GetSystem<ProjectileSystem>().Spawn(
            weapon.SourceId,
            casterId,
            origin,
            direction.LengthSquared() <= 0.0001f ? Vector3.Forward : direction.Normalized(),
            damage,
            DamageType.Physical,
            radius,
            range / speed,
            speed,
            presentationScale: 0.65f);

        var fireRate = PlayerAttributeResolver.ResolveFireRate(_context, tags, false) * weapon.FireRateMultiplier;
        weapon.StartCooldown(0.16f / MathF.Max(0.05f, fireRate));
        _context.Events.Publish(new SpellCastEvent(weapon.SourceId, casterId));
        return true;
    }

    public int CastChildSpells(
        ContentId spellId,
        EntityId casterId,
        Vector3 origin,
        Vector3 direction,
        int childCount,
        float damageMultiplier,
        int spellChainGeneration)
    {
        if (_context == null
            || !_context.Content.TryGetSpell(spellId, out var spell)
            || spell == null)
        {
            return 0;
        }

        var count = Math.Clamp(childCount, 1, 12);
        var normalizedDirection = direction.LengthSquared() <= 0.0001f ? Vector3.Forward : direction.Normalized();
        for (var repeatIndex = 0; repeatIndex < count; repeatIndex++)
        {
            var executionContext = BuildRepeatContext(
                _context,
                spell,
                casterId,
                origin,
                normalizedDirection,
                repeatIndex,
                count,
                MathF.Max(0f, damageMultiplier),
                Math.Max(0, spellChainGeneration),
                origin);
            _effectResolver.Execute(executionContext, spell.Effects);
        }

        return count;
    }

    public bool BeginCastFlow(EntityId casterId, int slotIndex)
    {
        if (_context == null
            || _context.State.Spellbook.CastFlow.IsActive
            || !_context.State.Spellbook.TryGetSlot(slotIndex, out var slot)
            || slot == null
            || !slot.HasSpell
            || !slot.IsReady
            || !_context.Content.TryGetSpell(slot.SpellId, out var spell)
            || spell == null)
        {
            return false;
        }

        if (!ItemBehaviorResolver.TryCreateCastFlowPlan(_context, spell, out var plan) || !plan.RequiresCharge)
        {
            return false;
        }

        _context.State.Spellbook.CastFlow.Begin(casterId, slotIndex, spell.Id, plan);
        _context.State.DebugMetrics.LastCastFlowSummary =
            plan.ReleaseKind == CastFlowReleaseKind.ExpandingRing
                ? $"charging {spell.Id} via {plan.SourceItemId} {plan.RequiredChargeSeconds:0.##}s ring x{plan.DamageMultiplier:0.##}"
                : $"charging {spell.Id} via {plan.SourceItemId} {plan.RequiredChargeSeconds:0.##}s +{plan.BonusSpellInstances}";
        _context.Events.Publish(new SpellChargeStartedEvent(spell.Id, casterId, plan.SourceItemId, plan.RequiredChargeSeconds));
        return true;
    }

    public bool CanBeginCastFlow(int slotIndex)
    {
        return _context != null
            && !_context.State.Spellbook.CastFlow.IsActive
            && _context.State.Spellbook.TryGetSlot(slotIndex, out var slot)
            && slot != null
            && slot.HasSpell
            && slot.IsReady
            && _context.Content.TryGetSpell(slot.SpellId, out var spell)
            && spell != null
            && ItemBehaviorResolver.TryCreateCastFlowPlan(_context, spell, out var plan)
            && plan.RequiresCharge;
    }

    public bool ReleaseCastFlow(Vector3 origin, Vector3 direction)
    {
        if (_context == null)
        {
            return false;
        }

        var castFlow = _context.State.Spellbook.CastFlow;
        if (!castFlow.IsActive)
        {
            return false;
        }

        var spellId = castFlow.SpellId;
        var casterId = castFlow.CasterId;
        var sourceItemId = castFlow.SourceItemId;
        var elapsed = castFlow.ChargeElapsedSeconds;
        var bonusSpellInstances = castFlow.BonusSpellInstances;
        var slotIndex = castFlow.SlotIndex;
        var plan = castFlow.Plan;

        if (!castFlow.IsReady)
        {
            castFlow.Clear();
            _context.State.DebugMetrics.LastCastFlowSummary =
                $"canceled {spellId} via {sourceItemId} at {elapsed:0.##}s incomplete";
            _context.Events.Publish(new SpellChargeCanceledEvent(spellId, casterId, sourceItemId, elapsed, "incomplete"));
            return false;
        }

        if (!_context.State.Spellbook.TryGetSlot(slotIndex, out var slot)
            || slot == null
            || !slot.HasSpell
            || !slot.SpellId.Equals(spellId)
            || !_context.Content.TryGetSpell(spellId, out var spell)
            || spell == null)
        {
            castFlow.Clear();
            _context.State.DebugMetrics.LastCastFlowSummary =
                $"canceled {spellId} via {sourceItemId}: spell changed";
            _context.Events.Publish(new SpellChargeCanceledEvent(spellId, casterId, sourceItemId, elapsed, "spell_changed"));
            return false;
        }

        var spellInstanceCount = ExecuteCastFlowRelease(casterId, slot, spell, origin, direction, plan, bonusSpellInstances);
        castFlow.Clear();
        _context.State.DebugMetrics.LastCastFlowSummary =
            plan.ReleaseKind == CastFlowReleaseKind.ExpandingRing
                ? $"released {spellId} via {sourceItemId} ring damage=x{plan.DamageMultiplier:0.##}"
                : $"released {spellId} via {sourceItemId} instances={spellInstanceCount}";
        _context.Events.Publish(new SpellChargeReleasedEvent(spellId, casterId, sourceItemId, spellInstanceCount));
        return true;
    }

    private int ExecuteCastFlowRelease(
        EntityId casterId,
        SpellSlotState slot,
        SpellRuntimeDefinition spell,
        Vector3 origin,
        Vector3 direction,
        CastFlowPlan plan,
        int bonusSpellInstances)
    {
        if (_context == null)
        {
            return 0;
        }

        if (plan.ReleaseKind == CastFlowReleaseKind.ExpandingRing)
        {
            SpawnExpandingRing(casterId, spell, plan);
            return 0;
        }

        return ExecuteCast(casterId, slot, spell, origin, direction, bonusSpellInstances);
    }

    private void SpawnExpandingRing(EntityId casterId, SpellRuntimeDefinition spell, CastFlowPlan plan)
    {
        if (_context == null)
        {
            return;
        }

        var ringTags = AbyssalRingTags;
        _context.State.ClearStatBreakdowns();
        var playerDamage = PlayerAttributeResolver.ResolveDamage(_context, 10f, ringTags);
        var damage = playerDamage * MathF.Max(0.1f, plan.DamageMultiplier);
        var endRadius = PlayerAttributeResolver.ResolveAreaRadius(_context, MathF.Max(0.1f, plan.BaseRadius), ringTags);
        var duration = PlayerAttributeResolver.ResolveDuration(_context, MathF.Max(0.05f, plan.BaseDurationSeconds), ringTags);
        var startRadius = MathF.Min(MathF.Max(0.05f, plan.StartRadius), endRadius);
        _context.GetSystem<AoESystem>().SpawnExpandingRing(
            plan.SourceItemId,
            casterId,
            _context.State.Player.Position,
            startRadius,
            endRadius,
            duration,
            MathF.Max(0.02f, plan.TickIntervalSeconds),
            MathF.Max(0.1f, plan.RingThickness),
            damage,
            DamageType.Arcane);
    }

    private int ExecuteCast(
        EntityId casterId,
        SpellSlotState slot,
        SpellRuntimeDefinition spell,
        Vector3 origin,
        Vector3 direction,
        int bonusSpellInstances)
    {
        if (_context == null)
        {
            return 0;
        }

        _context.State.ClearStatBreakdowns();
        var additionalSpellCount = PlayerAttributeResolver.ResolveAdditionalSpellCount(_context, spell.Tags);
        var spellCount = 1 + additionalSpellCount + Math.Clamp(bonusSpellInstances, 0, 12);
        for (var repeatIndex = 0; repeatIndex < spellCount; repeatIndex++)
        {
            var executionContext = BuildRepeatContext(_context, spell, casterId, origin, direction, repeatIndex, spellCount);
            _effectResolver.Execute(executionContext, spell.Effects);
        }

        var fireRate = PlayerAttributeResolver.ResolveFireRate(_context, spell.Tags);
        var cooldown = spell.CooldownSeconds / fireRate;
        slot.StartCooldown(MathF.Max(0.05f, cooldown));
        _context.Events.Publish(new SpellCastEvent(spell.Id, casterId));
        return spellCount;
    }

    public void ApplyMageLoadout()
    {
        if (_context == null)
        {
            return;
        }

        _context.State.Spellbook.ClearSlots();
        var assigned = 0;
        for (var slotIndex = 0; slotIndex < _context.State.SelectedMageStartingSpellIds.Count; slotIndex++)
        {
            var spellId = _context.State.SelectedMageStartingSpellIds[slotIndex];
            if (_context.Content.Spells.ContainsKey(spellId))
            {
                _context.State.Spellbook.AssignSlot(slotIndex, spellId);
                assigned++;
            }
        }

        if (assigned == 0)
        {
            GD.PushWarning($"Mage '{_context.State.SelectedMageId}' did not assign any valid starting spells.");
        }
    }

    private static SpellExecutionContext BuildRepeatContext(
        RunContext context,
        SpellRuntimeDefinition spell,
        EntityId casterId,
        Vector3 origin,
        Vector3 direction,
        int repeatIndex,
        int spellCount,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0,
        Vector3? placementOverride = null)
    {
        var normalizedDirection = direction.LengthSquared() <= 0.0001f ? Vector3.Forward : direction.Normalized();
        if (spellCount <= 1)
        {
            return new SpellExecutionContext(
                context,
                spell,
                casterId,
                origin,
                normalizedDirection,
                MathF.Max(0f, damageMultiplier),
                Math.Max(0, spellChainGeneration),
                placementOverride);
        }

        var right = Vector3.Up.Cross(normalizedDirection);
        if (right.LengthSquared() <= 0.0001f)
        {
            right = Vector3.Right;
        }

        right = right.Normalized();
        var offsetIndex = repeatIndex - (spellCount - 1) * 0.5f;
        var repeatedOrigin = origin + right * offsetIndex * ResolveRepeatLaneSpacing(spell);
        return new SpellExecutionContext(
            context,
            spell,
            casterId,
            repeatedOrigin,
            normalizedDirection,
            MathF.Max(0f, damageMultiplier),
            Math.Max(0, spellChainGeneration),
            placementOverride);
    }

    private static float ResolveRepeatLaneSpacing(SpellRuntimeDefinition spell)
    {
        var spacing = 0.35f;
        foreach (var effect in spell.Effects)
        {
            if (!effect.EffectType.Contains("projectile", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            spacing = MathF.Max(spacing, MathF.Max(0.05f, effect.Radius) * 2.15f);
        }

        return spacing;
    }

    private void TickCastFlow(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var castFlow = _context.State.Spellbook.CastFlow;
        if (!castFlow.IsActive)
        {
            return;
        }

        if (!_context.State.PhasePermissions.CanCastSpells)
        {
            CancelCastFlow("phase_changed");
            return;
        }

        if (!_context.State.Spellbook.TryGetSlot(castFlow.SlotIndex, out var slot)
            || slot == null
            || !slot.HasSpell
            || !slot.SpellId.Equals(castFlow.SpellId))
        {
            CancelCastFlow("spell_changed");
            return;
        }

        castFlow.Tick(delta);
        _context.State.DebugMetrics.LastCastFlowSummary =
            castFlow.IsReady
                ? $"ready {castFlow.SpellId} via {castFlow.SourceItemId}"
                : $"charging {castFlow.SpellId} {castFlow.ChargeElapsedSeconds:0.##}/{castFlow.RequiredChargeSeconds:0.##}s";

        if (castFlow.IsReady && !castFlow.ReadyEventPublished)
        {
            castFlow.ReadyEventPublished = true;
            _context.Events.Publish(new SpellChargeReadyEvent(castFlow.SpellId, castFlow.CasterId, castFlow.SourceItemId));
        }
    }

    private void CancelCastFlow(string reason)
    {
        if (_context == null)
        {
            return;
        }

        var castFlow = _context.State.Spellbook.CastFlow;
        if (!castFlow.IsActive)
        {
            return;
        }

        var spellId = castFlow.SpellId;
        var casterId = castFlow.CasterId;
        var sourceItemId = castFlow.SourceItemId;
        var elapsed = castFlow.ChargeElapsedSeconds;
        castFlow.Clear();
        _context.State.DebugMetrics.LastCastFlowSummary =
            $"canceled {spellId} via {sourceItemId}: {reason}";
        _context.Events.Publish(new SpellChargeCanceledEvent(spellId, casterId, sourceItemId, elapsed, reason));
    }
}
