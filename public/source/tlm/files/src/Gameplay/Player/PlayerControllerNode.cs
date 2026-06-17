using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Player;

public partial class PlayerControllerNode : CharacterBody3D
{
    private readonly PlayerMotor _motor = new();
    private readonly PlayerInputReader _input = new();
    private RunControllerNode? _runController;
    private Camera3D? _camera;
    private SpellPlacementPreviewNode? _placementPreview;
    private Node3D? _magePresentationRoot;
    private Node3D? _fallbackMageVisual;
    private Node? _magePresentationInstance;
    private SubscriptionToken _teleportToken;
    private ContentId _boundPresentationMageId;
    private float _yaw;
    private float _pitch;

    [Export] public float MouseSensitivity { get; set; } = 0.0025f;

    [Export] public float GamepadLookSensitivity { get; set; } = 2.4f;

    public override void _Ready()
    {
        _camera = GetNodeOrNull<Camera3D>("Camera3D");
        _magePresentationRoot = GetNodeOrNull<Node3D>("VisualRoot/MagePresentationRoot");
        _fallbackMageVisual = GetNodeOrNull<Node3D>("VisualRoot/FallbackMageVisual");
        _placementPreview = new SpellPlacementPreviewNode
        {
            Name = "SpellPlacementPreview"
        };
        AddChild(_placementPreview);
        _placementPreview.HidePreview();
        _runController = GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        if (_runController?.Context != null)
        {
            var context = _runController.Context;
            context.State.Player.Position = GlobalPosition;
            var maxHealth = PlayerAttributeResolver.ResolveValue(
                context,
                PlayerAttributeResolver.Health,
                context.State.Player.BaseMaxHealth,
                recordBreakdown: false);
            context.GetSystem<CombatSystem>().RegisterTarget(
                context.State.Player.EntityId,
                CombatTargetKind.Mage,
                maxHealth,
                context.State.Player.MageId);
            _teleportToken = context.Events.Subscribe<PlayerTeleportRequestedEvent>(OnPlayerTeleportRequested);
            UpdateMagePresentation(context);
        }

        Input.MouseMode = Input.MouseModeEnum.Captured;
    }

    public override void _ExitTree()
    {
        if (_runController?.Context != null)
        {
            _runController.Context.Events.Unsubscribe(_teleportToken);
        }
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        var context = _runController?.Context;
        if (context?.State.PhasePermissions.CanCastSpells != true
            || context.State.IsDebugUiInputCaptured
            || context.State.IsPlayerStasisActive)
        {
            return;
        }

        if (@event is InputEventMouseMotion motion)
        {
            var sensitivity = MouseSensitivity * Math.Clamp(context.Settings.MouseSensitivity, 0.1f, 4f);
            _yaw -= motion.Relative.X * sensitivity;
            _pitch = Mathf.Clamp(_pitch - motion.Relative.Y * sensitivity, -1.3f, 1.3f);
            Rotation = new Vector3(0f, _yaw, 0f);
            if (_camera != null)
            {
                _camera.Rotation = new Vector3(_pitch, 0f, 0f);
            }
        }
    }

    public override void _PhysicsProcess(double delta)
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        UpdateMagePresentation(context);
        UpdateMouseMode(context);
        if (context.State.IsDebugUiInputCaptured)
        {
            Velocity = Vector3.Zero;
            _motor.ResetTransientInput();
            MoveAndSlide();
            context.State.Player.Position = GlobalPosition;
            _placementPreview?.HidePreview();
            return;
        }

        var selectedSlot = _input.ReadSelectedSlot(context.State.Spellbook.SelectedSlotIndex);
        context.State.Spellbook.SelectSlot(selectedSlot);

        if (context.State.IsPlayerStasisActive)
        {
            Velocity = Vector3.Zero;
            _motor.ResetTransientInput();
            MoveAndSlide();
        }
        else if (context.State.PhasePermissions.CanCastSpells)
        {
            _motor.MoveSpeed = PlayerAttributeResolver.ResolveValue(
                context,
                PlayerAttributeResolver.MoveSpeed,
                context.State.Player.BaseMoveSpeed,
                recordBreakdown: false);
            _motor.Move(this, _input.ReadMove(), _input.IsJumpJustPressed(), GlobalTransform.Basis, delta);
        }
        else
        {
            Velocity = Vector3.Zero;
            _motor.ResetTransientInput();
            MoveAndSlide();
        }

        UpdateGamepadLook(context, delta);
        context.State.Player.Position = GlobalPosition;
        context.State.Player.AimDirection = GetAimDirection();
        UpdateSpellPlacementPreview(context);

        if (context.State.IsPlayerStasisActive)
        {
            _placementPreview?.HidePreview();
            return;
        }

        if (_input.IsActivatableItemJustPressed())
        {
            context.Commands.TryExecute(new UseActivatableItemCommand(), context, out _);
        }

        if (context.State.TemporaryWeaponMode?.IsActive == true)
        {
            if (_input.IsCastingHeld())
            {
                var origin = (_camera?.GlobalPosition ?? GlobalPosition) + context.State.Player.AimDirection * 0.8f;
                context.GetSystem<SpellSystem>().TryFireTemporaryWeapon(
                    context.State.Player.EntityId,
                    origin,
                    context.State.Player.AimDirection);
            }

            return;
        }

