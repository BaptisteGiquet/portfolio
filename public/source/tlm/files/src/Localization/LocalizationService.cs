using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Foundation;
using System.Globalization;

namespace TheLastMage.Localization;

public sealed class LocalizationService
{
    public const string DefaultLocale = "en";

    private readonly Dictionary<string, string> _englishEntries = new(StringComparer.Ordinal);
    private readonly Dictionary<string, string> _entries = new(StringComparer.Ordinal);
    private readonly HashSet<string> _missingKeys = new(StringComparer.Ordinal);
    private LocalizationSpreadsheet _spreadsheet = new();

    public static LocalizationService Current { get; } = new();

    public string CurrentLocale { get; private set; } = DefaultLocale;

    public IReadOnlyDictionary<string, string> EnglishEntries => _englishEntries;

    public IReadOnlyDictionary<string, string> Entries => _entries;

    public IReadOnlyList<string> AvailableLocales => _spreadsheet.LocaleColumns;

    public IReadOnlyCollection<string> MissingKeys => _missingKeys;

    public void LoadEnglish(ContentCatalog catalog, string locale = DefaultLocale, string spreadsheetProjectPath = LocalizationSpreadsheet.DefaultProjectPath)
    {
        _englishEntries.Clear();
        _entries.Clear();
        foreach (var entry in EnglishLocalizationSource.Build(catalog))
        {
            _englishEntries[entry.Key] = entry.Value;
        }

        _spreadsheet = LocalizationSpreadsheet.Load(spreadsheetProjectPath);
        SetLocale(locale);
        _missingKeys.Clear();
    }

    public void AddEnglishContent(ContentCatalog catalog)
    {
        EnglishLocalizationSource.AddContentEntries(_englishEntries, catalog);
        SetLocale(CurrentLocale);
    }

    public void SetLocale(string locale)
    {
        CurrentLocale = LocalizationSpreadsheet.NormalizeLocale(locale);
        _entries.Clear();
        foreach (var (key, english) in _englishEntries)
        {
            var translation = CurrentLocale == DefaultLocale ? null : _spreadsheet.TryGetTranslation(key, CurrentLocale);
            _entries[key] = string.IsNullOrWhiteSpace(translation) ? english : translation;
        }

        _missingKeys.Clear();
    }

    public bool HasKey(string key)
    {
        return _entries.ContainsKey(key);
    }

    public string Text(string key, params object?[] args)
    {
        if (!_entries.TryGetValue(key, out var template))
        {
            ReportMissing(key);
            return $"{LocalizationKeys.MissingMarkerPrefix}{key}{LocalizationKeys.MissingMarkerSuffix}";
        }

        if (args.Length == 0)
        {
            return template;
        }

        try
        {
            return string.Format(CultureInfo.InvariantCulture, template, args);
        }
        catch (FormatException exception)
        {
            GD.PushWarning($"Localization format failed for key '{key}': {exception.Message}");
            return template;
        }
    }

    public string ContentName(string kind, ContentId id, string fallback = "")
    {
        return ContentField(kind, id, "name", fallback);
    }

    public string ContentDescription(string kind, ContentId id, string fallback = "")
    {
        return ContentField(kind, id, "description", fallback);
    }

    public string ContentField(string kind, ContentId id, string field, string fallback = "")
    {
        var key = LocalizationKeys.ContentField(kind, id, field);
        if (_entries.TryGetValue(key, out var value))
        {
            return value;
        }

        ReportMissing(key);
        return string.IsNullOrWhiteSpace(fallback)
            ? $"{LocalizationKeys.MissingMarkerPrefix}{key}{LocalizationKeys.MissingMarkerSuffix}"
            : fallback;
    }

    public bool ContainsMissingMarker(string text)
    {
        return text.Contains(LocalizationKeys.MissingMarkerPrefix, StringComparison.Ordinal)
            && text.Contains(LocalizationKeys.MissingMarkerSuffix, StringComparison.Ordinal);
    }

    private void ReportMissing(string key)
    {
        if (_missingKeys.Add(key))
        {
            GD.PushWarning($"Missing localization key: {key}");
        }
    }
}
