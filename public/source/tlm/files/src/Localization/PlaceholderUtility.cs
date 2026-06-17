using System.Text.RegularExpressions;

namespace TheLastMage.Localization;

public static class PlaceholderUtility
{
    private static readonly Regex PlaceholderPattern = new(
        @"\{\d+(?::[^}]*)?\}",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    public static HashSet<string> Extract(string text)
    {
        return PlaceholderPattern
            .Matches(text ?? string.Empty)
            .Select(match => match.Value)
            .ToHashSet(StringComparer.Ordinal);
    }
}
