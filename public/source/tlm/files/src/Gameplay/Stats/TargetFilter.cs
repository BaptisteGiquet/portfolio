using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Stats;

public readonly record struct TargetFilter(TagId RequiredTag)
{
    public static readonly TargetFilter Any = new(new TagId(string.Empty));

    public bool Matches(IReadOnlyCollection<TagId> tags)
    {
        return GameplayTagPath.MatchesAny(RequiredTag, tags);
    }
}
