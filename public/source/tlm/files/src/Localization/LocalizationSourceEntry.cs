namespace TheLastMage.Localization;

public sealed record LocalizationSourceEntry(
    string Key,
    string Context,
    string English,
    string Notes);
