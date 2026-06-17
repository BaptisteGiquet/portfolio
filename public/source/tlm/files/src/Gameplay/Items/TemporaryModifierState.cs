using TheLastMage.Foundation;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Items;

public sealed class TemporaryModifierState
{
    public TemporaryModifierState(
        ContentId sourceId,
        StatId statId,
        ModifierOp operation,
        float value,
        int priority,
        TagId targetTag,
        float remainingSeconds)
    {
        SourceId = sourceId;
        StatId = statId;
        Operation = operation;
        Value = value;
        Priority = priority;
        TargetTag = targetTag;
        RemainingSeconds = remainingSeconds;
    }

    public ContentId SourceId { get; }

    public StatId StatId { get; }

    public ModifierOp Operation { get; }

    public float Value { get; }

    public int Priority { get; }

    public TagId TargetTag { get; }

    public float RemainingSeconds { get; private set; }

    public bool IsExpired => RemainingSeconds <= 0f;

    public void Tick(float delta)
    {
        RemainingSeconds = MathF.Max(0f, RemainingSeconds - MathF.Max(0f, delta));
    }
}
