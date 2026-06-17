using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;

namespace TheLastMage.Data.Tags;

public static class GameplayTagCatalogUtility
{
    public const string DefaultCatalogPath = "res://data/tags/gameplay_tags.tres";

    public static GameplayTagCatalog LoadDefaultCatalog()
    {
        return ResourceLoader.Load<GameplayTagCatalog>(
            DefaultCatalogPath,
            string.Empty,
            ResourceLoader.CacheMode.Ignore) ?? new GameplayTagCatalog();
    }

    public static GameplayTagCatalog BuildImportedCatalog(ContentCatalog content, GameplayTagCatalog? seed = null)
    {
        var catalog = seed ?? new GameplayTagCatalog();
        foreach (var tag in EnumerateAuthoredTags(content, includeTargets: true))
        {
            catalog.EnsureTag(tag);
        }

        return catalog;
    }

    public static IReadOnlyList<string> EnumerateAuthoredTags(ContentCatalog content, bool includeTargets)
    {
        var tags = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        foreach (var definition in content.Mages.Cast<ContentDefinition>()
                     .Concat(content.Spells)
                     .Concat(content.Items)
                     .Concat(content.Enemies)
                     .Concat(content.Waves)
                     .Concat(content.Defenses)
                     .Concat(content.Achievements))
        {
            AddContentTags(tags, definition);
        }

        if (includeTargets)
        {
            foreach (var item in content.Items)
            {
                foreach (var effect in item.Effects)
                {
                    Add(tags, effect?.TargetTag);
                    foreach (var compiled in ItemEffectCompiler.Compile(effect!))
                    {
                        Add(tags, compiled.TargetTag);
                    }
                }
            }
        }

        return tags.OrderBy(tag => tag, StringComparer.OrdinalIgnoreCase).ToArray();
    }

    public static ValidationResult ValidateCatalog(GameplayTagCatalog catalog)
    {
        var result = ValidationResult.Valid();
        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var tag in catalog.Tags)
        {
            if (tag == null)
            {
                result.AddError("gameplay_tags.null_definition", "Gameplay tag catalog contains an empty definition.");
                continue;
            }

            var path = GameplayTagPath.Normalize(tag.Path);
            if (!GameplayTagPath.IsValid(path))
            {
                result.AddError("gameplay_tags.invalid_path", $"Gameplay tag '{tag.Path}' has an invalid path.");
                continue;
            }

            if (!string.Equals(tag.Path, path, StringComparison.Ordinal))
            {
                result.AddError("gameplay_tags.not_normalized", $"Gameplay tag '{tag.Path}' must be normalized as '{path}'.");
            }

            if (!paths.Add(path))
            {
                result.AddError("gameplay_tags.duplicate_path", $"Duplicate gameplay tag '{path}'.");
            }

            var parent = GameplayTagPath.GetParent(path);
            if (!string.IsNullOrWhiteSpace(parent) && !catalog.Contains(parent))
            {
                result.AddError("gameplay_tags.missing_parent", $"Gameplay tag '{path}' is missing parent tag '{parent}'.");
            }
        }

        return result;
    }

    public static bool IsDeprecated(GameplayTagCatalog catalog, string tag)
    {
        return catalog.Find(tag)?.IsDeprecated == true;
    }

    private static void AddContentTags(HashSet<string> tags, ContentDefinition definition)
    {
        foreach (var tag in definition.Tags)
        {
            Add(tags, tag);
        }
    }

    private static void Add(HashSet<string> tags, string? tag)
    {
        var normalized = GameplayTagPath.Normalize(tag);
        if (GameplayTagPath.IsValid(normalized))
        {
            tags.Add(normalized);
        }
    }
}
