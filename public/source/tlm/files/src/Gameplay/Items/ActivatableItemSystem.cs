using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Items;

public sealed class ActivatableItemSystem : IGameSystem
{
    private readonly List<TeleportProjectileTravel> _teleportProjectiles = new();
    private Node3D? _presentationRoot;
    private RunContext? _context;

    public ActivatableItemSystem(Node3D? presentationRoot = null)
    {
        _presentationRoot = presentationRoot;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
    }

    public void Tick(float delta)
    {
    }

    public void FixedTick(float delta)
    {
        TickTeleportProjectiles(delta);
    }

    public void Shutdown()
    {
        foreach (var projectile in _teleportProjectiles)
        {
            projectile.Visual?.QueueFree();
        }

        _teleportProjectiles.Clear();
        _presentationRoot = null;
        _context = null;
    }

    public bool UseEquippedItem(out string reason)
    {
        reason = string.Empty;
        if (_context == null)
        {
            reason = "activatable item system is not initialized";
            return false;
        }

        var state = _context.State.Build.ActivatableItem;
        if (state == null)
        {
            reason = "no activatable item is equipped";
            return false;
        }

        var item = _context.Content.GetItem(state.ItemId);
        foreach (var effect in item.ActiveEffects)
        {
            ExecuteActiveEffect(item, effect);
        }

        var used = _context.State.Build.TryUseActivatableItem(out var depleted);
        if (!used)
        {
            reason = $"activatable item {item.Id} has no remaining activations";
            return false;
        }

        var remaining = _context.State.Build.ActivatableItem?.RemainingActivations ?? 0;
        _context.State.DebugMetrics.LastActivatableItemSummary = depleted
            ? $"{item.Id} used and depleted"
            : item.ActiveEffects.Count == 0
                ? $"{item.Id} used no active effects remaining={(state.HasUnlimitedActivations ? "unlimited" : remaining.ToString())}"
                : $"{item.Id} used remaining={(state.HasUnlimitedActivations ? "unlimited" : remaining.ToString())}";
        _context.Events.Publish(new ActivatableItemUsedEvent(item.Id, remaining, depleted));
        if (depleted)
        {
            _context.Events.Publish(new ActivatableItemClearedEvent(item.Id));
        }

        return true;
    }

    private void ExecuteActiveEffect(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        switch (effect.EffectType)
        {
            case ActiveEffectIds.MagicBall:
                _context.State.DebugMetrics.LastActivatableItemSummary = $"{item.Id} magic_ball";
                break;
            case ActiveEffectIds.TeleportPlayer:
                TeleportPlayer(item, effect);
                break;
            case ActiveEffectIds.CooldownTick:
                _context.State.Spellbook.TickCooldowns(MathF.Max(0f, effect.Value));
                break;
            case ActiveEffectIds.GrantSouls:
                _context.State.Souls += Math.Max(0, Mathf.RoundToInt(effect.Value));
                break;
            case ActiveEffectIds.GrantMaterials:
                _context.State.Materials += Math.Max(0, Mathf.RoundToInt(effect.Value));
                break;
            case ActiveEffectIds.HealPlayer:
                HealPlayer(item, effect);
                break;
            case ActiveEffectIds.GrantHitProtection:
                GrantHitProtection(item, effect);
                break;
            case ActiveEffectIds.SleepNearbyEnemies:
                SleepNearbyEnemies(item, effect);
                break;
            case ActiveEffectIds.TransformEnemiesToForm:
                TransformEnemiesToForm(item, effect);
                break;
            case ActiveEffectIds.PlayerStasis:
                PlayerStasis(item, effect);
                break;
            case ActiveEffectIds.TemporaryStatModifier:
                TemporaryStatModifier(item, effect);
                break;
            case ActiveEffectIds.AssaultRifleMode:
                AssaultRifleMode(item, effect);
                break;
            case ActiveEffectIds.ThrowTeleportProjectile:
                ThrowTeleportProjectile(item, effect);
                break;
        }
    }

