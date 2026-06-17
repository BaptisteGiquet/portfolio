using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Presentation.Feedback;

public readonly record struct FeedbackRequest(string Kind, ContentId SourceId, Vector3 Position, int Priority);
