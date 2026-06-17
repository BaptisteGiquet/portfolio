using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class ContentCatalog : Resource
{
    [Export] public Godot.Collections.Array<MageDefinition> Mages { get; set; } = new();

    [Export] public Godot.Collections.Array<SpellDefinition> Spells { get; set; } = new();

    [Export] public Godot.Collections.Array<ItemDefinition> Items { get; set; } = new();

    [Export] public Godot.Collections.Array<EnemyArchetypeDefinition> Enemies { get; set; } = new();

    [Export] public Godot.Collections.Array<WaveDefinition> Waves { get; set; } = new();

    [Export] public Godot.Collections.Array<DefenseDefinition> Defenses { get; set; } = new();

    [Export] public Godot.Collections.Array<AchievementDefinition> Achievements { get; set; } = new();

    [Export] public Godot.Collections.Array<FeedbackProfile> FeedbackProfiles { get; set; } = new();
}
