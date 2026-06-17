using Godot;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.UI;

public static class RunUiLocator
{
    public static RunControllerNode? FindRunController(Node node)
    {
        return node.GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
    }
}
