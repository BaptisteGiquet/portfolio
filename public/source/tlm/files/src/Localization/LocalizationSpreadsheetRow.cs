namespace TheLastMage.Localization;

public sealed class LocalizationSpreadsheetRow
{
    public string Key { get; set; } = string.Empty;

    public string Context { get; set; } = string.Empty;

    public string English { get; set; } = string.Empty;

    public string Notes { get; set; } = string.Empty;

    public Dictionary<string, string> Translations { get; } = new(StringComparer.OrdinalIgnoreCase);
}
