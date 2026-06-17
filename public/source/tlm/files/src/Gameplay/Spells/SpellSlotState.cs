using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Spells;

public sealed class SpellSlotState
{
    public ContentId SpellId { get; private set; } = ContentId.From(string.Empty);

    public float CooldownRemainingSeconds { get; private set; }

    public bool HasSpell => SpellId.IsValid;

    public bool IsReady => CooldownRemainingSeconds <= 0f;

    public void Assign(ContentId spellId)
    {
        SpellId = spellId;
        CooldownRemainingSeconds = 0f;
    }

    public void Clear()
    {
        SpellId = ContentId.From(string.Empty);
        CooldownRemainingSeconds = 0f;
    }

    public void StartCooldown(float seconds)
    {
        CooldownRemainingSeconds = MathF.Max(0f, seconds);
    }

    public void Restore(ContentId spellId, float cooldownRemainingSeconds)
    {
        SpellId = spellId;
        CooldownRemainingSeconds = MathF.Max(0f, cooldownRemainingSeconds);
    }

    public void Tick(float delta)
    {
        if (CooldownRemainingSeconds <= 0f)
        {
            return;
        }

        CooldownRemainingSeconds = MathF.Max(0f, CooldownRemainingSeconds - delta);
    }
}
