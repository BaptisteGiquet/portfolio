using TheLastMage.Foundation;
using TheLastMage.Save;

namespace TheLastMage.Gameplay.Items;

public sealed class ItemKnowledgeService
{
    private readonly ProfileSaveDto _profile;
    private readonly Action<ProfileSaveDto>? _persist;
    private readonly HashSet<string> _discoveredItemIds;

    public ItemKnowledgeService(ProfileSaveDto profile, Action<ProfileSaveDto>? persist = null)
    {
        _profile = profile;
        _persist = persist;
        _discoveredItemIds = profile.DiscoveredItemIds.ToHashSet(StringComparer.Ordinal);
    }

    public bool CanReveal(ContentId itemId)
    {
        return itemId.IsValid && _discoveredItemIds.Contains(itemId.Value);
    }

    public void MarkDiscovered(ContentId itemId)
    {
        if (!itemId.IsValid || !_discoveredItemIds.Add(itemId.Value))
        {
            return;
        }

        _profile.DiscoveredItemIds.Add(itemId.Value);
        _persist?.Invoke(_profile);
    }
}
