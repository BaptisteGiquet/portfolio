using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class GameplayTagCatalog : Resource
{
    [Export] public Godot.Collections.Array<GameplayTagDefinition> Tags { get; set; } = new();

    public GameplayTagDefinition? Find(string path)
    {
        var normalized = GameplayTagPath.Normalize(path);
        foreach (var tag in Tags)
        {
            if (tag != null && string.Equals(GameplayTagPath.Normalize(tag.Path), normalized, StringComparison.OrdinalIgnoreCase))
            {
                return tag;
            }
        }

        return null;
    }

    public bool Contains(string path)
    {
        return Find(path) != null;
    }

    public void EnsureTag(string path, string description = "", string usageHint = "")
    {
        var normalized = GameplayTagPath.Normalize(path);
        if (!GameplayTagPath.IsValid(normalized))
        {
            return;
        }

        foreach (var parent in GameplayTagPath.GetSelfAndAncestors(normalized).Reverse())
        {
            if (Contains(parent))
            {
                continue;
            }

            Tags.Add(new GameplayTagDefinition
            {
                Path = parent,
                Description = string.Equals(parent, normalized, StringComparison.OrdinalIgnoreCase) ? description : string.Empty,
                UsageHint = string.Equals(parent, normalized, StringComparison.OrdinalIgnoreCase) ? usageHint : "Parent tag generated for hierarchy."
            });
        }
    }

    public IReadOnlyList<GameplayTagDefinition> OrderedTags()
    {
        return Tags
            .Where(tag => tag != null && GameplayTagPath.IsValid(tag.Path))
            .OrderBy(tag => GameplayTagPath.Normalize(tag.Path), StringComparer.OrdinalIgnoreCase)
            .ToArray();
    }
}
