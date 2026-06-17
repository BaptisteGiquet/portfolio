using TheLastMage.Foundation;

namespace TheLastMage.Localization;

public static class LocalizationKeys
{
    public const string MissingMarkerPrefix = "!!";
    public const string MissingMarkerSuffix = "!!";

    public static string ContentName(string kind, ContentId id)
    {
        return ContentField(kind, id, "name");
    }

    public static string ContentDescription(string kind, ContentId id)
    {
        return ContentField(kind, id, "description");
    }

    public static string ContentField(string kind, ContentId id, string field)
    {
        return $"content.{NormalizeSegment(kind)}.{NormalizeSegment(id.Value)}.{NormalizeSegment(field)}";
    }

    public static string Ui(string section, string name)
    {
        return $"ui.{NormalizeSegment(section)}.{NormalizeSegment(name)}";
    }

    private static string NormalizeSegment(string value)
    {
        return value.Trim().Replace(' ', '_').Replace('-', '_').ToLowerInvariant();
    }
}
