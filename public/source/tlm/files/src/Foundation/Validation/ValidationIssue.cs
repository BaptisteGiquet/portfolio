namespace TheLastMage.Foundation.Validation;

public readonly record struct ValidationIssue(
    ValidationSeverity Severity,
    string Code,
    string Message,
    string? Path = null);
