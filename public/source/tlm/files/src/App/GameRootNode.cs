using Godot;

namespace TheLastMage.App;

public partial class GameRootNode : Node
{
    private const string RunRootScene = "res://scenes/run/run_root.tscn";

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed("debug_start_run"))
        {
            GetTree().ChangeSceneToFile(RunRootScene);
        }
    }
}
