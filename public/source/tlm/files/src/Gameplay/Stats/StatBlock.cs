using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Stats;

public sealed class StatBlock
{
    private readonly Dictionary<StatId, float> _baseValues = new();

    public void SetBase(StatId statId, float value)
    {
        _baseValues[statId] = value;
    }

    public float GetBase(StatId statId)
    {
        return _baseValues.GetValueOrDefault(statId);
    }
}
