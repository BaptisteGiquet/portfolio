using Godot;
using System.Text.RegularExpressions;
using TheLastMage.Localization;

namespace TheLastMage.Controls;

public static partial class InputPromptService
{
    private static InputDeviceKind _lastDeviceKind = InputDeviceKind.KeyboardMouse;
    private static int _bindingVersion;

    public static InputDeviceKind LastDeviceKind => _lastDeviceKind;

    public static int BindingVersion => _bindingVersion;

    public static void ObserveInputEvent(InputEvent inputEvent)
    {
        if (inputEvent is InputEventMouseMotion mouseMotion && mouseMotion.Relative.LengthSquared() <= 0.0001f)
        {
            return;
        }

        if (inputEvent is InputEventJoypadMotion joyMotion && MathF.Abs(joyMotion.AxisValue) < 0.25f)
        {
            return;
        }

        if (inputEvent is InputEventKey { Echo: true })
        {
            return;
        }

        var deviceKind = GetDeviceKind(inputEvent);
        if (deviceKind != _lastDeviceKind)
        {
            _lastDeviceKind = deviceKind;
            _bindingVersion++;
        }
    }

    public static void NotifyBindingsChanged()
    {
        _bindingVersion++;
    }

    public static InputDeviceKind GetDeviceKind(InputEvent inputEvent)
    {
        return inputEvent is InputEventJoypadButton or InputEventJoypadMotion
            ? InputDeviceKind.Controller
            : InputDeviceKind.KeyboardMouse;
    }

    public static string ActionLabel(string actionName)
    {
        return ActionLabel(actionName, _lastDeviceKind);
    }

    public static string ActionLabel(string actionName, InputDeviceKind deviceKind)
    {
        var events = InputBindingService.GetActionEvents(actionName);
        var matching = events
            .Where(inputEvent => GetDeviceKind(inputEvent) == deviceKind)
            .Select(FormatEvent)
            .Where(label => !string.IsNullOrWhiteSpace(label))
            .Distinct(StringComparer.Ordinal)
            .ToList();

        if (matching.Count == 0)
        {
            matching = events
                .Select(FormatEvent)
                .Where(label => !string.IsNullOrWhiteSpace(label))
                .Distinct(StringComparer.Ordinal)
                .ToList();
        }

        return matching.Count == 0
            ? LocalizationService.Current.Text("ui.pause.unbound")
            : string.Join("/", matching);
    }

    public static string FormatEvent(InputEvent inputEvent)
    {
        return inputEvent switch
        {
            InputEventJoypadButton joyButton => FormatJoyButton(joyButton.ButtonIndex),
            InputEventJoypadMotion joyMotion => FormatJoyAxis(joyMotion.Axis, joyMotion.AxisValue),
            InputEventMouseButton mouseButton => FormatMouseButton(mouseButton.ButtonIndex),
            InputEventKey key => FormatKey(key),
            _ => CleanInputText(inputEvent.AsText())
        };
    }

    private static string FormatJoyButton(JoyButton button)
    {
        return button switch
        {
            JoyButton.A => "A",
            JoyButton.B => "B",
            JoyButton.X => "X",
            JoyButton.Y => "Y",
            JoyButton.LeftShoulder => "LB",
            JoyButton.RightShoulder => "RB",
            JoyButton.LeftStick => "LS",
            JoyButton.RightStick => "RS",
            JoyButton.Back => "View",
            JoyButton.Start => "Menu",
            JoyButton.DpadUp => "D-pad Up",
            JoyButton.DpadDown => "D-pad Down",
            JoyButton.DpadLeft => "D-pad Left",
            JoyButton.DpadRight => "D-pad Right",
            _ => CleanInputText(button.ToString())
        };
    }

    private static string FormatJoyAxis(JoyAxis axis, float value)
    {
        var positive = value >= 0f;
        return axis switch
        {
            JoyAxis.LeftX => positive ? "LS Right" : "LS Left",
            JoyAxis.LeftY => positive ? "LS Down" : "LS Up",
            JoyAxis.RightX => positive ? "RS Right" : "RS Left",
            JoyAxis.RightY => positive ? "RS Down" : "RS Up",
            JoyAxis.TriggerLeft => "LT",
            JoyAxis.TriggerRight => "RT",
            _ => $"{axis} {(positive ? "+" : "-")}"
        };
    }

    private static string FormatMouseButton(MouseButton button)
    {
        return button switch
        {
            MouseButton.Left => "LMB",
            MouseButton.Right => "RMB",
            MouseButton.Middle => "MMB",
            MouseButton.WheelUp => "Wheel Up",
            MouseButton.WheelDown => "Wheel Down",
            _ => CleanInputText(button.ToString())
        };
    }

    private static string FormatKey(InputEventKey key)
    {
        var label = key.Keycode.ToString();
        label = label switch
        {
            "Escape" => "Esc",
            "Enter" => "Enter",
            "Space" => "Space",
            _ => label
        };

        var modifiers = new List<string>();
        if (key.CtrlPressed)
        {
            modifiers.Add("Ctrl");
        }

        if (key.AltPressed)
        {
            modifiers.Add("Alt");
        }

        if (key.ShiftPressed)
        {
            modifiers.Add("Shift");
        }

        if (key.MetaPressed)
        {
            modifiers.Add("Meta");
        }

        modifiers.Add(label);
        return string.Join("+", modifiers);
    }

    private static string CleanInputText(string text)
    {
        var cleaned = text.Replace(" (Physical)", string.Empty, StringComparison.Ordinal)
            .Replace("Mouse Button ", "Mouse ", StringComparison.Ordinal)
            .Trim();
        return WhitespaceRegex().Replace(cleaned, " ");
    }

    [GeneratedRegex(@"\s+")]
    private static partial Regex WhitespaceRegex();
}
