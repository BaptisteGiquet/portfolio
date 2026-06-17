using Godot;
using TheLastMage.Controls;

namespace TheLastMage.Gameplay.Player;

public sealed class PlayerInputReader
{
    public Vector2 ReadMove()
    {
        return Input.GetVector(InputActions.MoveLeft, InputActions.MoveRight, InputActions.MoveBack, InputActions.MoveForward);
    }

    public Vector2 ReadLook()
    {
        return Input.GetVector(InputActions.LookLeft, InputActions.LookRight, InputActions.LookUp, InputActions.LookDown);
    }

    public bool IsCastingHeld()
    {
        return Input.IsActionPressed(InputActions.CastPrimary);
    }

    public bool IsCastingJustPressed()
    {
        return Input.IsActionJustPressed(InputActions.CastPrimary);
    }

    public bool IsCastingJustReleased()
    {
        return Input.IsActionJustReleased(InputActions.CastPrimary);
    }

    public bool IsJumpJustPressed()
    {
        return Input.IsActionJustPressed("jump");
    }

    public bool IsActivatableItemJustPressed()
    {
        return Input.IsActionJustPressed(InputActions.UseActivatableItem);
    }

    public int ReadSelectedSlot(int current)
    {
        for (var i = 0; i < 5; i++)
        {
            if (Input.IsActionJustPressed($"spell_slot_{i + 1}"))
            {
                return i;
            }
        }

        if (Input.IsActionJustPressed(InputActions.SpellNext))
        {
            return WrapSlot(current + 1);
        }

        if (Input.IsActionJustPressed(InputActions.SpellPrevious))
        {
            return WrapSlot(current - 1);
        }

        return current;
    }

    private static int WrapSlot(int slot)
    {
        return (slot % 5 + 5) % 5;
    }
}
