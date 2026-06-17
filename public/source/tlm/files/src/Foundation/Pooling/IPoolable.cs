namespace TheLastMage.Foundation.Pooling;

public interface IPoolable
{
    void OnPoolGet();

    void OnPoolReturn();
}