    private void TeleportPlayer(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var distance = effect.Value;
        var direction = _context.State.Player.AimDirection.LengthSquared() > 0.01f
            ? _context.State.Player.AimDirection.Normalized()
            : Vector3.Forward;
        var target = _context.State.Player.Position + direction * distance;
        _context.State.Player.Position = target;
        _context.State.DebugMetrics.LastActivatableItemSummary = $"{item.Id} teleport_player {distance:0.##}m";
        _context.Events.Publish(new PlayerTeleportRequestedEvent(item.Id, target));
    }

    private void HealPlayer(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var combat = _context.GetSystem<CombatSystem>();
        if (!combat.TryGetHealth(_context.State.Player.EntityId, out var health) || health == null)
        {
            return;
        }

        health.Repair(MathF.Max(0f, effect.Value));
        _context.State.DebugMetrics.LastActivatableItemSummary = $"{item.Id} heal_player {effect.Value:0.##}";
    }

    private void GrantHitProtection(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var hitCount = Math.Clamp((int)MathF.Floor(MathF.Max(1f, effect.Value) + 0.001f), 1, 99);
        _context.GetSystem<DamagePreventionSystem>().GrantFullHitProtection(
            _context.State.Player.EntityId,
            item.Id,
            hitCount);
        _context.State.DebugMetrics.LastActivatableItemSummary = $"{item.Id} grant_hit_protection hits={hitCount}";
    }

    private void SleepNearbyEnemies(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var radius = effect.Radius > 0f ? effect.Radius : MathF.Max(1f, effect.Value);
        var duration = effect.DurationSeconds > 0f ? effect.DurationSeconds : 5f;
        var count = _context.GetSystem<EnemySystem>().ApplySleepInRadius(
            item.Id,
            _context.State.Player.Position,
            radius,
            duration);
        _context.State.DebugMetrics.LastActivatableItemSummary =
            $"{item.Id} sleep_nearby_enemies radius={radius:0.##} duration={duration:0.##} count={count}";
    }

    private void TransformEnemiesToForm(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var duration = effect.DurationSeconds > 0f ? effect.DurationSeconds : 10f;
        var maxHealth = effect.Value > 0f ? effect.Value : 1f;
        var count = _context.GetSystem<EnemySystem>().ApplyTemporaryFormToAll(
            item.Id,
            duration,
            maxHealth,
            disablesAttacks: true,
            item.EnemyFormPresentationScene);
        _context.State.DebugMetrics.LastActivatableItemSummary =
            $"{item.Id} transform_enemies_to_form form=among_us duration={duration:0.##} hp={maxHealth:0.##} count={count}";
    }

    private void PlayerStasis(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var duration = effect.DurationSeconds > 0f ? effect.DurationSeconds : MathF.Max(0.1f, effect.Value);
        _context.State.GrantPlayerStasis(item.Id, duration);
        _context.State.DebugMetrics.LastActivatableItemSummary =
            $"{item.Id} player_stasis duration={duration:0.##}";
    }

    private void TemporaryStatModifier(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null || !effect.TargetStatId.IsValid)
        {
            return;
        }

