[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateRange(0, 5)]
    [int]$Phase,

    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
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

function Get-RelativePortablePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$FullPath
    )
    return $FullPath.Substring($Root.Length).TrimStart([char[]]@('\', '/')).Replace('\', '/')
}

$resolvedProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
$uprojectFiles = @(Get-ChildItem -LiteralPath $resolvedProjectRoot -Filter "*.uproject" -File)
if ($uprojectFiles.Count -eq 0) {
    throw "ProjectRoot must point to a folder containing a .uproject file: $resolvedProjectRoot"
}

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path
$manifestPath = Join-Path $PSScriptRoot "phase-manifest.json"
$sourceManifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$phaseData = @($sourceManifest.phases | Where-Object { [int]$_.phase -eq $Phase }) | Select-Object -First 1
if ($null -eq $phaseData) {
    throw "Phase $Phase is not defined in $manifestPath"
}

$pluginRoot = Join-Path $resolvedProjectRoot "Plugins"
if (-not (Test-Path -LiteralPath $pluginRoot -PathType Container)) {
    New-Item -ItemType Directory -Path $pluginRoot | Out-Null
}

$pluginName = [string]$sourceManifest.pluginName
$moduleName = [string]$sourceManifest.runtimeModuleName
$pluginDestination = Join-Path $pluginRoot $pluginName
$descriptorPath = Join-Path $pluginDestination ($pluginName + ".uplugin")
$integrationManifestPath = Join-Path $pluginDestination "WorldMechanicsIntegration.json"
$existingIntegration = $null

if ($Phase -eq 0) {
    if (Test-Path -LiteralPath $pluginDestination) {
        throw "The plugin folder already exists: $pluginDestination. Back it up and remove it manually only if you intend to start a new integration."
    }
}
else {
    if (-not (Test-Path -LiteralPath $descriptorPath -PathType Leaf) -or
        -not (Test-Path -LiteralPath $integrationManifestPath -PathType Leaf)) {
        throw "WorldMechanics phase 0 is missing. Export and build phase 0 first."
    }

    $existingIntegration = Get-Content -LiteralPath $integrationManifestPath -Raw | ConvertFrom-Json
    $installedPhases = @($existingIntegration.integratedPhases)
    if ($installedPhases.Count -ne $Phase) {
        throw "Expected phases 0 through $($Phase - 1) before adding phase $Phase. Current manifest contains $($installedPhases.Count) phase(s)."
    }
    for ($i = 0; $i -lt $Phase; $i++) {
        if ([int]$installedPhases[$i] -ne $i) {
            throw "Phases must be integrated in strict order. Phase $i is missing or out of order."
        }
    }
}

# Stage only this phase. Existing plugin code and user-authored adapters are never deleted.
$tempRoot = Join-Path $pluginRoot (".wm-stage-" + [Guid]::NewGuid().ToString("N"))
$stagedPlugin = Join-Path $tempRoot $pluginName
$stagedModuleRoot = Join-Path (Join-Path $stagedPlugin "Source") $moduleName
$publicRoot = Join-Path $stagedModuleRoot "Public"
$privateRoot = Join-Path $stagedModuleRoot "Private"
New-Item -ItemType Directory -Path $publicRoot -Force | Out-Null
New-Item -ItemType Directory -Path $privateRoot -Force | Out-Null

$portableFilePaths = New-Object System.Collections.Generic.List[string]
$headerCount = 0
$sourceCount = 0

try {
    foreach ($portableModule in @($phaseData.portableModules)) {
        $portableRoot = Join-Path (Join-Path $repositoryRoot "Modules") ([string]$portableModule)
        if (-not (Test-Path -LiteralPath $portableRoot -PathType Container)) {
            throw "Portable module does not exist: $portableRoot"
        }

        $includeRoot = Join-Path $portableRoot "include"
        if (Test-Path -LiteralPath $includeRoot -PathType Container) {
            foreach ($header in @(Get-ChildItem -LiteralPath $includeRoot -Filter "*.hpp" -File -Recurse)) {
                $relativePath = Get-RelativePortablePath -Root $includeRoot -FullPath $header.FullName
                $targetPath = Join-Path $publicRoot ($relativePath.Replace('/', [IO.Path]::DirectorySeparatorChar))
                $targetDirectory = Split-Path -Parent $targetPath
                New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
                if (Test-Path -LiteralPath $targetPath) {
                    throw "Duplicate header in this phase: $relativePath"
                }
                Copy-Item -LiteralPath $header.FullName -Destination $targetPath
                $portableFilePaths.Add(("Source/{0}/Public/{1}" -f $moduleName, $relativePath))
                $headerCount++
            }
        }

        $sourceRoot = Join-Path $portableRoot "src"
        if (Test-Path -LiteralPath $sourceRoot -PathType Container) {
            foreach ($source in @(Get-ChildItem -LiteralPath $sourceRoot -Filter "*.cpp" -File -Recurse)) {
                $relativePath = Get-RelativePortablePath -Root $sourceRoot -FullPath $source.FullName
                $targetPath = Join-Path $privateRoot ($relativePath.Replace('/', [IO.Path]::DirectorySeparatorChar))
                $targetDirectory = Split-Path -Parent $targetPath
                New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
                if (Test-Path -LiteralPath $targetPath) {
                    throw "Duplicate source filename in this phase: $relativePath"
                }
                Copy-Item -LiteralPath $source.FullName -Destination $targetPath
                $portableFilePaths.Add(("Source/{0}/Private/{1}" -f $moduleName, $relativePath))
                $sourceCount++
            }
        }
    }

    if ($headerCount -eq 0) {
        throw "No portable headers were copied for phase $Phase."
    }

    if ($Phase -eq 0) {
        $descriptor = [ordered]@{
            FileVersion = 3
            Version = 1
            VersionName = "0.1.0"
            FriendlyName = "World Mechanics"
            Description = "Incrementally integrated, engine-neutral world mechanics algorithms."
            Category = "Gameplay"
            CanContainContent = $false
            IsBetaVersion = $true
            Modules = @(
                [ordered]@{
                    Name = $moduleName
                    Type = "Runtime"
                    LoadingPhase = "Default"
                }
            )
        }
        Write-Utf8NoBom -Path (Join-Path $stagedPlugin ($pluginName + ".uplugin")) -Content ($descriptor | ConvertTo-Json -Depth 10)

        $buildRules = @"
using UnrealBuildTool;

public class $moduleName : ModuleRules
{
    public $moduleName(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new string[] { "Core" });
    }
}
"@
        Write-Utf8NoBom -Path (Join-Path $stagedModuleRoot ($moduleName + ".Build.cs")) -Content $buildRules

        $moduleEntry = @"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, $moduleName)
"@
        Write-Utf8NoBom -Path (Join-Path $privateRoot ($moduleName + "Module.cpp")) -Content $moduleEntry

        $packageReadme = @"
# World Mechanics Unreal Plugin

This plugin is assembled phase-by-phase by Tools/UnrealIntegration/Export-Phase.ps1.

The plugin compiles portable, engine-neutral C++ algorithms in one runtime module. Engine-specific Actor, UObject, subsystem, navigation, rendering, collision-publication, inventory-component and Editor adapters are not supplied by this package yet.

See Docs/UNREAL_INCREMENTAL_INTEGRATION.md in the source repository before wiring these algorithms to the live game world.
"@
        Write-Utf8NoBom -Path (Join-Path $stagedPlugin "README.md") -Content $packageReadme

        $integrationRecord = [ordered]@{
            schemaVersion = 1
            targetEngine = "Unreal Engine 5.8"
            pluginName = $pluginName
            moduleName = $moduleName
            integratedPhases = @($Phase)
            exportedFiles = @($portableFilePaths.ToArray())
        }
        Write-Utf8NoBom -Path (Join-Path $stagedPlugin "WorldMechanicsIntegration.json") -Content ($integrationRecord | ConvertTo-Json -Depth 10)
    }
    else {
        # Detect name collisions before touching the project plugin.
        foreach ($stagedFile in @(Get-ChildItem -LiteralPath $stagedPlugin -File -Recurse)) {
            $relativeToPlugin = Get-RelativePortablePath -Root $stagedPlugin -FullPath $stagedFile.FullName
            $destination = Join-Path $pluginDestination ($relativeToPlugin.Replace('/', [IO.Path]::DirectorySeparatorChar))
            if (Test-Path -LiteralPath $destination) {
                $existingHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
                $stagedHash = (Get-FileHash -LiteralPath $stagedFile.FullName -Algorithm SHA256).Hash
                if ($existingHash -ne $stagedHash) {
                    throw "Existing plugin file differs from the portable source: $destination. Nothing was overwritten; review and merge the file manually."
                }
            }
        }

        foreach ($stagedFile in @(Get-ChildItem -LiteralPath $stagedPlugin -File -Recurse)) {
            $relativeToPlugin = Get-RelativePortablePath -Root $stagedPlugin -FullPath $stagedFile.FullName
            $destination = Join-Path $pluginDestination ($relativeToPlugin.Replace('/', [IO.Path]::DirectorySeparatorChar))
            if (-not (Test-Path -LiteralPath $destination -PathType Leaf)) {
                $destinationDirectory = Split-Path -Parent $destination
                New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
                Copy-Item -LiteralPath $stagedFile.FullName -Destination $destination
            }
        }

        $allPhases = @($installedPhases) + @($Phase)
        $allFiles = @($existingIntegration.exportedFiles) + @($portableFilePaths.ToArray())
        $integrationRecord = [ordered]@{
            schemaVersion = 1
            targetEngine = "Unreal Engine 5.8"
            pluginName = $pluginName
            moduleName = $moduleName
            integratedPhases = @($allPhases)
            exportedFiles = @($allFiles | Select-Object -Unique)
        }
        # Record the phase only after all portable files are copied successfully.
        Write-Utf8NoBom -Path $integrationManifestPath -Content ($integrationRecord | ConvertTo-Json -Depth 10)
    }

    if ($Phase -eq 0) {
        Move-Item -LiteralPath $stagedPlugin -Destination $pluginDestination
    }

    Write-Output "Integrated World Mechanics phase $Phase ($($phaseData.name))."
    Write-Output "Plugin: $pluginDestination"
    Write-Output "Runtime module: $moduleName"
    Write-Output "Portable headers added: $headerCount"
    Write-Output "Portable .cpp files added: $sourceCount"
    Write-Output "Next: regenerate project files and build in Unreal Engine 5.8. Only then export the next phase."
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
