[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateRange(0, 5)]
    [int]$Phase,

    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [switch]$ReplaceExisting
)

$ErrorActionPreference = "Stop"

function Write-Utf8NoBom {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Content
    )
    $encoding = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($Path, $Content, $encoding)
}

$resolvedProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
$uprojectFiles = @(Get-ChildItem -LiteralPath $resolvedProjectRoot -Filter "*.uproject" -File)
if ($uprojectFiles.Count -eq 0) {
    throw "ProjectRoot must point to a folder containing a .uproject file: $resolvedProjectRoot"
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path
$manifestPath = Join-Path $PSScriptRoot "phase-manifest.json"
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$phaseData = @($manifest.phases | Where-Object { [int]$_.phase -eq $Phase }) | Select-Object -First 1
if ($null -eq $phaseData) {
    throw "Phase $Phase is not defined in $manifestPath"
}

$pluginRoot = Join-Path $resolvedProjectRoot "Plugins"
if (-not (Test-Path -LiteralPath $pluginRoot -PathType Container)) {
    New-Item -ItemType Directory -Path $pluginRoot | Out-Null
}

# Require the dependency chain to exist before adding a higher phase.
foreach ($requiredPlugin in @($phaseData.requiresPlugins)) {
    $requiredDescriptor = Join-Path (Join-Path $pluginRoot ([string]$requiredPlugin)) ([string]$requiredPlugin + ".uplugin")
    if (-not (Test-Path -LiteralPath $requiredDescriptor -PathType Leaf)) {
        throw "Missing dependency plugin '$requiredPlugin'. Add phases in order (0, 1, 2, 3, 4, 5) before adding phase $Phase."
    }
}

$destinationPlugin = Join-Path $pluginRoot ([string]$phaseData.pluginName)
if (Test-Path -LiteralPath $destinationPlugin) {
    if (-not $ReplaceExisting) {
        throw "Plugin folder already exists: $destinationPlugin. Back it up first; use -ReplaceExisting only if you intend to replace the entire generated plugin folder."
    }
}

# Stage beside the target so the final directory move stays on the same volume.
$tempRoot = Join-Path $pluginRoot (".wm-stage-" + [Guid]::NewGuid().ToString("N"))
$stagedPlugin = Join-Path $tempRoot ([string]$phaseData.pluginName)
New-Item -ItemType Directory -Path $stagedPlugin -Force | Out-Null
$headerCount = 0
$sourceCount = 0

try {
    $descriptorDependencies = @(
        @($phaseData.requiresPlugins) | ForEach-Object {
            [ordered]@{ Name = [string]$_; Enabled = $true }
        }
    )
    $descriptor = [ordered]@{
        FileVersion = 3
        Version = 1
        VersionName = "0.1.0"
        FriendlyName = [string]$phaseData.friendlyName
        Description = [string]$phaseData.description
        Category = "Gameplay"
        CanContainContent = $false
        IsBetaVersion = $true
        Modules = @(
            [ordered]@{
                Name = [string]$phaseData.moduleName
                Type = "Runtime"
                LoadingPhase = "Default"
            }
        )
        Plugins = $descriptorDependencies
    }

    Write-Utf8NoBom -Path (Join-Path $stagedPlugin ([string]$phaseData.pluginName + ".uplugin")) -Content ($descriptor | ConvertTo-Json -Depth 10)
    
    $sourceModuleName = [string]$phaseData.moduleName
    $moduleRoot = Join-Path (Join-Path $stagedPlugin "Source") $sourceModuleName
    $publicRoot = Join-Path $moduleRoot "Public"
    $privateRoot = Join-Path $moduleRoot "Private"
    New-Item -ItemType Directory -Path $publicRoot -Force | Out-Null
    New-Item -ItemType Directory -Path $privateRoot -Force | Out-Null

    $moduleDependencies = @("Core") + @($phaseData.moduleDependencies)
    $moduleDependencyText = ($moduleDependencies | ForEach-Object { '"' + [string]$_ + '"' }) -join ", "
    $buildRules = @"
using UnrealBuildTool;

public class $sourceModuleName : ModuleRules
{
    public $sourceModuleName(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new string[] { $moduleDependencyText });
    }
}
"@
    Write-Utf8NoBom -Path (Join-Path $moduleRoot ($sourceModuleName + ".Build.cs")) -Content $buildRules

    $moduleEntry = @"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, $sourceModuleName)
"@
    Write-Utf8NoBom -Path (Join-Path $privateRoot ($sourceModuleName + "Module.cpp")) -Content $moduleEntry

    foreach ($portableModule in @($phaseData.portableModules)) {
        $portableRoot = Join-Path (Join-Path $repositoryRoot "Modules") ([string]$portableModule)
        if (-not (Test-Path -LiteralPath $portableRoot -PathType Container)) {
            throw "Portable module does not exist: $portableRoot"
        }

        $includeRoot = Join-Path $portableRoot "include"
        if (Test-Path -LiteralPath $includeRoot -PathType Container) {
            foreach ($header in @(Get-ChildItem -LiteralPath $includeRoot -Filter "*.hpp" -File -Recurse)) {
                $relativePath = $header.FullName.Substring($includeRoot.Length).TrimStart([char[]]@('\', '/'))
                $targetPath = Join-Path $publicRoot $relativePath
                $targetDirectory = Split-Path -Parent $targetPath
                New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
                if (Test-Path -LiteralPath $targetPath) {
                    throw "Header name collision in this phase: $targetPath"
                }
                Copy-Item -LiteralPath $header.FullName -Destination $targetPath
                $headerCount++
            }
        }

        $sourceRoot = Join-Path $portableRoot "src"
        if (Test-Path -LiteralPath $sourceRoot -PathType Container) {
            foreach ($source in @(Get-ChildItem -LiteralPath $sourceRoot -Filter "*.cpp" -File -Recurse)) {
                $relativePath = $source.FullName.Substring($sourceRoot.Length).TrimStart([char[]]@('\', '/'))
                $targetPath = Join-Path $privateRoot $relativePath
                $targetDirectory = Split-Path -Parent $targetPath
                New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
                if (Test-Path -LiteralPath $targetPath) {
                    throw "Source filename collision in this phase: $targetPath"
                }
                Copy-Item -LiteralPath $source.FullName -Destination $targetPath
                $sourceCount++
            }
        }
    }

    if ($headerCount -eq 0) {
        throw "No portable headers were copied for phase $Phase."
    }

    $packageReadme = @"
# $($phaseData.friendlyName)

Generated by \`Tools/UnrealIntegration/Export-Phase.ps1\`.

- Portable headers copied: $headerCount
- Portable .cpp files copied: $sourceCount
- C++ standard: C++17
- Engine-specific Actor, UObject, subsystem, navigation, rendering and editor adapters: **not included yet**

This package exposes the existing engine-neutral \`wm\` algorithms as an Unreal runtime module. It is an integration starting point, not a claim that every mechanic is already wired to the live game world. Run Unreal Build Tool first, then implement and test the adapters described in \`Docs/UNREAL_INCREMENTAL_INTEGRATION.md\`.
"@
    Write-Utf8NoBom -Path (Join-Path $stagedPlugin "README.md") -Content $packageReadme

    if (Test-Path -LiteralPath $destinationPlugin) {
        if (-not $ReplaceExisting) {
            throw "Plugin folder already exists: $destinationPlugin"
        }
        Remove-Item -LiteralPath $destinationPlugin -Recurse -Force
    }

    Move-Item -LiteralPath $stagedPlugin -Destination $destinationPlugin
    Write-Output "Created phase $Phase plugin: $destinationPlugin"
    Write-Output "Plugin: $($phaseData.pluginName)"
    Write-Output "Runtime module: $sourceModuleName"
    Write-Output "Headers copied: $headerCount"
    Write-Output "Sources copied: $sourceCount"
    Write-Output "Next: regenerate project files / build in Unreal Engine 5.8 before adding the next phase."
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
