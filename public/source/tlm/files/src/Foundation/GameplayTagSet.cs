namespace TheLastMage.Foundation;

public sealed class GameplayTagSet : IReadOnlyList<TagId>
{
    private static readonly TagId[] EmptyTags = Array.Empty<TagId>();

    private readonly TagId[] _tags;
    private readonly HashSet<string> _selfAndAncestors;

    public static readonly GameplayTagSet Empty = new(EmptyTags);

    private GameplayTagSet(TagId[] tags)
    {
        _tags = tags;
        _selfAndAncestors = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        for (var i = 0; i < tags.Length; i++)
        {
            foreach (var tag in GameplayTagPath.GetSelfAndAncestors(tags[i].Value))
            {
                _selfAndAncestors.Add(tag);
            }
        }
    }

    public int Count => _tags.Length;

    public TagId this[int index] => _tags[index];

    public static GameplayTagSet From(IEnumerable<TagId> tags)
    {
        var normalizedTags = tags
            .Where(tag => tag.IsValid)
            .Distinct()
            .ToArray();
        return normalizedTags.Length == 0 ? Empty : new GameplayTagSet(normalizedTags);
    }

    public bool MatchesTarget(TagId targetTag)
    {
        return !targetTag.IsValid || _selfAndAncestors.Contains(targetTag.Value);
    }

    public IEnumerator<TagId> GetEnumerator()
    {
        return ((IEnumerable<TagId>)_tags).GetEnumerator();
    }

    System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
    {
        return GetEnumerator();
    }
}
