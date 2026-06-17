namespace TheLastMage.Foundation.Events;

public sealed class GameEventBus
{
    private sealed class HandlerEntry
    {
        public required SubscriptionToken Token { get; init; }
        public required Action<object> Invoke { get; init; }
    }

    private readonly Dictionary<Type, List<HandlerEntry>> _handlers = new();
    private readonly Dictionary<SubscriptionToken, Type> _tokenTypes = new();

    public event Action<Type>? EventPublished;

    public void Publish<TEvent>(in TEvent gameEvent)
        where TEvent : struct, IGameEvent
    {
        var eventType = typeof(TEvent);
        EventPublished?.Invoke(eventType);

        if (!_handlers.TryGetValue(eventType, out var handlers))
        {
            return;
        }

        var snapshot = handlers.ToArray();
        foreach (var handler in snapshot)
        {
            handler.Invoke(gameEvent);
        }
    }

    public SubscriptionToken Subscribe<TEvent>(Action<TEvent> handler)
        where TEvent : struct, IGameEvent
    {
        var eventType = typeof(TEvent);
        var token = new SubscriptionToken(Guid.NewGuid());
        var entry = new HandlerEntry
        {
            Token = token,
            Invoke = value => handler((TEvent)value)
        };

        if (!_handlers.TryGetValue(eventType, out var handlers))
        {
            handlers = new List<HandlerEntry>();
            _handlers.Add(eventType, handlers);
        }

        handlers.Add(entry);
        _tokenTypes[token] = eventType;
        return token;
    }

    public void Unsubscribe(SubscriptionToken token)
    {
        if (!_tokenTypes.Remove(token, out var eventType))
        {
            return;
        }

        if (_handlers.TryGetValue(eventType, out var handlers))
        {
            handlers.RemoveAll(entry => entry.Token.Equals(token));
        }
    }

    public void Clear()
    {
        _handlers.Clear();
        _tokenTypes.Clear();
    }
}
