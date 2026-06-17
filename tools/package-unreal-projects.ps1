param(
  [string[]]$ProjectKey = @(),
  [string]$OutputRoot = "K:\GameDev\Portfolio\artifacts\unreal-project-downloads",
  [string]$OneDriveCopyRoot = ""
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
Add-Type -AssemblyName System.IO.Compression.FileSystem

$projects = @(
  @{
    Key = "hospital-flow"
    Title = "Emergency Room simulator"
    SourcePath = "K:\GameDev\Projets\EmergencyRoom"
    StageFolderName = "EmergencyRoom"
    ArchiveName = "EmergencyRoom-UnrealProject.zip"
    Uproject = "Moba.uproject"
    EngineVersion = "Unreal Engine 5.7"
    StartingPoint = "Editor startup map: /Game/EmergencyRoom/Maps/HubMap.HubMap. Game default map: /Game/EmergencyRoom/Maps/FrontEndTestMap.FrontEndTestMap."
    Attribution = "Original/self-directed Unreal project. Imported Marketplace or sample assets remain credited to their original owners where applicable."
    PortfolioSlug = "hospital-flow"
  },
  @{
    Key = "moba-gas"
    Title = "Multiplayer MOBA GAS Prototype"
    SourcePath = "D:\GameDev\Anciens Projets\Apprentissage\MOBA GAS\LOD"
    StageFolderName = "Moba"
    ArchiveName = "Moba-UnrealProject.zip"
    Uproject = "Moba.uproject"
    EngineVersion = "Unreal Engine 5.7"
    StartingPoint = "Game and editor startup map: /Game/Maps/GameLevel.GameLevel."
    Attribution = "Course-based project from Jingtian Li's Udemy course, Multiplayer in Unreal with GAS and AWS Dedicated Servers. The archive is shared as implementation evidence and does not claim original art, design, or course ownership."
    PortfolioSlug = "moba-gas"
  },
  @{
    Key = "slash-cpp"
    Title = "Slash C++ Learning Project"
    SourcePath = "D:\GameDev\Anciens Projets\Apprentissage\Tutoriels Stephen Ulibarri\Learn UE 5 C++ Programming\Slash"
    StageFolderName = "SlashCpp"
    ArchiveName = "SlashCpp-UnrealProject.zip"
    Uproject = "Slash.uproject"
    EngineVersion = "Unreal Engine 5.6"
    StartingPoint = "Game and editor startup map: /Game/Maps/Minimal.Minimal."
    Attribution = "Course-based project from Stephen Ulibarri's Udemy course, Unreal Engine 5 C++ The Ultimate Game Developer Course. The archive is shared as implementation evidence and does not claim original art, design, or course ownership."
    PortfolioSlug = "slash-cpp"
  }
)

if ($ProjectKey.Count -gt 0) {
  $requestedKeys = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
  foreach ($key in $ProjectKey) {
    [void]$requestedKeys.Add($key)
  }

  $projects = @($projects | Where-Object { $requestedKeys.Contains($_.Key) })

  if ($projects.Count -ne $requestedKeys.Count) {
    $knownKeys = ($projects | ForEach-Object { $_.Key }) -join ", "
    throw "Unknown project key in request. Known selected keys: $knownKeys"
  }
}

$includedTopLevelItems = @("Config", "Content", "Source", "Plugins", "Build")
$excludedDirectoryNames = @(
  ".git",
  ".idea",
  ".vs",
  ".vscode",
  "Binaries",
  "DerivedDataCache",
  "Intermediate",
  "Packaged",
  "Saved",
  "Zen"
)
$excludedFilePatterns = @(
  "*.log",
  "*.sln",
  "*.suo",
  "*.user",
  "*.DotSettings.user",
  "*.opensdf",
  "*.VC.db",
  ".DS_Store",
  "Thumbs.db"
)

function Resolve-FullPath([string]$Path) {
  $fullPath = [System.IO.Path]::GetFullPath($Path)
  return $fullPath.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
}

function Assert-ChildPath([string]$RootPath, [string]$TargetPath, [string]$Label) {
  $root = Resolve-FullPath $RootPath
  $target = Resolve-FullPath $TargetPath
  $comparison = [System.StringComparison]::OrdinalIgnoreCase

  if ($target -eq $root -or $target.StartsWith("$root$([System.IO.Path]::DirectorySeparatorChar)", $comparison)) {
    return
  }

  throw "Unsafe $Label outside expected root. Root: $root Target: $target"
}

function Test-ExcludedDirectory([string]$Name) {
  foreach ($excludedName in $excludedDirectoryNames) {
    if ($Name -ieq $excludedName) {
      return $true
    }
  }

  return $false
}

function Test-ExcludedFile([string]$Name) {
  foreach ($pattern in $excludedFilePatterns) {
    if ($Name -like $pattern) {
      return $true
    }
  }

  return $false
}

function Copy-FilteredDirectory([string]$Source, [string]$Destination) {
  New-Item -ItemType Directory -Path $Destination -Force | Out-Null

  foreach ($entry in Get-ChildItem -LiteralPath $Source -Force) {
    $target = Join-Path $Destination $entry.Name

    if ($entry.PSIsContainer) {
      if (Test-ExcludedDirectory $entry.Name) {
        continue
      }

      Copy-FilteredDirectory -Source $entry.FullName -Destination $target
      continue
    }

    if (Test-ExcludedFile $entry.Name) {
      continue
    }

    Copy-Item -LiteralPath $entry.FullName -Destination $target -Force
  }
}

function Write-RecruiterReadme($Project, [string]$DestinationPath) {
  $readme = @"
# $($Project.Title)

This ZIP is a cleaned Unreal project archive prepared for portfolio review.

## Unreal Engine

$($Project.EngineVersion)

## Open This Project

Open `$($Project.Uproject)` from the root of this extracted folder.

If Visual Studio project files are missing, right-click `$($Project.Uproject)` and choose `Generate Visual Studio project files`.

Unreal may ask to rebuild C++ modules on first open. Accept the rebuild if prompted.

## Starting Point

$($Project.StartingPoint)

## Scope And Attribution

$($Project.Attribution)

## Portfolio Source Browser

The source can also be browsed directly in the portfolio:

`#/projects/$($Project.PortfolioSlug)/source`

## Archive Contents

This archive includes `Config/`, `Content/`, `Source/`, `Plugins/` and `Build/` when present, plus the `.uproject` file and this README.

Generated local Unreal folders are intentionally excluded: `.git/`, `.vs/`, `.idea/`, `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `Zen/`, `Packaged/`, logs, local user files, and generated IDE files.
"@

  Set-Content -LiteralPath $DestinationPath -Value $readme -Encoding UTF8
}

$resolvedOutputRoot = Resolve-FullPath $OutputRoot
$stagingRoot = Join-Path $resolvedOutputRoot "staging"
$archivesRoot = Join-Path $resolvedOutputRoot "archives"

New-Item -ItemType Directory -Path $resolvedOutputRoot -Force | Out-Null
Assert-ChildPath -RootPath $resolvedOutputRoot -TargetPath $stagingRoot -Label "staging root"
Assert-ChildPath -RootPath $resolvedOutputRoot -TargetPath $archivesRoot -Label "archives root"

if (Test-Path -LiteralPath $stagingRoot) {
  Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null
New-Item -ItemType Directory -Path $archivesRoot -Force | Out-Null

foreach ($project in $projects) {
  $projectSource = Resolve-FullPath $project.SourcePath
  if (-not (Test-Path -LiteralPath $projectSource -PathType Container)) {
    throw "Project source path not found: $projectSource"
  }

  $stageProjectRoot = Join-Path $stagingRoot $project.StageFolderName
  Assert-ChildPath -RootPath $stagingRoot -TargetPath $stageProjectRoot -Label "project stage path"
  New-Item -ItemType Directory -Path $stageProjectRoot -Force | Out-Null

  foreach ($itemName in $includedTopLevelItems) {
    $sourceItem = Join-Path $projectSource $itemName
    if (Test-Path -LiteralPath $sourceItem -PathType Container) {
      Copy-FilteredDirectory -Source $sourceItem -Destination (Join-Path $stageProjectRoot $itemName)
    }
  }

  $uprojectPath = Join-Path $projectSource $project.Uproject
  if (-not (Test-Path -LiteralPath $uprojectPath -PathType Leaf)) {
    throw "Project file not found: $uprojectPath"
  }

  Copy-Item -LiteralPath $uprojectPath -Destination (Join-Path $stageProjectRoot $project.Uproject) -Force
  Write-RecruiterReadme -Project $project -DestinationPath (Join-Path $stageProjectRoot "README.md")

  $archivePath = Join-Path $archivesRoot $project.ArchiveName
  if (Test-Path -LiteralPath $archivePath) {
    Remove-Item -LiteralPath $archivePath -Force
  }

  [System.IO.Compression.ZipFile]::CreateFromDirectory(
    $stageProjectRoot,
    $archivePath,
    [System.IO.Compression.CompressionLevel]::Optimal,
    $true
  )
  $archiveSize = (Get-Item -LiteralPath $archivePath).Length
  $archiveSizeMb = [Math]::Round($archiveSize / 1MB, 1)
  Write-Host "$($project.ArchiveName): $archiveSizeMb MB"

  if ($OneDriveCopyRoot) {
    $resolvedOneDriveCopyRoot = Resolve-FullPath $OneDriveCopyRoot
    New-Item -ItemType Directory -Path $resolvedOneDriveCopyRoot -Force | Out-Null
    Copy-Item -LiteralPath $archivePath -Destination (Join-Path $resolvedOneDriveCopyRoot $project.ArchiveName) -Force
  }
}

Write-Host "Archives written to $archivesRoot"
