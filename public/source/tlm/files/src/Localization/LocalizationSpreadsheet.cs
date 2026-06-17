using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;
using System.Text;

namespace TheLastMage.Localization;

public sealed class LocalizationSpreadsheet
{
    public const string DefaultProjectPath = "res://data/localization/Localization.csv";

    public static readonly string[] FixedColumns =
    {
        "key",
        "context",
        "en",
        "notes"
    };

    private readonly List<LocalizationSpreadsheetRow> _rows = new();
    private readonly List<string> _localeColumns = new();

    public IReadOnlyList<LocalizationSpreadsheetRow> Rows => _rows;

    public IReadOnlyList<string> LocaleColumns => _localeColumns;

    public static LocalizationSpreadsheet FromSource(ContentCatalog catalog)
    {
        var spreadsheet = new LocalizationSpreadsheet();
        foreach (var entry in EnglishLocalizationSource.BuildSourceEntries(catalog).OrderBy(entry => entry.Key, StringComparer.Ordinal))
        {
            spreadsheet._rows.Add(new LocalizationSpreadsheetRow
            {
                Key = entry.Key,
                Context = entry.Context,
                English = entry.English,
                Notes = entry.Notes
            });
        }

        return spreadsheet;
    }

    public static LocalizationSpreadsheet ExportSource(ContentCatalog catalog, string projectPath = DefaultProjectPath)
    {
        var source = FromSource(catalog);
        if (ProjectFileExists(projectPath))
        {
            var existing = Load(projectPath);
            source.MergeTranslationsFrom(existing);
        }

        source.Save(projectPath);
        return source;
    }

    public static LocalizationSpreadsheet ImportAndNormalize(ContentCatalog catalog, string sourceProjectPath, string targetProjectPath = DefaultProjectPath)
    {
        var imported = Load(sourceProjectPath);
        var normalized = FromSource(catalog);
        normalized.MergeTranslationsFrom(imported);
        normalized.Save(targetProjectPath);
        return normalized;
    }

    public static LocalizationSpreadsheet Load(string projectPath = DefaultProjectPath)
    {
        var spreadsheet = new LocalizationSpreadsheet();
        var path = Globalize(projectPath);
        if (!File.Exists(path))
        {
            return spreadsheet;
        }

        var rows = ParseCsv(File.ReadAllText(path, Encoding.UTF8));
        if (rows.Count == 0)
        {
            return spreadsheet;
        }

        var header = rows[0];
        var keyIndex = FindColumn(header, "key");
        var contextIndex = FindColumn(header, "context");
        var englishIndex = FindColumn(header, "en");
        var notesIndex = FindColumn(header, "notes");
        var localeIndexes = new List<(string Locale, int Index)>();

        for (var index = 0; index < header.Count; index++)
        {
            var name = header[index].Trim();
            if (string.IsNullOrWhiteSpace(name) || FixedColumns.Contains(name, StringComparer.OrdinalIgnoreCase))
            {
                continue;
            }

            spreadsheet._localeColumns.Add(NormalizeLocale(name));
            localeIndexes.Add((NormalizeLocale(name), index));
        }

        for (var rowIndex = 1; rowIndex < rows.Count; rowIndex++)
        {
            var columns = rows[rowIndex];
            var key = Get(columns, keyIndex);
            if (string.IsNullOrWhiteSpace(key))
            {
                continue;
            }

            var row = new LocalizationSpreadsheetRow
            {
                Key = key.Trim(),
                Context = Get(columns, contextIndex),
                English = Get(columns, englishIndex),
                Notes = Get(columns, notesIndex)
            };

            foreach (var (locale, index) in localeIndexes)
            {
                row.Translations[locale] = Get(columns, index);
            }

            spreadsheet._rows.Add(row);
        }

        return spreadsheet;
    }

