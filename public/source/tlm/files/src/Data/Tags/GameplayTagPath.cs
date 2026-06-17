using TheLastMage.Foundation;

namespace TheLastMage.Foundation;

public static class GameplayTagPath
{
    public static string Normalize(string? value)
    {
        var normalized = new string((value ?? string.Empty)
            .Trim()
            .ToLowerInvariant()
            .Select(character => char.IsLetterOrDigit(character) || character == '_' || character == '.'
                ? character
                : '_')
            .ToArray());

        while (normalized.Contains("__", StringComparison.Ordinal))
        {
            normalized = normalized.Replace("__", "_", StringComparison.Ordinal);
        }

        while (normalized.Contains("..", StringComparison.Ordinal))
        {
            normalized = normalized.Replace("..", ".", StringComparison.Ordinal);
        }

        return normalized.Trim('_', '.');
    }

    public static bool IsValid(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return false;
        }

        var tag = value.Trim();
        if (!string.Equals(tag, Normalize(tag), StringComparison.Ordinal))
        {
            return false;
        }

        if (tag.StartsWith(".", StringComparison.Ordinal)
            || tag.EndsWith(".", StringComparison.Ordinal)
            || tag.Contains("..", StringComparison.Ordinal))
        {
            return false;
        }

        foreach (var segment in tag.Split('.'))
        {
            if (segment.Length == 0)
            {
                return false;
            }
        }

        return true;
    }

    public static string GetParent(string path)
    {
        var normalized = Normalize(path);
        var index = normalized.LastIndexOf('.');
        return index <= 0 ? string.Empty : normalized[..index];
    }

    public static IEnumerable<string> GetAncestors(string path)
    {
        var parent = GetParent(path);
        while (!string.IsNullOrWhiteSpace(parent))
        {
            yield return parent;
            parent = GetParent(parent);
        }
    }

    public static IEnumerable<string> GetSelfAndAncestors(string path)
    {
        var normalized = Normalize(path);
        if (GameplayTagPath.IsValid(normalized))
        {
            yield return normalized;
        }

        foreach (var parent in GetAncestors(normalized))
        {
            yield return parent;
        }
    }

    public static bool IsSameOrParentOf(TagId targetTag, TagId sourceTag)
    {
        return IsSameOrParentOf(targetTag.ToString(), sourceTag.ToString());
    }

    public static bool IsSameOrParentOf(string? targetTag, string? sourceTag)
    {
        var target = Normalize(targetTag);
        if (string.IsNullOrWhiteSpace(target))
        {
            return true;
        }

        var source = Normalize(sourceTag);
        if (string.IsNullOrWhiteSpace(source))
        {
            return false;
        }

        return string.Equals(source, target, StringComparison.OrdinalIgnoreCase)
            || source.StartsWith(target + ".", StringComparison.OrdinalIgnoreCase);
    }

    public static bool MatchesAny(TagId targetTag, IEnumerable<TagId> sourceTags)
    {
        if (!targetTag.IsValid)
        {
            return true;
        }

        if (sourceTags is GameplayTagSet tagSet)
        {
            return tagSet.MatchesTarget(targetTag);
        }

        return !targetTag.IsValid || sourceTags.Any(sourceTag => IsSameOrParentOf(targetTag, sourceTag));
    }

}
