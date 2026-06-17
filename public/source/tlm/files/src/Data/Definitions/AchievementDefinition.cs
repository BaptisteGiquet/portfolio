using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class AchievementDefinition : ContentDefinition
{
    [Export] public string RequirementKey { get; set; } = string.Empty;

    [Export] public Godot.Collections.Array<string> UnlockMageIds { get; set; } = new();

    [Export] public Godot.Collections.Array<string> UnlockItemIds { get; set; } = new();
}
