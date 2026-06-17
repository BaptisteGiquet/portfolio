using Godot;
using TheLastMage.Debugging.Tools;
using TheLastMage.UI;

namespace TheLastMage.Debugging;

public partial class DebugServicesNode : Node
{
    private const string DebugOverlayScenePath = "res://scenes/debug/debug_overlay.tscn";

    private CanvasLayer? _debugUiLayer;

    public override void _Ready()
    {
        EnsureDebugOverlay();
        EnsureDebugUiLayer();
        EnsureDebugControl<ToolingGatePanelNode>("ToolingGatePanel");
        EnsureDebugControl<BuildInspectorPanel>("BuildInspectorPanel");
        EnsureDebugControl<AchievementPanel>("AchievementPanel");
        EnsureDebugControl<PauseSettingsPanel>("PauseSettingsPanel");
    }

    private void EnsureDebugOverlay()
    {
        if (GetNodeOrNull<CanvasLayer>("DebugOverlay") != null)
        {
            return;
        }

        var scene = ResourceLoader.Load<PackedScene>(DebugOverlayScenePath);
        if (scene?.Instantiate() is not CanvasLayer overlay)
        {
            GD.PushWarning($"DebugServices could not instantiate {DebugOverlayScenePath}.");
            return;
        }

        overlay.Name = "DebugOverlay";
        AddChild(overlay);
    }

    private void EnsureDebugUiLayer()
    {
        _debugUiLayer = GetNodeOrNull<CanvasLayer>("DebugUiLayer");
        if (_debugUiLayer != null)
        {
            return;
        }

        _debugUiLayer = new CanvasLayer
        {
            Name = "DebugUiLayer",
            Layer = 100
        };
        AddChild(_debugUiLayer);
    }

    private void EnsureDebugControl<TControl>(string nodeName)
        where TControl : Control, new()
    {
        if (_debugUiLayer == null || _debugUiLayer.GetNodeOrNull<TControl>(nodeName) != null)
        {
            return;
        }

        var control = new TControl
        {
            Name = nodeName
        };
        _debugUiLayer.AddChild(control);
    }
}
