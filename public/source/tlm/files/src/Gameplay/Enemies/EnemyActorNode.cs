using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.StatusEffects;

namespace TheLastMage.Gameplay.Enemies;

public partial class EnemyActorNode : Node3D
{
    private static readonly Color DamagedColor = new(1f, 0.12f, 0.04f, 1f);
    private static readonly Color DeadColor = new(0.12f, 0.02f, 0.02f, 1f);

    private Node3D? _visualRoot;
    private MeshInstance3D? _bodyMesh;
    private Node? _temporaryFormInstance;
    private PackedScene? _temporaryFormScene;
    private AnimationPlayer? _animationPlayer;
    private AnimationTree? _animationTree;
    private Label3D? _roleLabel;
    private Label3D? _statusLabel;
    private Label3D? _telegraphLabel;
    private StandardMaterial3D? _material;
    private EnemyPlaceholderVisualProfile _visualProfile = EnemyPlaceholderVisualProfile.Default;
    private float _animationTime;
    private float _damageFlashRemaining;
    private float _deathTimer;
    private bool _isDead;
    private bool _isMoving;

    [Export] public string IdleAnimationName { get; set; } = "idle";

    [Export] public string MoveAnimationName { get; set; } = "move";

    [Export] public string HurtAnimationName { get; set; } = "hit";

    [Export] public string AttackAnimationName { get; set; } = "attack";

    [Export] public string DeathAnimationName { get; set; } = "death";

    public EntityId BoundEntityId { get; private set; } = EntityId.None;

    public void Bind(EntityId entityId, ContentId archetypeId, string role, IReadOnlyList<TagId> tags)
    {
        BoundEntityId = entityId;
        _visualProfile = EnemyPlaceholderVisualProfile.For(archetypeId, role, tags);
        ApplyVisualProfile();
    }

    public override void _Ready()
    {
        EnsureSceneReferences();
        ApplyVisualProfile();
        TryPlayAnimation(IdleAnimationName);
    }

    public void SetMoving(bool isMoving)
    {
        if (_isDead || _isMoving == isMoving)
        {
            return;
        }

        _isMoving = isMoving;
        TryPlayAnimation(_isMoving ? MoveAnimationName : IdleAnimationName);
    }

    public override void _Process(double delta)
    {
        if (_visualRoot == null)
        {
            return;
        }

        _animationTime += (float)delta;
        if (!_isDead)
        {
            _visualRoot.Position = new Vector3(0f, MathF.Sin(_animationTime * 8f) * 0.04f, 0f);
        }

        if (_damageFlashRemaining > 0f)
        {
            _damageFlashRemaining -= (float)delta;
            if (_damageFlashRemaining <= 0f && !_isDead && _material != null)
            {
                _material.AlbedoColor = _visualProfile.AliveColor;
            }
        }

        if (_isDead)
        {
            _deathTimer -= (float)delta;
            if (_deathTimer <= 0f)
            {
                QueueFree();
            }
        }
    }

    public void FlashDamage()
    {
        if (_isDead)
        {
            return;
        }

        _damageFlashRemaining = 0.16f;
        if (_material != null)
        {
            _material.AlbedoColor = DamagedColor;
        }

        TryPlayAnimation(HurtAnimationName);
    }

    public void MarkDead()
    {
        _isDead = true;
        _deathTimer = 0.35f;
        _damageFlashRemaining = 0f;
        ClearTemporaryForm();
        SetStatusOverlay(string.Empty);
        SetAbilityTelegraph(string.Empty);
        if (_material != null)
        {
            _material.AlbedoColor = DeadColor;
        }

        TryPlayAnimation(DeathAnimationName);
    }

    public void SetAbilityTelegraph(string text)
    {
        if (_telegraphLabel == null)
        {
            return;
        }

        _telegraphLabel.Text = _isDead ? string.Empty : text;
        if (!string.IsNullOrWhiteSpace(text))
        {
            TryPlayAnimation(AttackAnimationName);
        }
    }

