using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
public abstract partial class ContentDefinition : Resource
{
    [Export] public string Id { get; set; } = string.Empty;

    [Export] public string DisplayName { get; set; } = string.Empty;

    [Export(PropertyHint.MultilineText)] public string Description { get; set; } = string.Empty;

    [Export] public Godot.Collections.Array<string> Tags { get; set; } = new();
}