        var duration = effect.DurationSeconds > 0f ? effect.DurationSeconds : 6f;
        var value = effect.Value > 0f ? effect.Value : 1f;
        _context.State.Build.GrantTemporaryModifier(
            item.Id,
            StatId.From(effect.TargetStatId.Value),
            ModifierOp.Multiplicative,
            value,
            10,
            default,
            duration);
        _context.GetSystem<CombatSystem>().RefreshPlayerHealthFromStats();
        _context.State.DebugMetrics.LastActivatableItemSummary =
            $"{item.Id} temporary_stat_modifier {effect.TargetStatId} x{value:0.##} duration={duration:0.##}";
    }

    private void AssaultRifleMode(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var duration = effect.DurationSeconds > 0f ? effect.DurationSeconds : 8f;
        var fireRateMultiplier = effect.Value > 0f ? effect.Value : 2f;
        _context.State.GrantTemporaryWeaponMode(item.Id, duration, fireRateMultiplier);
        _context.State.DebugMetrics.LastActivatableItemSummary =
            $"{item.Id} assault_rifle_mode fireRate=x{fireRateMultiplier:0.##} duration={duration:0.##}";
    }

    private void ThrowTeleportProjectile(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        if (_context == null)
        {
            return;
        }

        var direction = _context.State.Player.AimDirection.LengthSquared() > 0.01f
            ? _context.State.Player.AimDirection.Normalized()
            : Vector3.Forward;
        var distance = effect.Value > 0f ? effect.Value : 14f;
        var duration = effect.DurationSeconds > 0f ? effect.DurationSeconds : 0.55f;
        var arcHeight = effect.Radius > 0f ? effect.Radius : 2.2f;
        var start = _context.State.Player.Position + new Vector3(0f, 1.2f, 0f);
        var end = _context.State.Player.Position + direction * distance;
        end.Y = _context.State.Player.Position.Y;
        var visual = CreateTeleportProjectileVisual(start);
        _teleportProjectiles.Add(new TeleportProjectileTravel(
            item.Id,
            start,
            end,
            duration,
            arcHeight,
            visual));
        _context.State.DebugMetrics.LastActivatableItemSummary =
            $"{item.Id} throw_teleport_projectile distance={distance:0.##} duration={duration:0.##}";
    }

    private MeshInstance3D? CreateTeleportProjectileVisual(Vector3 start)
    {
        if (_presentationRoot == null)
        {
            return null;
        }

        var visual = new MeshInstance3D
        {
            Name = "TeleportPearlProjectile",
            Mesh = new SphereMesh
            {
                Radius = 0.16f,
                Height = 0.32f
            },
            MaterialOverride = new StandardMaterial3D
            {
                AlbedoColor = new Color(0.25f, 0.95f, 0.82f, 1f),
                EmissionEnabled = true,
                Emission = new Color(0.1f, 0.7f, 0.62f, 1f),
                EmissionEnergyMultiplier = 1.2f
            }
        };
        _presentationRoot.AddChild(visual);
        visual.GlobalPosition = start;
        return visual;
    }

    private void TickTeleportProjectiles(float delta)
    {
        if (_context == null)
        {
            return;
        }

        for (var i = _teleportProjectiles.Count - 1; i >= 0; i--)
        {
            var projectile = _teleportProjectiles[i];
            projectile.Tick(delta);
            if (projectile.Visual != null)
            {
                projectile.Visual.GlobalPosition = projectile.CurrentPosition;
            }

            if (!projectile.IsComplete)
            {
                continue;
            }

            _context.State.Player.Position = projectile.End;
            projectile.Visual?.QueueFree();
            _teleportProjectiles.RemoveAt(i);
            _context.State.DebugMetrics.LastActivatableItemSummary =
                $"{projectile.SourceId} throw_teleport_projectile landed";
            _context.Events.Publish(new PlayerTeleportRequestedEvent(projectile.SourceId, projectile.End));
        }
    }

    private sealed class TeleportProjectileTravel
    {
        private readonly float _durationSeconds;
        private readonly float _arcHeight;
        private float _elapsedSeconds;

        public TeleportProjectileTravel(
            ContentId sourceId,
            Vector3 start,
            Vector3 end,
            float durationSeconds,
            float arcHeight,
            MeshInstance3D? visual)
        {
            SourceId = sourceId;
            Start = start;
            End = end;
            _durationSeconds = MathF.Max(0.05f, durationSeconds);
            _arcHeight = MathF.Max(0f, arcHeight);
            Visual = visual;
            CurrentPosition = start;
        }

        public ContentId SourceId { get; }

        public Vector3 Start { get; }

        public Vector3 End { get; }

        public MeshInstance3D? Visual { get; }

        public Vector3 CurrentPosition { get; private set; }

        public bool IsComplete => _elapsedSeconds >= _durationSeconds;

        public void Tick(float delta)
        {
            _elapsedSeconds = MathF.Min(_durationSeconds, _elapsedSeconds + MathF.Max(0f, delta));
            var t = _elapsedSeconds / _durationSeconds;
            CurrentPosition = Start.Lerp(End, t) + Vector3.Up * (MathF.Sin(t * MathF.PI) * _arcHeight);
        }
    }
}