    public void SetStatusOverlay(string statusKind)
    {
        if (_statusLabel == null)
        {
            return;
        }

        if (string.IsNullOrWhiteSpace(statusKind) || _isDead)
        {
            _statusLabel.Text = string.Empty;
            return;
        }

        var rule = StatusEffectRules.GetOrDefault(statusKind);
        _statusLabel.Text = rule.DisplayText;
        _statusLabel.Modulate = GetStatusColor(statusKind);
    }

    public void SetTemporaryForm(string formId, PackedScene? presentationScene)
    {
        if (_isDead)
        {
            return;
        }

        if (string.IsNullOrWhiteSpace(formId))
        {
            ClearTemporaryForm();
            return;
        }

        if (_roleLabel != null)
        {
            _roleLabel.Text = formId.Contains("among", StringComparison.OrdinalIgnoreCase) ? "SUS" : formId.ToUpperInvariant();
            _roleLabel.Modulate = new Color(1f, 0.95f, 0.95f, 1f);
        }

        if (_visualRoot == null)
        {
            return;
        }

        if (presentationScene != null)
        {
            if (!ReferenceEquals(_temporaryFormScene, presentationScene))
            {
                ClearTemporaryFormInstance();
                _temporaryFormScene = presentationScene;
                _temporaryFormInstance = presentationScene.Instantiate();
                _visualRoot.AddChild(_temporaryFormInstance);
            }

            if (_bodyMesh != null)
            {
                _bodyMesh.Visible = false;
            }

            return;
        }

        ClearTemporaryFormInstance();
        if (_bodyMesh != null)
        {
            _bodyMesh.Visible = true;
        }

        _visualRoot.Scale = new Vector3(0.8f, 0.8f, 0.8f);
        if (_material != null && _damageFlashRemaining <= 0f)
        {
            _material.AlbedoColor = new Color(0.92f, 0.08f, 0.06f, 1f);
        }
    }

    public void ClearTemporaryForm()
    {
        ClearTemporaryFormInstance();
        if (_bodyMesh != null)
        {
            _bodyMesh.Visible = true;
        }

        ApplyVisualProfile();
    }

    private static StandardMaterial3D CreateMaterial(Color color)
    {
        return new StandardMaterial3D
        {
            AlbedoColor = color,
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
        };
    }

