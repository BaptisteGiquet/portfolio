namespace TheLastMage.Controls;

public static class InputActions
{
    public const string MoveForward = "move_forward";
    public const string MoveBack = "move_back";
    public const string MoveLeft = "move_left";
    public const string MoveRight = "move_right";
    public const string LookUp = "look_up";
    public const string LookDown = "look_down";
    public const string LookLeft = "look_left";
    public const string LookRight = "look_right";
    public const string CastPrimary = "cast_primary";
    public const string UseActivatableItem = "use_activatable_item";
    public const string SpellPrevious = "spell_previous";
    public const string SpellNext = "spell_next";
    public const string Interact = "interact";
    public const string CancelAction = "cancel_action";
    public const string Pause = "pause";
    public const string OpenBuildInspector = "open_build_inspector";

    public const string UiAccept = "ui_accept";
    public const string UiCancel = "ui_cancel";
    public const string UiUp = "ui_up";
    public const string UiDown = "ui_down";
    public const string UiLeft = "ui_left";
    public const string UiRight = "ui_right";
    public const string UiFocusNext = "ui_focus_next";
    public const string UiFocusPrev = "ui_focus_prev";

    public static readonly string[] PromptTrackedActions =
    {
        CastPrimary,
        UseActivatableItem,
        SpellPrevious,
        SpellNext,
        Interact,
        CancelAction,
        Pause,
        OpenBuildInspector,
        UiAccept,
        UiCancel,
        UiFocusNext,
        UiFocusPrev
    };
}
