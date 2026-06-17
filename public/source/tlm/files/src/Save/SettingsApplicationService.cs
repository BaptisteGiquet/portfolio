using Godot;

namespace TheLastMage.Save;

public static class SettingsApplicationService
{
    public static void Apply(SettingsSaveDto settings, Viewport? viewport)
    {
        ApplyAudio(settings);
        ApplyVideo(settings, viewport);
    }

    private static void ApplyAudio(SettingsSaveDto settings)
    {
        var masterBus = AudioServer.GetBusIndex("Master");
        if (masterBus >= 0)
        {
            AudioServer.SetBusVolumeLinear(masterBus, Math.Clamp(settings.MasterVolume, 0f, 1f));
        }
    }

    private static void ApplyVideo(SettingsSaveDto settings, Viewport? viewport)
    {
        Engine.MaxFps = Math.Max(0, settings.FpsCap);
        DisplayServer.WindowSetVsyncMode(settings.VSync
            ? DisplayServer.VSyncMode.Enabled
            : DisplayServer.VSyncMode.Disabled);

        var mode = NormalizeDisplayMode(settings.DisplayMode);
        DisplayServer.WindowSetMode(mode);
        if (mode == DisplayServer.WindowMode.Windowed && TryParseResolution(settings.Resolution, out var resolution))
        {
            DisplayServer.WindowSetSize(resolution);
        }

        if (viewport != null)
        {
            viewport.Scaling3DScale = Math.Clamp(settings.RenderScale, 0.5f, 1.25f);
        }
    }

    private static DisplayServer.WindowMode NormalizeDisplayMode(string displayMode)
    {
        return displayMode.Trim().ToLowerInvariant() switch
        {
            "fullscreen" => DisplayServer.WindowMode.ExclusiveFullscreen,
            "borderless" => DisplayServer.WindowMode.Fullscreen,
            _ => DisplayServer.WindowMode.Windowed
        };
    }

    private static bool TryParseResolution(string value, out Vector2I resolution)
    {
        resolution = new Vector2I(1280, 720);
        var parts = value.Split('x', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        if (parts.Length != 2
            || !int.TryParse(parts[0], out var width)
            || !int.TryParse(parts[1], out var height))
        {
            return false;
        }

        resolution = new Vector2I(Math.Clamp(width, 640, 7680), Math.Clamp(height, 360, 4320));
        return true;
    }
}