    private void EnsureSceneReferences()
    {
        _visualRoot = GetNodeOrNull<Node3D>("VisualRoot");
        if (_visualRoot == null)
        {
            _visualRoot = new Node3D { Name = "VisualRoot" };
            AddChild(_visualRoot);
        }

        var modelRoot = _visualRoot.GetNodeOrNull<Node3D>("ModelRoot");
        if (modelRoot == null)
        {
            modelRoot = new Node3D { Name = "ModelRoot" };
            _visualRoot.AddChild(modelRoot);
        }

        _bodyMesh = modelRoot.GetNodeOrNull<MeshInstance3D>("PlaceholderBody")
            ?? _visualRoot.GetNodeOrNull<MeshInstance3D>("PlaceholderBody");
        if (_bodyMesh == null)
        {
            _bodyMesh = new MeshInstance3D
            {
                Name = "PlaceholderBody",
                Mesh = new CapsuleMesh
                {
                    Radius = 0.35f,
                    Height = 1.8f
                },
                MaterialOverride = CreateMaterial(_visualProfile.AliveColor)
            };
            modelRoot.AddChild(_bodyMesh);
        }

        if (_bodyMesh.MaterialOverride is StandardMaterial3D existingMaterial)
        {
            _material = existingMaterial.Duplicate() as StandardMaterial3D ?? existingMaterial;
            _bodyMesh.MaterialOverride = _material;
        }
        else
        {
            _material = CreateMaterial(_visualProfile.AliveColor);
            _bodyMesh.MaterialOverride = _material;
        }
        _animationPlayer = GetNodeOrNull<AnimationPlayer>("AnimationPlayer");
        if (_animationPlayer == null)
        {
            _animationPlayer = new AnimationPlayer { Name = "AnimationPlayer" };
            AddChild(_animationPlayer);
        }

        _animationTree = GetNodeOrNull<AnimationTree>("AnimationTree");
        if (_animationTree == null)
        {
            _animationTree = new AnimationTree { Name = "AnimationTree" };
            AddChild(_animationTree);
        }

        _roleLabel = GetNodeOrNull<Label3D>("RoleLabel");
        if (_roleLabel == null)
        {
            _roleLabel = new Label3D
            {
                Name = "RoleLabel",
                Text = string.Empty,
                Position = new Vector3(0f, 1.95f, 0f),
                FontSize = 28,
                PixelSize = 0.02f,
                OutlineSize = 5,
                NoDepthTest = true,
                Billboard = BaseMaterial3D.BillboardModeEnum.Enabled
            };
            AddChild(_roleLabel);
        }

        _statusLabel = GetNodeOrNull<Label3D>("StatusLabel");
        if (_statusLabel == null)
        {
            _statusLabel = new Label3D
            {
                Name = "StatusLabel",
                Text = string.Empty,
                Position = new Vector3(0f, 1.55f, 0f),
                FontSize = 18,
                PixelSize = 0.018f,
                OutlineSize = 4,
                Modulate = Colors.White,
                NoDepthTest = true,
                Billboard = BaseMaterial3D.BillboardModeEnum.Enabled
            };
            AddChild(_statusLabel);
        }

        _telegraphLabel = GetNodeOrNull<Label3D>("TelegraphLabel");
        if (_telegraphLabel == null)
        {
            _telegraphLabel = new Label3D
            {
                Name = "TelegraphLabel",
                Text = string.Empty,
                Position = new Vector3(0f, 2.35f, 0f),
                FontSize = 22,
                PixelSize = 0.02f,
                OutlineSize = 5,
                Modulate = new Color(1f, 0.86f, 0.24f, 1f),
                NoDepthTest = true,
                Billboard = BaseMaterial3D.BillboardModeEnum.Enabled
            };
            AddChild(_telegraphLabel);
        }

        if (GetNodeOrNull<Area3D>("EnemyHurtbox") == null)
        {
            var hurtbox = new Area3D { Name = "EnemyHurtbox" };
            hurtbox.AddChild(new CollisionShape3D
            {
                Shape = new CapsuleShape3D
                {
                    Radius = 0.45f,
                    Height = 1.8f
                }
            });
            AddChild(hurtbox);
        }
    }

    private void TryPlayAnimation(string animationName)
    {
        if (_animationPlayer == null
            || string.IsNullOrWhiteSpace(animationName)
            || !_animationPlayer.HasAnimation(animationName)
            || _animationPlayer.CurrentAnimation == animationName)
        {
            return;
        }

        _animationPlayer.Play(animationName);
    }

    private void ApplyVisualProfile()
    {
        if (_visualRoot != null)
        {
            _visualRoot.Scale = _visualProfile.Scale;
        }

        if (_material != null && !_isDead && _damageFlashRemaining <= 0f)
        {
            _material.AlbedoColor = _visualProfile.AliveColor;
        }

        if (_roleLabel != null)
        {
            _roleLabel.Text = _visualProfile.RoleMarker;
            _roleLabel.Modulate = _visualProfile.LabelColor;
        }
    }

    private void ClearTemporaryFormInstance()
    {
        if (_temporaryFormInstance != null)
        {
            _temporaryFormInstance.QueueFree();
            _temporaryFormInstance = null;
        }

        _temporaryFormScene = null;
    }

