using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class SpellDefinition : ContentDefinition
{
    [Export] public float CooldownSeconds { get; set; } = 1f;

    [Export] public float ManaCost { get; set; }

    [Export] public string FeedbackProfileId { get; set; } = string.Empty;

    [Export] public Godot.Collections.Array<EffectSpec> Effects { get; set; } = new();
}
