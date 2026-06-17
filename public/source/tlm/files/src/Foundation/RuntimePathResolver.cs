using Godot;
using System.IO;

namespace TheLastMage.Foundation;

public static class RuntimePathResolver
{
    private const string UserDataArgumentPrefix = "--tlm-user-data=";
    private const string RegressionUserDataArgumentPrefix = "--tlm-regression-user-data=";
    private const string UserPathPrefix = "user://";

    public static string GlobalizeUserPath(string godotUserPath)
    {
        var regressionRoot = GetRegressionUserDataRoot();
        if (string.IsNullOrWhiteSpace(regressionRoot))
        {
            return ProjectSettings.GlobalizePath(godotUserPath);
        }

        var relativePath = godotUserPath.StartsWith(UserPathPrefix, StringComparison.Ordinal)
            ? godotUserPath[UserPathPrefix.Length..]
            : godotUserPath;
        return Path.Combine(regressionRoot, relativePath.Replace('/', Path.DirectorySeparatorChar));
    }

    public static string GetRegressionUserDataRoot()
    {
        foreach (var argument in OS.GetCmdlineUserArgs())
        {
            if (argument.StartsWith(UserDataArgumentPrefix, StringComparison.OrdinalIgnoreCase))
            {
                return ResolveUserDataArgument(argument[UserDataArgumentPrefix.Length..]);
            }

            if (!argument.StartsWith(RegressionUserDataArgumentPrefix, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            return ResolveUserDataArgument(argument[RegressionUserDataArgumentPrefix.Length..]);
        }

        return string.Empty;
    }

    private static string ResolveUserDataArgument(string value)
    {
        var path = value.Trim();
        if (string.IsNullOrWhiteSpace(path))
        {
            return string.Empty;
        }

        return Path.IsPathRooted(path)
            ? path
            : Path.GetFullPath(Path.Combine(ProjectSettings.GlobalizePath("res://"), path));
    }
}
