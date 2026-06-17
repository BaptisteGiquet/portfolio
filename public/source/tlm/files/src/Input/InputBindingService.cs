using Godot;
using TheLastMage.Save;

namespace TheLastMage.Controls;

public static class InputBindingService
{
    private const float DefaultDeadzone = 0.35f;

    private static readonly Dictionary<string, Func<InputEvent>[]> DefaultBindings = new()
    {
        [InputActions.MoveForward] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Z),
            () => CreateJoyAxis(JoyAxis.LeftY, -1f),
            () => CreateJoyButton(JoyButton.DpadUp)
        },
        [InputActions.MoveBack] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.S),
            () => CreateJoyAxis(JoyAxis.LeftY, 1f),
            () => CreateJoyButton(JoyButton.DpadDown)
        },
        [InputActions.MoveLeft] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Q),
            () => CreateJoyAxis(JoyAxis.LeftX, -1f),
            () => CreateJoyButton(JoyButton.DpadLeft)
        },
        [InputActions.MoveRight] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.D),
            () => CreateJoyAxis(JoyAxis.LeftX, 1f),
            () => CreateJoyButton(JoyButton.DpadRight)
        },
        [InputActions.LookUp] = new Func<InputEvent>[] { () => CreateJoyAxis(JoyAxis.RightY, -1f) },
        [InputActions.LookDown] = new Func<InputEvent>[] { () => CreateJoyAxis(JoyAxis.RightY, 1f) },
        [InputActions.LookLeft] = new Func<InputEvent>[] { () => CreateJoyAxis(JoyAxis.RightX, -1f) },
        [InputActions.LookRight] = new Func<InputEvent>[] { () => CreateJoyAxis(JoyAxis.RightX, 1f) },
        [InputActions.CastPrimary] = new Func<InputEvent>[]
        {
            () => CreateMouse(MouseButton.Left),
            () => CreateJoyAxis(JoyAxis.TriggerRight, 1f)
        },
        [InputActions.UseActivatableItem] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.E),
            () => CreateJoyButton(JoyButton.Y)
        },
        [InputActions.SpellPrevious] = new Func<InputEvent>[]
        {
            () => CreateMouse(MouseButton.WheelUp),
            () => CreateJoyButton(JoyButton.LeftShoulder)
        },
        [InputActions.SpellNext] = new Func<InputEvent>[]
        {
            () => CreateMouse(MouseButton.WheelDown),
            () => CreateJoyButton(JoyButton.RightShoulder)
        },
        [InputActions.Interact] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Enter),
            () => CreateJoyButton(JoyButton.A)
        },
        [InputActions.CancelAction] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Escape),
            () => CreateJoyButton(JoyButton.B)
        },
        [InputActions.Pause] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Escape),
            () => CreateJoyButton(JoyButton.Start)
        },
        [InputActions.OpenBuildInspector] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.B),
            () => CreateJoyButton(JoyButton.Back)
        },
        [InputActions.UiAccept] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Enter),
            () => CreateJoyButton(JoyButton.A)
        },
        [InputActions.UiCancel] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Escape),
            () => CreateJoyButton(JoyButton.B)
        },
        [InputActions.UiUp] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Up),
            () => CreateJoyButton(JoyButton.DpadUp),
            () => CreateJoyAxis(JoyAxis.LeftY, -1f)
        },
        [InputActions.UiDown] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Down),
            () => CreateJoyButton(JoyButton.DpadDown),
            () => CreateJoyAxis(JoyAxis.LeftY, 1f)
        },
        [InputActions.UiLeft] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Left),
            () => CreateJoyButton(JoyButton.DpadLeft),
            () => CreateJoyAxis(JoyAxis.LeftX, -1f)
        },
        [InputActions.UiRight] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Right),
            () => CreateJoyButton(JoyButton.DpadRight),
            () => CreateJoyAxis(JoyAxis.LeftX, 1f)
        },
        [InputActions.UiFocusNext] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Tab),
            () => CreateJoyButton(JoyButton.RightShoulder)
        },
        [InputActions.UiFocusPrev] = new Func<InputEvent>[]
        {
            () => CreateKey(Key.Tab, shift: true),
            () => CreateJoyButton(JoyButton.LeftShoulder)
        }
    };

    public static void EnsureDefaultActions()
    {
        foreach (var (actionName, bindings) in DefaultBindings)
        {
            var action = new StringName(actionName);
            if (!InputMap.HasAction(action))
            {
                InputMap.AddAction(action, DefaultDeadzone);
            }

            foreach (var createBinding in bindings)
            {
                var inputEvent = createBinding();
                if (!HasMatchingEvent(action, inputEvent))
                {
                    InputMap.ActionAddEvent(action, inputEvent);
                }
            }
        }
    }

    public static void ApplySavedBindings(SettingsSaveDto settings)
    {
        EnsureDefaultActions();
        foreach (var (actionName, bindings) in settings.InputBindings)
        {
            if (bindings.Count == 0)
            {
                continue;
            }

            var action = new StringName(actionName);
            if (!InputMap.HasAction(action))
            {
                InputMap.AddAction(action, DefaultDeadzone);
            }

            InputMap.ActionEraseEvents(action);
            foreach (var binding in bindings)
            {
                var inputEvent = ToInputEvent(binding);
                if (inputEvent != null)
                {
                    InputMap.ActionAddEvent(action, inputEvent);
                }
            }
        }
    }

    public static bool TryRebindAction(SettingsSaveDto settings, string actionName, InputEvent sourceEvent, out string reason)
    {
        EnsureDefaultActions();
        var action = new StringName(actionName);
        if (!InputMap.HasAction(action))
        {
            reason = $"Unknown input action '{actionName}'.";
            return false;
        }

        if (!TryCreateBindableEvent(sourceEvent, out var replacement, out var deviceKind))
        {
            reason = "That input cannot be used as a binding.";
            return false;
        }

        var retained = InputMap.ActionGetEvents(action)
            .Where(inputEvent => InputPromptService.GetDeviceKind(inputEvent) != deviceKind)
            .Select(CloneEvent)
            .Where(inputEvent => inputEvent != null)
            .Cast<InputEvent>()
            .ToList();
        retained.Add(replacement);

        InputMap.ActionEraseEvents(action);
        foreach (var inputEvent in retained)
        {
            InputMap.ActionAddEvent(action, inputEvent);
        }

        settings.InputBindings[actionName] = retained
            .Select(ToSaveDto)
            .Where(dto => dto != null)
            .Cast<InputBindingSaveDto>()
            .ToList();
        InputPromptService.NotifyBindingsChanged();
        reason = string.Empty;
        return true;
    }

    public static void ResetActionToDefault(SettingsSaveDto settings, string actionName)
    {
        if (!DefaultBindings.TryGetValue(actionName, out var bindings))
        {
            return;
        }

        var action = new StringName(actionName);
        if (!InputMap.HasAction(action))
        {
            InputMap.AddAction(action, DefaultDeadzone);
        }

        InputMap.ActionEraseEvents(action);
        foreach (var createBinding in bindings)
        {
            InputMap.ActionAddEvent(action, createBinding());
        }

        settings.InputBindings.Remove(actionName);
        InputPromptService.NotifyBindingsChanged();
    }

    public static IReadOnlyList<InputEvent> GetActionEvents(string actionName)
    {
        var action = new StringName(actionName);
        return InputMap.HasAction(action)
            ? InputMap.ActionGetEvents(action).ToArray()
            : Array.Empty<InputEvent>();
    }

    private static bool HasMatchingEvent(StringName action, InputEvent candidate)
    {
        foreach (var existing in InputMap.ActionGetEvents(action))
        {
            if (EventsMatch(existing, candidate))
            {
                return true;
            }
        }

        return false;
    }

    private static bool TryCreateBindableEvent(InputEvent sourceEvent, out InputEvent replacement, out InputDeviceKind deviceKind)
    {
        replacement = null!;
        deviceKind = InputDeviceKind.KeyboardMouse;
        switch (sourceEvent)
        {
            case InputEventKey { Pressed: true, Echo: false } key:
                replacement = CreateKey(key.Keycode, key.ShiftPressed, key.CtrlPressed, key.AltPressed, key.MetaPressed);
                return key.Keycode != Godot.Key.None;
            case InputEventMouseButton { Pressed: true } mouse:
                replacement = CreateMouse(mouse.ButtonIndex);
                return true;
            case InputEventJoypadButton { Pressed: true } joyButton:
                deviceKind = InputDeviceKind.Controller;
                replacement = CreateJoyButton(joyButton.ButtonIndex);
                return true;
            case InputEventJoypadMotion joyMotion when MathF.Abs(joyMotion.AxisValue) >= 0.55f:
                deviceKind = InputDeviceKind.Controller;
                replacement = CreateJoyAxis(joyMotion.Axis, MathF.Sign(joyMotion.AxisValue));
                return true;
            default:
                return false;
        }
    }

    private static InputEvent? CloneEvent(InputEvent inputEvent)
    {
        return inputEvent switch
        {
            InputEventKey key => CreateKey(key.Keycode, key.ShiftPressed, key.CtrlPressed, key.AltPressed, key.MetaPressed),
            InputEventMouseButton mouse => CreateMouse(mouse.ButtonIndex),
            InputEventJoypadButton joyButton => CreateJoyButton(joyButton.ButtonIndex),
            InputEventJoypadMotion joyMotion => CreateJoyAxis(joyMotion.Axis, joyMotion.AxisValue),
            _ => null
        };
    }

    private static InputBindingSaveDto? ToSaveDto(InputEvent inputEvent)
    {
        return inputEvent switch
        {
            InputEventKey key => new InputBindingSaveDto
            {
                Kind = "key",
                Key = key.Keycode.ToString()
            },
            InputEventMouseButton mouse => new InputBindingSaveDto
            {
                Kind = "mouse",
                MouseButton = mouse.ButtonIndex.ToString()
            },
            InputEventJoypadButton joyButton => new InputBindingSaveDto
            {
                Kind = "joy_button",
                JoyButton = joyButton.ButtonIndex.ToString()
            },
            InputEventJoypadMotion joyMotion => new InputBindingSaveDto
            {
                Kind = "joy_axis",
                JoyAxis = joyMotion.Axis.ToString(),
                AxisValue = MathF.Sign(joyMotion.AxisValue)
            },
            _ => null
        };
    }

    private static InputEvent? ToInputEvent(InputBindingSaveDto binding)
    {
        return binding.Kind switch
        {
            "key" when Enum.TryParse<Key>(binding.Key, out var key) => CreateKey(key),
            "mouse" when Enum.TryParse<MouseButton>(binding.MouseButton, out var mouseButton) => CreateMouse(mouseButton),
            "joy_button" when Enum.TryParse<JoyButton>(binding.JoyButton, out var joyButton) => CreateJoyButton(joyButton),
            "joy_axis" when Enum.TryParse<JoyAxis>(binding.JoyAxis, out var joyAxis) => CreateJoyAxis(joyAxis, binding.AxisValue),
            _ => null
        };
    }

    private static bool EventsMatch(InputEvent left, InputEvent right)
    {
        return (left, right) switch
        {
            (InputEventKey a, InputEventKey b) => a.Keycode == b.Keycode
                && a.ShiftPressed == b.ShiftPressed
                && a.CtrlPressed == b.CtrlPressed
                && a.AltPressed == b.AltPressed
                && a.MetaPressed == b.MetaPressed,
            (InputEventMouseButton a, InputEventMouseButton b) => a.ButtonIndex == b.ButtonIndex,
            (InputEventJoypadButton a, InputEventJoypadButton b) => a.ButtonIndex == b.ButtonIndex,
            (InputEventJoypadMotion a, InputEventJoypadMotion b) => a.Axis == b.Axis
                && MathF.Sign(a.AxisValue) == MathF.Sign(b.AxisValue),
            _ => false
        };
    }

    private static InputEventKey CreateKey(Key key, bool shift = false, bool ctrl = false, bool alt = false, bool meta = false)
    {
        return new InputEventKey
        {
            Keycode = key,
            ShiftPressed = shift,
            CtrlPressed = ctrl,
            AltPressed = alt,
            MetaPressed = meta
        };
    }

    private static InputEventMouseButton CreateMouse(MouseButton button)
    {
        return new InputEventMouseButton { ButtonIndex = button };
    }

    private static InputEventJoypadButton CreateJoyButton(JoyButton button)
    {
        return new InputEventJoypadButton { ButtonIndex = button };
    }

    private static InputEventJoypadMotion CreateJoyAxis(JoyAxis axis, float value)
    {
        return new InputEventJoypadMotion { Axis = axis, AxisValue = value };
    }
}
