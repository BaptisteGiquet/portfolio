using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class ItemEffectSpec : Resource
{
    [Export] public ItemEffectKind Kind { get; set; } = ItemEffectKind.AddMultiplyStat;

    [Export] public string StatId { get; set; } = string.Empty;

    [Export] public float Value { get; set; } = 1f;

    [Export] public float SecondaryValue { get; set; } = 1f;

    [Export] public float TertiaryValue { get; set; }

    [Export(PropertyHint.Range, "0,100,0.1,suffix:%")] public float ChancePercent { get; set; } = 100f;

    [Export] public int Priority { get; set; } = 10;

    [Export] public string TargetTag { get; set; } = string.Empty;
}
