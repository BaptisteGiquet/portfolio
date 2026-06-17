using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class GameplayTagDefinition : Resource
{
    [Export] public string Path { get; set; } = string.Empty;

    [Export(PropertyHint.MultilineText)] public string Description { get; set; } = string.Empty;

    [Export] public bool IsDeprecated { get; set; }

    [Export(PropertyHint.MultilineText)] public string UsageHint { get; set; } = string.Empty;

    [Export] public Godot.Collections.Array<string> AllowedContentCategories { get; set; } = new();
}
