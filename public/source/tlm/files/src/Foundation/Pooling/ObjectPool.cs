namespace TheLastMage.Foundation.Pooling;

public sealed class ObjectPool<T>
    where T : class
{
    private readonly Func<T> _factory;
    private readonly Action<T>? _onGet;
    private readonly Action<T>? _onReturn;
    private readonly Stack<T> _available = new();

    public ObjectPool(Func<T> factory, Action<T>? onGet = null, Action<T>? onReturn = null)
    {
        _factory = factory;
        _onGet = onGet;
        _onReturn = onReturn;
    }

    public int AvailableCount => _available.Count;

    public int TotalCreated { get; private set; }

    public T Get()
    {
        var item = _available.Count > 0 ? _available.Pop() : Create();
        _onGet?.Invoke(item);
        if (item is IPoolable poolable)
        {
            poolable.OnPoolGet();
        }

        return item;
    }

    public void Return(T item)
    {
        if (item is IPoolable poolable)
        {
            poolable.OnPoolReturn();
        }

        _onReturn?.Invoke(item);
        _available.Push(item);
    }

    private T Create()
    {
        TotalCreated++;
        return _factory();
    }
}
