using TheLastMage.Data.Definitions;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;
using System.Text.RegularExpressions;

namespace TheLastMage.Localization;

public static class LocalizationValidator
{
    private static readonly Regex DirectTextAssignmentPattern = new(
        @"(?:Text|PlaceholderText|TooltipText|DialogText)\s*=\s*""(?<text>[^""]*[A-Za-z][^""]*)""",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static readonly Regex DirectUiFactoryPattern = new(
        @"UiStyle\.Create(?:Label|Button)\(\s*""(?<text>[^""]*[A-Za-z][^""]*)""",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static readonly string[] ProductionUiFiles =
    {
        "src/UI/HudView.cs",
        "src/UI/MarketPanel.cs",
        "src/UI/PauseSettingsPanel.cs",
        "src/UI/OptionsMenuPanel.cs",
        "src/UI/FrontEndPanel.cs",
        "src/UI/RunViewModels.cs"
    };

    public static ValidationResult Validate(ContentCatalog catalog, bool includeRawStringScan = true)
    {
        var result = Validate(catalog, EnglishLocalizationSource.Build(catalog), includeRawStringScan);
        if (LocalizationSpreadsheet.ProjectFileExists(LocalizationSpreadsheet.DefaultProjectPath))
        {
            result.Merge(LocalizationSpreadsheet.Load(LocalizationSpreadsheet.DefaultProjectPath).ValidateAgainst(catalog));
        }

        return result;
    }

    public static ValidationResult Validate(
        ContentCatalog catalog,
        IReadOnlyDictionary<string, string> entries,
        bool includeRawStringScan = true)
    {
        var result = ValidationResult.Valid();
        ValidateRequiredKeys(result, catalog, entries);
        ValidateMissingOrBlankValues(result, entries);

        if (includeRawStringScan)
        {
            ValidateRawProductionUiStrings(result);
        }

        return result;
    }

    private static void ValidateRequiredKeys(
        ValidationResult result,
        ContentCatalog catalog,
        IReadOnlyDictionary<string, string> entries)
    {
        RequireCommon(result, entries, "spell", catalog.Spells);
        RequireCommon(result, entries, "enemy", catalog.Enemies);
        RequireCommon(result, entries, "wave", catalog.Waves);
        RequireCommon(result, entries, "defense", catalog.Defenses);
        RequireCommon(result, entries, "mage", catalog.Mages);
        RequireCommon(result, entries, "achievement", catalog.Achievements);

        foreach (var item in catalog.Items)
        {
            RequireCommon(result, entries, "item", new[] { item });
            Require(result, entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "hidden_description"));
            Require(result, entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_stat_text"));
            Require(result, entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_behavior_text"));
            Require(result, entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_effect_text"));
        }

        foreach (var key in RequiredUiKeys())
        {
            Require(result, entries, key);
        }
    }

    private static void RequireCommon<TDefinition>(
        ValidationResult result,
        IReadOnlyDictionary<string, string> entries,
        string kind,
        IEnumerable<TDefinition> definitions)
        where TDefinition : ContentDefinition
    {
        foreach (var definition in definitions)
        {
            var id = ContentId.From(definition.Id);
            Require(result, entries, LocalizationKeys.ContentName(kind, id));
            Require(result, entries, LocalizationKeys.ContentDescription(kind, id));
        }
    }

    private static void Require(ValidationResult result, IReadOnlyDictionary<string, string> entries, string key)
    {
        if (!entries.ContainsKey(key))
        {
            result.AddError("localization.missing_key", $"Missing localization key '{key}'.");
        }
    }

    private static void ValidateMissingOrBlankValues(
        ValidationResult result,
        IReadOnlyDictionary<string, string> entries)
    {
        foreach (var (key, value) in entries)
        {
            if (string.IsNullOrWhiteSpace(value) && IsRequiredVisibleKey(key))
            {
                result.AddWarning("localization.blank_value", $"Localization key '{key}' has a blank English value.");
            }
        }
    }

    private static bool IsRequiredVisibleKey(string key)
    {
        return key.EndsWith(".name", StringComparison.Ordinal)
            || key.StartsWith("ui.", StringComparison.Ordinal);
    }

    private static void ValidateRawProductionUiStrings(ValidationResult result)
    {
        var root = Directory.GetCurrentDirectory();
        foreach (var relativePath in ProductionUiFiles)
        {
            var fullPath = Path.Combine(root, relativePath.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(fullPath))
            {
                result.AddWarning("localization.raw_scan_missing_file", $"Could not scan production UI file '{relativePath}'.");
                continue;
            }

            var text = File.ReadAllText(fullPath);
            ValidateRawMatches(result, relativePath, DirectTextAssignmentPattern.Matches(text));
            ValidateRawMatches(result, relativePath, DirectUiFactoryPattern.Matches(text));
        }
    }

    private static void ValidateRawMatches(ValidationResult result, string path, MatchCollection matches)
    {
        foreach (Match match in matches)
        {
            var text = match.Groups["text"].Value;
            if (IsAllowedRawString(text))
            {
                continue;
            }

            result.AddError(
                "localization.raw_player_visible_string",
                $"Production UI file '{path}' assigns raw player-visible text '{text}'. Route it through LocalizationService.",
                path);
        }
    }

    private static bool IsAllowedRawString(string text)
    {
        return text.StartsWith("ui.", StringComparison.Ordinal)
            || text.StartsWith("content.", StringComparison.Ordinal)
            || text.StartsWith("res://", StringComparison.Ordinal);
    }

    private static IEnumerable<string> RequiredUiKeys()
    {
        return EnglishLocalizationSource.Build(new ContentCatalog()).Keys.Where(key => key.StartsWith("ui.", StringComparison.Ordinal));
    }
}
