namespace TheLastMage.Data.Definitions;

public static class AuthoringDropdownOptions
{
    public const string UnresolvedSuffix = " (unresolved)";

    public static IReadOnlyList<AuthoringDropdownOption> BuildPreservingSelected(
        IEnumerable<string> knownValues,
        string? selected)
    {
        var options = new List<AuthoringDropdownOption>();
        var selectedValue = selected?.Trim() ?? string.Empty;
        var selectedIsKnown = false;

        foreach (var knownValue in knownValues)
        {
            var value = knownValue.Trim();
            if (string.IsNullOrWhiteSpace(value)
                || options.Any(option => string.Equals(option.Value, value, StringComparison.OrdinalIgnoreCase)))
            {
                continue;
            }

            options.Add(new AuthoringDropdownOption(value, value, false));
            selectedIsKnown |= string.Equals(value, selectedValue, StringComparison.OrdinalIgnoreCase);
        }

        if (!string.IsNullOrWhiteSpace(selectedValue) && !selectedIsKnown)
        {
            options.Add(new AuthoringDropdownOption(selectedValue, $"{selectedValue}{UnresolvedSuffix}", true));
        }

        return options;
    }
}

public sealed record AuthoringDropdownOption(string Value, string Label, bool IsUnresolved);
