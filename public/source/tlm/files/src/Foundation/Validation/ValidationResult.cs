namespace TheLastMage.Foundation.Validation;

public sealed class ValidationResult
{
    private readonly List<ValidationIssue> _issues = new();

    public IReadOnlyList<ValidationIssue> Issues => _issues;

    public bool HasErrors => _issues.Any(issue => issue.Severity == ValidationSeverity.Error);

    public bool IsValid => !HasErrors;

    public void Add(ValidationSeverity severity, string code, string message, string? path = null)
    {
        _issues.Add(new ValidationIssue(severity, code, message, path));
    }

    public void AddError(string code, string message, string? path = null)
    {
        Add(ValidationSeverity.Error, code, message, path);
    }

    public void AddWarning(string code, string message, string? path = null)
    {
        Add(ValidationSeverity.Warning, code, message, path);
    }

    public void AddInfo(string code, string message, string? path = null)
    {
        Add(ValidationSeverity.Info, code, message, path);
    }

    public void Merge(ValidationResult other)
    {
        _issues.AddRange(other.Issues);
    }

    public static ValidationResult Valid()
    {
        return new ValidationResult();
    }
}
