namespace TheLastMage.Foundation.Random;

public sealed class RandomService
{
    private readonly int _seed;
    private readonly Dictionary<RandomStreamId, System.Random> _streams = new();

    public RandomService(int seed)
    {
        _seed = seed;
    }

    public int Seed => _seed;

    public System.Random Stream(RandomStreamId streamId)
    {
        if (!_streams.TryGetValue(streamId, out var random))
        {
            random = new System.Random(DeriveSeed(_seed, streamId.ToString()));
            _streams.Add(streamId, random);
        }

        return random;
    }

    private static int DeriveSeed(int seed, string streamName)
    {
        unchecked
        {
            var hash = 2166136261;
            foreach (var character in streamName)
            {
                hash ^= character;
                hash *= 16777619;
            }

            return (int)(hash ^ (uint)seed);
        }
    }
}