        if (_input.IsCastingJustPressed()
            && context.GetSystem<SpellSystem>().CanBeginCastFlow(context.State.Spellbook.SelectedSlotIndex))
        {
            context.Commands.TryExecute(
                new BeginCastFlowCommand(context.State.Player.EntityId, context.State.Spellbook.SelectedSlotIndex),
                context,
                out _);
        }

        if (_input.IsCastingJustReleased() && context.State.Spellbook.CastFlow.IsActive)
        {
            var origin = (_camera?.GlobalPosition ?? GlobalPosition) + context.State.Player.AimDirection * 0.8f;
            context.Commands.TryExecute(new ReleaseCastFlowCommand(origin, context.State.Player.AimDirection), context, out _);
        }

        if (_input.IsCastingHeld()
            && (!context.State.Spellbook.CastFlow.IsActive || context.State.Spellbook.CastFlow.AllowsConcurrentCasting)
            && CanAttemptHeldCast(context))
        {
            var origin = (_camera?.GlobalPosition ?? GlobalPosition) + context.State.Player.AimDirection * 0.8f;
            var command = new CastSpellCommand(
                context.State.Player.EntityId,
                context.State.Spellbook.SelectedSlotIndex,
                origin,
                context.State.Player.AimDirection);
            context.Commands.TryExecute(command, context, out _);
        }
    }

    private void OnPlayerTeleportRequested(PlayerTeleportRequestedEvent gameEvent)
    {
        GlobalPosition = gameEvent.TargetPosition;
        Velocity = Vector3.Zero;
        var context = _runController?.Context;
        if (context != null)
        {
            context.State.Player.Position = GlobalPosition;
        }
    }

    private void UpdateMagePresentation(RunContext context)
    {
        if (_magePresentationRoot == null
            || _boundPresentationMageId.Equals(context.State.SelectedMageId)
            || !context.Content.TryGetMage(context.State.SelectedMageId, out var mage)
            || mage == null)
        {
            return;
        }

        ClearMagePresentation();
        _boundPresentationMageId = context.State.SelectedMageId;
        if (mage.PresentationScene == null)
        {
            SetFallbackMageVisible(true);
            return;
        }

        _magePresentationInstance = mage.PresentationScene.Instantiate();
        _magePresentationRoot.AddChild(_magePresentationInstance);
        SetFallbackMageVisible(false);
    }

    private void ClearMagePresentation()
    {
        if (_magePresentationInstance != null)
        {
            _magePresentationInstance.QueueFree();
            _magePresentationInstance = null;
        }

        if (_magePresentationRoot == null)
        {
            return;
        }

        foreach (var child in _magePresentationRoot.GetChildren())
        {
            child.QueueFree();
        }
    }

    private void SetFallbackMageVisible(bool visible)
    {
        if (_fallbackMageVisual != null)
        {
            _fallbackMageVisual.Visible = visible;
        }
    }

    private Vector3 GetAimDirection()
    {
        if (_camera != null)
        {
            return -_camera.GlobalTransform.Basis.Z.Normalized();
        }

        return -GlobalTransform.Basis.Z.Normalized();
    }

    private void UpdateGamepadLook(RunContext context, double delta)
    {
        var look = _input.ReadLook();
        if (look.LengthSquared() <= 0.0001f)
        {
            return;
        }

        var scale = GamepadLookSensitivity
            * Math.Clamp(context.Settings.GamepadSensitivity, 0.1f, 4f)
            * (float)delta;
        _yaw -= look.X * scale;
        _pitch = Mathf.Clamp(_pitch - look.Y * scale, -1.3f, 1.3f);
        Rotation = new Vector3(0f, _yaw, 0f);
        if (_camera != null)
        {
            _camera.Rotation = new Vector3(_pitch, 0f, 0f);
        }
    }

    private static bool CanAttemptHeldCast(RunContext context)
    {
        return context.State.Spellbook.TryGetSlot(context.State.Spellbook.SelectedSlotIndex, out var slot)
            && slot != null
            && slot.HasSpell
            && slot.IsReady;
    }

    private void UpdateSpellPlacementPreview(RunContext context)
    {
        if (_placementPreview == null || !context.State.PhasePermissions.CanCastSpells)
        {
            _placementPreview?.HidePreview();
            return;
        }

        if (!context.State.Spellbook.TryGetSlot(context.State.Spellbook.SelectedSlotIndex, out var slot)
            || slot == null
            || !slot.HasSpell
            || !context.Content.TryGetSpell(slot.SpellId, out var spell)
            || spell == null)
        {
            _placementPreview.HidePreview();
            return;
        }

        var origin = (_camera?.GlobalPosition ?? GlobalPosition) + context.State.Player.AimDirection * 0.8f;
        if (SpellPlacement.TryBuildPreview(context, spell, origin, context.State.Player.AimDirection, out var preview))
        {
            _placementPreview.ShowPreview(preview);
            return;
        }

        _placementPreview.HidePreview();
    }

    private static void UpdateMouseMode(RunContext context)
    {
        var targetMode = context.State.IsDebugUiInputCaptured || !context.State.PhasePermissions.CanCastSpells
            ? Input.MouseModeEnum.Visible
            : Input.MouseModeEnum.Captured;

        if (Input.MouseMode != targetMode)
        {
            Input.MouseMode = targetMode;
        }
    }
}
