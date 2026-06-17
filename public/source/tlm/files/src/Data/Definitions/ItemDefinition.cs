using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class ItemDefinition : ContentDefinition
{
    [Export] public int ItemNumber { get; set; }

    [Export] public ItemKind Kind { get; set; } = ItemKind.Passive;

    [Export] public bool IsFlavorOnly { get; set; }

    [Export(PropertyHint.MultilineText)] public string HiddenDescription { get; set; } = "Its power is not yet known.";

    [Export(PropertyHint.MultilineText)] public string RevealedStatText { get; set; } = string.Empty;

    [Export(PropertyHint.MultilineText)] public string RevealedBehaviorText { get; set; } = string.Empty;

    [Export(PropertyHint.MultilineText)] public string RevealedEffectText { get; set; } = string.Empty;

    [Export] public Texture2D? Icon { get; set; }

    [Export] public PackedScene? EnemyFormPresentationScene { get; set; }

    [Export] public Godot.Collections.Array<ItemPoolWeightDefinition> PoolWeights { get; set; } = new();

    [Export] public string UnlockAchievementId { get; set; } = string.Empty;

    [Export] public bool HasUnlimitedActivations { get; set; } = true;

    [Export] public int MaxActivations { get; set; }

    [Export] public Godot.Collections.Array<ItemEffectSpec> Effects { get; set; } = new();

    [Export] public Godot.Collections.Array<EffectSpec> ActiveEffects { get; set; } = new();

    public int PlayersPicked { get; set; }

    public float WinRatePercent { get; set; }

    public string GetResolvedRevealedStatText()
    {
        return string.IsNullOrWhiteSpace(RevealedStatText)
            && string.IsNullOrWhiteSpace(RevealedBehaviorText)
            ? RevealedEffectText
            : RevealedStatText;
    }

    public string GetCombinedRevealedText()
    {
        return CombineRevealedText(RevealedStatText, RevealedBehaviorText, RevealedEffectText);
    }

    public static string CombineRevealedText(string statText, string behaviorText, string legacyText = "")
    {
        var stats = statText.Trim();
        var behavior = behaviorText.Trim();
        if (string.IsNullOrWhiteSpace(stats) && string.IsNullOrWhiteSpace(behavior))
        {
            return legacyText.Trim();
        }

        if (string.IsNullOrWhiteSpace(stats))
        {
            return behavior;
        }

        if (string.IsNullOrWhiteSpace(behavior))
        {
            return stats;
        }

        return $"Stats:\n{stats}\n\nBehavior:\n{behavior}";
    }
}
