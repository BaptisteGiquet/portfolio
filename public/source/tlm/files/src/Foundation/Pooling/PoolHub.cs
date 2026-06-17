namespace TheLastMage.Foundation.Pooling;

public sealed class PoolHub
{
    private readonly Dictionary<string, object> _pools = new();

    public void Register(string key, object pool)
    {
        _pools[key] = pool;
    }

    public bool TryGet<TPool>(string key, out TPool? pool)
        where TPool : class
    {
        if (_pools.TryGetValue(key, out var value) && value is TPool typed)
        {
            pool = typed;
            return true;
        }

        pool = null;
        return false;
    }

    public IReadOnlyDictionary<string, object> Snapshot() => _pools;
}