    private static Color GetStatusColor(string statusKind)
    {
        if (string.Equals(statusKind, StatusEffectRules.Slow, StringComparison.OrdinalIgnoreCase))
        {
            return new Color(0.45f, 0.85f, 1f, 1f);
        }

        if (string.Equals(statusKind, StatusEffectRules.Burn, StringComparison.OrdinalIgnoreCase))
        {
            return new Color(1f, 0.32f, 0.08f, 1f);
        }

        return new Color(1f, 0.9f, 0.25f, 1f);
    }

    private readonly record struct EnemyPlaceholderVisualProfile(
        Color AliveColor,
        Color LabelColor,
        Vector3 Scale,
        string RoleMarker)
    {
        public static readonly EnemyPlaceholderVisualProfile Default = new(
            new Color(0.92f, 0.92f, 0.88f, 1f),
            new Color(0.98f, 0.98f, 0.9f, 1f),
            Vector3.One,
            string.Empty);

        public static EnemyPlaceholderVisualProfile For(ContentId archetypeId, string role, IReadOnlyList<TagId> tags)
        {
            if (HasTag(tags, "enemy.rank.boss")
                || HasTag(tags, "enemy.rank.champion")
                || string.Equals(role, "Boss", StringComparison.OrdinalIgnoreCase)
                || archetypeId.Value.Contains("champion", StringComparison.OrdinalIgnoreCase)
                || archetypeId.Value.Contains("boss", StringComparison.OrdinalIgnoreCase))
            {
                return new EnemyPlaceholderVisualProfile(
                    new Color(0.98f, 0.72f, 0.18f, 1f),
                    new Color(1f, 0.9f, 0.35f, 1f),
                    new Vector3(1.65f, 1.45f, 1.65f),
                    "CHAMPION");
            }

            if (HasTag(tags, "enemy.trait.defense_pressure")
                || string.Equals(role, "Brute", StringComparison.OrdinalIgnoreCase)
                || archetypeId.Value.Contains("brute", StringComparison.OrdinalIgnoreCase))
            {
                return new EnemyPlaceholderVisualProfile(
                    new Color(0.72f, 0.28f, 0.18f, 1f),
                    new Color(1f, 0.48f, 0.25f, 1f),
                    new Vector3(1.35f, 1.22f, 1.35f),
                    "BRUTE");
            }

            if (HasTag(tags, "enemy.trait.armored")
                || string.Equals(role, "Soldier", StringComparison.OrdinalIgnoreCase)
                || archetypeId.Value.Contains("soldier", StringComparison.OrdinalIgnoreCase))
            {
                return new EnemyPlaceholderVisualProfile(
                    new Color(0.58f, 0.58f, 0.52f, 1f),
                    new Color(0.86f, 0.86f, 0.78f, 1f),
                    new Vector3(1.05f, 1.08f, 1.05f),
                    "SOLDIER");
            }

            if (HasTag(tags, "enemy.attack.ranged")
                || string.Equals(role, "Ranged", StringComparison.OrdinalIgnoreCase)
                || archetypeId.Value.Contains("archer", StringComparison.OrdinalIgnoreCase))
            {
                return new EnemyPlaceholderVisualProfile(
                    new Color(0.28f, 0.55f, 0.88f, 1f),
                    new Color(0.45f, 0.8f, 1f, 1f),
                    new Vector3(0.9f, 1.05f, 0.9f),
                    "RANGED");
            }

            if (HasTag(tags, "enemy.trait.anti_magic")
                || archetypeId.Value.Contains("acolyte", StringComparison.OrdinalIgnoreCase))
            {
                return new EnemyPlaceholderVisualProfile(
                    new Color(0.65f, 0.34f, 0.92f, 1f),
                    new Color(0.9f, 0.65f, 1f, 1f),
                    new Vector3(0.95f, 1.12f, 0.95f),
                    "ANTI");
            }

            return Default;
        }

        private static bool HasTag(IReadOnlyList<TagId> tags, string tag)
        {
            return GameplayTagPath.MatchesAny(TagId.From(tag), tags);
        }
    }
}
