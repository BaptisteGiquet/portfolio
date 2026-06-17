using TheLastMage.Data.Definitions;
using TheLastMage.Foundation;

namespace TheLastMage.Data.Tags;

public static class GameplayTagReferenceMigration
{
    public static int RenameInCatalog(ContentCatalog catalog, string oldPath, string newPath)
    {
        var oldNormalized = GameplayTagPath.Normalize(oldPath);
        var newNormalized = GameplayTagPath.Normalize(newPath);
        if (!GameplayTagPath.IsValid(oldNormalized)
            || !GameplayTagPath.IsValid(newNormalized)
            || string.Equals(oldNormalized, newNormalized, StringComparison.OrdinalIgnoreCase))
        {
            return 0;
        }

        var changed = 0;
        foreach (var definition in catalog.Mages.Cast<ContentDefinition>()
                     .Concat(catalog.Spells)
                     .Concat(catalog.Items)
                     .Concat(catalog.Enemies)
                     .Concat(catalog.Waves)
                     .Concat(catalog.Defenses)
                     .Concat(catalog.Achievements))
        {
            changed += RenameTags(definition.Tags, oldNormalized, newNormalized);
        }

        foreach (var item in catalog.Items)
        {
            foreach (var effect in item.Effects)
            {
                if (TryRenameTagValue(effect.TargetTag, oldNormalized, newNormalized, out var renamed))
                {
                    effect.TargetTag = renamed;
                    changed++;
                }
            }
        }

        return changed;
    }

    private static int RenameTags(Godot.Collections.Array<string> tags, string oldPath, string newPath)
    {
        var changed = 0;
        for (var i = 0; i < tags.Count; i++)
        {
            if (!TryRenameTagValue(tags[i], oldPath, newPath, out var renamed))
            {
                continue;
            }

            tags[i] = renamed;
            changed++;
        }

        return changed;
    }

    private static bool TryRenameTagValue(string value, string oldPath, string newPath, out string renamed)
    {
        var normalized = GameplayTagPath.Normalize(value);
        if (string.Equals(normalized, oldPath, StringComparison.OrdinalIgnoreCase))
        {
            renamed = newPath;
            return true;
        }

        var oldPrefix = oldPath + ".";
        if (normalized.StartsWith(oldPrefix, StringComparison.OrdinalIgnoreCase))
        {
            renamed = newPath + normalized[oldPath.Length..];
            return true;
        }

        renamed = normalized;
        return false;
    }
}