    public void Save(string projectPath = DefaultProjectPath)
    {
        var path = Globalize(projectPath);
        var directory = Path.GetDirectoryName(path);
        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory);
        }

        File.WriteAllText(path, ToCsv(), Encoding.UTF8);
    }

    public string? TryGetTranslation(string key, string locale)
    {
        var normalizedLocale = NormalizeLocale(locale);
        foreach (var row in _rows)
        {
            if (string.Equals(row.Key, key, StringComparison.Ordinal)
                && row.Translations.TryGetValue(normalizedLocale, out var value)
                && !string.IsNullOrWhiteSpace(value))
            {
                return value;
            }
        }

        return null;
    }

    public void SetTranslation(string key, string locale, string value)
    {
        var normalizedLocale = NormalizeLocale(locale);
        if (!_localeColumns.Contains(normalizedLocale, StringComparer.OrdinalIgnoreCase))
        {
            _localeColumns.Add(normalizedLocale);
        }

        var row = _rows.FirstOrDefault(row => string.Equals(row.Key, key, StringComparison.Ordinal));
        if (row == null)
        {
            row = new LocalizationSpreadsheetRow { Key = key };
            _rows.Add(row);
        }

        row.Translations[normalizedLocale] = value;
    }

    public ValidationResult ValidateAgainst(ContentCatalog catalog)
    {
        var result = ValidationResult.Valid();
        ValidateStructure(result);

        var source = EnglishLocalizationSource.Build(catalog);
        var sourceKeys = source.Keys.ToHashSet(StringComparer.Ordinal);
        var rowKeys = new HashSet<string>(StringComparer.Ordinal);

        foreach (var row in _rows)
        {
            if (!rowKeys.Add(row.Key))
            {
                result.AddError("localization.duplicate_spreadsheet_key", $"Localization spreadsheet has duplicate key '{row.Key}'.");
                continue;
            }

            if (!source.TryGetValue(row.Key, out var sourceEnglish))
            {
                result.AddWarning("localization.stale_spreadsheet_key", $"Localization spreadsheet key '{row.Key}' is not in current source text.");
                continue;
            }

            if (!string.Equals(row.English, sourceEnglish, StringComparison.Ordinal))
            {
                result.AddWarning("localization.source_text_changed", $"Localization source text changed for '{row.Key}'. Re-export before sending to translators.");
            }

            ValidateTranslationPlaceholders(result, row, sourceEnglish);
        }

        foreach (var key in sourceKeys)
        {
            if (!rowKeys.Contains(key))
            {
                result.AddWarning("localization.spreadsheet_missing_source_key", $"Localization spreadsheet is missing source key '{key}'. Re-export the source CSV.");
            }
        }

        return result;
    }

    public void MergeTranslationsFrom(LocalizationSpreadsheet existing)
    {
        _localeColumns.Clear();
        foreach (var locale in existing.LocaleColumns)
        {
            if (!_localeColumns.Contains(locale, StringComparer.OrdinalIgnoreCase))
            {
                _localeColumns.Add(locale);
            }
        }

        var existingByKey = existing.Rows.ToDictionary(row => row.Key, StringComparer.Ordinal);
        foreach (var row in _rows)
        {
            if (!existingByKey.TryGetValue(row.Key, out var existingRow))
            {
                continue;
            }

            foreach (var locale in _localeColumns)
            {
                if (existingRow.Translations.TryGetValue(locale, out var translation))
                {
                    row.Translations[locale] = translation;
                }
            }
        }
    }

    public string ToCsv()
    {
        var columns = FixedColumns.Concat(_localeColumns).ToArray();
        var builder = new StringBuilder();
        AppendCsvRow(builder, columns);
        foreach (var row in _rows.OrderBy(row => row.Key, StringComparer.Ordinal))
        {
            var values = new List<string>
            {
                row.Key,
                row.Context,
                row.English,
                row.Notes
            };

            foreach (var locale in _localeColumns)
            {
                values.Add(row.Translations.TryGetValue(locale, out var translation) ? translation : string.Empty);
            }

            AppendCsvRow(builder, values);
        }

        return builder.ToString();
    }

    public static bool ProjectFileExists(string projectPath)
    {
        return File.Exists(Globalize(projectPath));
    }

    public static string NormalizeLocale(string locale)
    {
        return string.IsNullOrWhiteSpace(locale)
            ? LocalizationService.DefaultLocale
            : locale.Trim().Replace('-', '_').ToLowerInvariant();
    }

    private void ValidateStructure(ValidationResult result)
    {
        if (_rows.Count == 0)
        {
            result.AddInfo("localization.no_spreadsheet", "No localization spreadsheet exists yet. Export the English source CSV before adding translations.");
            return;
        }

        var duplicateLocales = _localeColumns
            .GroupBy(locale => locale, StringComparer.OrdinalIgnoreCase)
            .Where(group => group.Count() > 1)
            .Select(group => group.Key);
        foreach (var locale in duplicateLocales)
        {
            result.AddError("localization.duplicate_locale_column", $"Localization spreadsheet has duplicate locale column '{locale}'.");
        }
    }

    private static void ValidateTranslationPlaceholders(
        ValidationResult result,
        LocalizationSpreadsheetRow row,
        string sourceEnglish)
    {
        var sourcePlaceholders = PlaceholderUtility.Extract(sourceEnglish);
        foreach (var (locale, translation) in row.Translations)
        {
            if (string.IsNullOrWhiteSpace(translation))
            {
                continue;
            }

            var translationPlaceholders = PlaceholderUtility.Extract(translation);
            if (!sourcePlaceholders.SetEquals(translationPlaceholders))
            {
                result.AddError(
                    "localization.placeholder_mismatch",
                    $"Locale '{locale}' translation for '{row.Key}' must preserve placeholders: source=[{string.Join(", ", sourcePlaceholders)}] translation=[{string.Join(", ", translationPlaceholders)}].");
            }
        }
    }

    private static string Globalize(string projectPath)
    {
        if (projectPath.StartsWith("user://", StringComparison.OrdinalIgnoreCase))
        {
            return RuntimePathResolver.GlobalizeUserPath(projectPath);
        }

        return projectPath.StartsWith("res://", StringComparison.OrdinalIgnoreCase)
            ? ProjectSettings.GlobalizePath(projectPath)
            : Path.GetFullPath(projectPath);
    }

    private static int FindColumn(IReadOnlyList<string> header, string column)
    {
        for (var index = 0; index < header.Count; index++)
        {
            if (string.Equals(header[index].Trim(), column, StringComparison.OrdinalIgnoreCase))
            {
                return index;
            }
        }

        return -1;
    }

    private static string Get(IReadOnlyList<string> columns, int index)
    {
        return index >= 0 && index < columns.Count ? columns[index] : string.Empty;
    }

    private static void AppendCsvRow(StringBuilder builder, IEnumerable<string> values)
    {
        var first = true;
        foreach (var value in values)
        {
            if (!first)
            {
                builder.Append(',');
            }

            first = false;
            AppendCsvValue(builder, value);
        }

        builder.AppendLine();
    }

    private static void AppendCsvValue(StringBuilder builder, string value)
    {
        var safe = value ?? string.Empty;
        if (safe.Contains('"') || safe.Contains(',') || safe.Contains('\n') || safe.Contains('\r'))
        {
            builder.Append('"');
            builder.Append(safe.Replace("\"", "\"\"", StringComparison.Ordinal));
            builder.Append('"');
            return;
        }

        builder.Append(safe);
    }

    private static List<List<string>> ParseCsv(string text)
    {
        var rows = new List<List<string>>();
        var row = new List<string>();
        var field = new StringBuilder();
        var inQuotes = false;

        for (var index = 0; index < text.Length; index++)
        {
            var character = text[index];
            if (inQuotes)
            {
                if (character == '"')
                {
                    if (index + 1 < text.Length && text[index + 1] == '"')
                    {
                        field.Append('"');
                        index++;
                    }
                    else
                    {
                        inQuotes = false;
                    }
                }
                else
                {
                    field.Append(character);
                }

                continue;
            }

            if (character == '"')
            {
                inQuotes = true;
            }
            else if (character == ',')
            {
                row.Add(field.ToString());
                field.Clear();
            }
            else if (character == '\n')
            {
                row.Add(field.ToString());
                field.Clear();
                rows.Add(row);
                row = new List<string>();
            }
            else if (character != '\r')
            {
                field.Append(character);
            }
        }

        if (field.Length > 0 || row.Count > 0)
        {
            row.Add(field.ToString());
            rows.Add(row);
        }

        return rows;
    }
}
