namespace TheLastMage.Presentation.Feedback;

public sealed class FeedbackAudioRules
{
    private readonly Dictionary<string, string> _groups = new(StringComparer.OrdinalIgnoreCase)
    {
        ["damage"] = "impacts",
        ["death"] = "horde_deaths",
        ["enemy_ability"] = "specialist_calls",
        ["enemy_telegraph"] = "specialist_calls",
        ["aoe"] = "spell_areas",
        ["status"] = "status_layers",
        ["summon"] = "summons",
        ["ui"] = "ui"
    };

    public string ResolveGroup(string feedbackKind)
    {
        foreach (var pair in _groups)
        {
            if (feedbackKind.StartsWith(pair.Key, StringComparison.OrdinalIgnoreCase))
            {
                return pair.Value;
            }
        }

        return "spells";
    }

    public string BuildSummary()
    {
        return "groups: impacts, horde_deaths, specialist_calls, spell_areas, status_layers, summons, ui";
    }
}
