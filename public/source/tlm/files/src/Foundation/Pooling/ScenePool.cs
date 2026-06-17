using Godot;

namespace TheLastMage.Foundation.Pooling;

public sealed class ScenePool<TNode>
    where TNode : Node
{
    private readonly PackedScene _scene;
    private readonly Node _parent;
    private readonly Stack<TNode> _available = new();

    public ScenePool(PackedScene scene, Node parent)
    {
        _scene = scene;
        _parent = parent;
    }

    public int AvailableCount => _available.Count;

    public int TotalCreated { get; private set; }

    public TNode Get()
    {
        var node = _available.Count > 0 ? _available.Pop() : Create();
        if (node.GetParent() == null)
        {
            _parent.AddChild(node);
        }

        node.ProcessMode = Node.ProcessModeEnum.Inherit;
        if (node is IPoolable poolable)
        {
            poolable.OnPoolGet();
        }

        return node;
    }

    public void Return(TNode node)
    {
        if (node is IPoolable poolable)
        {
            poolable.OnPoolReturn();
        }

        if (node.GetParent() != null)
        {
            node.GetParent().RemoveChild(node);
        }

        node.ProcessMode = Node.ProcessModeEnum.Disabled;
        _available.Push(node);
    }

    private TNode Create()
    {
        TotalCreated++;
        return _scene.Instantiate<TNode>();
    }
}
