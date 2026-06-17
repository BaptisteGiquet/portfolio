using Godot;

namespace TheLastMage.Gameplay.Player;

public sealed class PlayerMotor
{
    private const float Gravity = 24f;
    private const float JumpBufferSeconds = 0.12f;
    private const float CoyoteSeconds = 0.08f;
    private float _jumpBufferRemaining;
    private float _coyoteRemaining;

    public float MoveSpeed { get; set; } = 5.5f;

    public float Acceleration { get; set; } = 16f;

    public float Deceleration { get; set; } = 20f;

    public float AirAcceleration { get; set; } = 4.5f;

    public float AirDeceleration { get; set; } = 2f;

    public float JumpVelocity { get; set; } = 7f;

    public void ResetTransientInput()
    {
        _jumpBufferRemaining = 0f;
        _coyoteRemaining = 0f;
    }

    public void Move(CharacterBody3D body, Vector2 moveInput, bool jumpRequested, Basis viewBasis, double delta)
    {
        var step = (float)delta;
        if (jumpRequested)
        {
            _jumpBufferRemaining = JumpBufferSeconds;
        }
        else if (_jumpBufferRemaining > 0f)
        {
            _jumpBufferRemaining = Mathf.Max(0f, _jumpBufferRemaining - step);
        }

        var isOnFloor = body.IsOnFloor();
        if (isOnFloor)
        {
            _coyoteRemaining = CoyoteSeconds;
        }
        else if (_coyoteRemaining > 0f)
        {
            _coyoteRemaining = Mathf.Max(0f, _coyoteRemaining - step);
        }

        var forward = -viewBasis.Z;
        forward.Y = 0f;
        forward = forward.Normalized();
        var right = viewBasis.X;
        right.Y = 0f;
        right = right.Normalized();

        var desiredDirection = (right * moveInput.X + forward * moveInput.Y);
        if (desiredDirection.LengthSquared() > 1f)
        {
            desiredDirection = desiredDirection.Normalized();
        }

        var velocity = body.Velocity;
        var horizontal = new Vector3(velocity.X, 0f, velocity.Z);
        var desiredVelocity = desiredDirection * MoveSpeed;
        var hasMoveInput = desiredDirection.LengthSquared() > 0.001f;
        var rate = hasMoveInput
            ? (isOnFloor ? Acceleration : AirAcceleration)
            : (isOnFloor ? Deceleration : AirDeceleration);
        horizontal = horizontal.MoveToward(desiredVelocity, rate * step);

        velocity.X = horizontal.X;
        velocity.Z = horizontal.Z;
        if (!isOnFloor)
        {
            velocity.Y -= Gravity * step;
        }
        else if (velocity.Y < 0f)
        {
            velocity.Y = 0f;
        }

        if (_jumpBufferRemaining > 0f && _coyoteRemaining > 0f)
        {
            velocity.Y = JumpVelocity;
            _jumpBufferRemaining = 0f;
            _coyoteRemaining = 0f;
        }

        body.Velocity = velocity;
        body.MoveAndSlide();
    }
}
