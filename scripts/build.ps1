#!/usr/bin/env pwsh

<#
.SYNOPSIS
    build script for project to compile using cmake presets.

.PARAMETER Preset
    CMake preset to use for building. This parameter is mandatory and must specify
    a valid preset name such as:
    - Windows: windows-msvc-debug/release, windows-clang-debug/release
    - Unix-like: unixlike-gcc-debug/release, unixlike-clang-debug/release
    - Emscripten: emscripten-debug/release

.PARAMETER Config
    Build configuration to use. Must be either 'Debug' or 'Release'.
    Default: Release (automatically derived from preset name if preset contains debug/release)
    
    Debug builds include debug symbols and disable optimizations.
    Release builds enable optimizations and may strip debug symbols.

.PARAMETER BuildDir
    Base directory for build outputs. The actual build directory will be
    {BuildDir}/build/{Preset} relative to the workspace root.
    Default: "out"
    
    Examples:
    - "out" creates builds in ./out/build/{preset}/
    - "build" creates builds in ./build/build/{preset}/
    - "../builds" creates builds in ../builds/build/{preset}/

.PARAMETER Targets
    Specific target names to build instead of building all targets. Can be specified
    multiple times to build multiple specific targets.
    Example: -Targets "HelloWorld", "MyLibrary"

.PARAMETER ExcludeTargets
    Target names to exclude from building. Useful when building most targets but
    wanting to skip specific ones (e.g., slow targets or ones with external dependencies).
    Example: -ExcludeTargets "SlowTarget", "OptionalLibrary"

.PARAMETER BuildDir
    Build directory name relative to project root. By default uses 'out' which
    matches the cmake-initializer preset configuration.
    Default: "out"

.PARAMETER Jobs
    Number of parallel build jobs to use during compilation. Higher values can
    speed up builds on multi-core systems but may increase memory usage.
    Default: Number of CPU cores detected on the system

.PARAMETER VerboseOutput
    Enable verbose build output. Shows detailed compilation commands and progress
    information. Useful for debugging build issues.
    Default: false

.PARAMETER ExtraArgs
    Additional arguments to pass directly to CMake commands. Useful for passing
    custom variables or options that aren't covered by other parameters.
    Example: @("-DBUILD_TESTING=ON", "-DCMAKE_VERBOSE_MAKEFILE=ON")

.EXAMPLE
    .\scripts\build.ps1 -Preset unixlike-gcc-release
    
    Build with default settings using GCC Release preset.
    This is the most common usage for development builds.

.EXAMPLE
    .\scripts\build.ps1 -Preset windows-msvc-debug -VerboseOutput
    
    Build Debug configuration with a verbose output.
    Useful for creating portable debug builds with detailed build information.

.EXAMPLE
    .\scripts\build.ps1 -Preset unixlike-clang-release -VerboseOutput
    
    Build with Clang compiler and verbose output.
    Good for testing with different compilers or debugging build issues.

.EXAMPLE
    .\scripts\build.ps1 -Preset unixlike-gcc-release -Jobs 16
    
    Build optimized release version with 16 parallel jobs.
    Ideal for creating fast, portable release builds on high-core-count systems.

.NOTES
    Requires CMake 3.21+ and appropriate compilers to be installed and available in PATH.
    The script automatically detects project structure and adapts to cmake-initializer
    project layout with project subdirectory.

.LINK
    https://github.com/JustJam/cmake-initializer
#>
param(
    [Parameter(Mandatory=$true)]
    [string]$Preset,
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [string[]]$Targets = @(),
    [string[]]$ExcludeTargets = @(),
    [string]$BuildDir = "out",
    [int]$Jobs = 0,
    [switch]$VerboseOutput,
    [string[]]$ExtraArgs = @()
)

# Set error action preference
$ErrorActionPreference = "Stop"

# Get script directory and project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Detect if we have a project subdirectory structure
$ProjectDir = $ProjectRoot
if (Test-Path (Join-Path $ProjectRoot "project\CMakePresets.json")) {
    $ProjectDir = Join-Path $ProjectRoot "project"
    Write-Host "Using project subdirectory: $ProjectDir" -ForegroundColor DarkGray
} elseif (-not (Test-Path (Join-Path $ProjectRoot "CMakePresets.json"))) {
    throw "Could not find CMakePresets.json in $ProjectRoot or $ProjectRoot\project"
}

# Determine number of jobs if not specified
if ($Jobs -eq 0) {
    $Jobs = [Environment]::ProcessorCount
}

# Platform detection
$Platform = if ($PSVersionTable.PSVersion.Major -ge 6) {
    if ($IsWindows) { "Windows" }
    elseif ($IsLinux) { "Linux" }
    else { "macOS" }
} else {
    if ($env:OS -eq "Windows_NT") { "Windows" } else { "Unix" }
}

Write-Host "cmake-initializer Build Script" -ForegroundColor Cyan
Write-Host "Platform: $Platform" -ForegroundColor Green

# Derive build configuration from preset name
if ($Preset -match "debug") {
    $Config = "Debug"
} elseif ($Preset -match "release") {
    $Config = "Release"
}

Write-Host "Configuration: $Config" -ForegroundColor Green
Write-Host "Preset: $Preset" -ForegroundColor Green

# Build CMake configuration arguments
$ConfigArgs = @()

# Add extra arguments if provided
if ($ExtraArgs -and $ExtraArgs.Count -gt 0) {
    $ConfigArgs += $ExtraArgs
    Write-Host "Extra CMake args: $($ExtraArgs -join ' ')" -ForegroundColor Yellow
}

Write-Host "Configuration: $Config" -ForegroundColor Green
Write-Host "Preset: $Preset" -ForegroundColor Green

# Change to project directory
Push-Location $ProjectDir

try {
    # Validate preset exists before configuring
    Write-Host "Configuring project..." -ForegroundColor Blue
    
    # Check if preset is available
    $AvailablePresets = & cmake --list-presets 2>$null | Where-Object { $_ -match '^\s*"([^"]+)"' } | ForEach-Object { $Matches[1] }
    if ($AvailablePresets -and $Preset -notin $AvailablePresets) {
        Write-Host "Preset '$Preset' is not available on this platform." -ForegroundColor Red
        Write-Host "Available presets:" -ForegroundColor Yellow
        foreach ($p in $AvailablePresets) {
            Write-Host "  - $p" -ForegroundColor Yellow
        }
        throw "Invalid preset: $Preset"
    }
    
    # Configure the project - use workspace root for build output
    $BuildOutputDir = Join-Path $ProjectRoot "$BuildDir/build/$Preset"
    
    $ConfigCmd = @("cmake", "-S", ".", "-B", $BuildOutputDir, "--preset", $Preset)
    $ConfigCmd += $ConfigArgs
    
    if ($VerboseOutput) {
        Write-Host "Command: $($ConfigCmd -join ' ')" -ForegroundColor DarkGray
    }
    
    # Run the initial configuration
    & $ConfigCmd[0] $ConfigCmd[1..($ConfigCmd.Length-1)]
    $ConfigResult = $LASTEXITCODE
    
    # If configuration failed and this is an Emscripten build, try with --fresh
    if ($ConfigResult -ne 0 -and $Preset -match "emscripten") {
        Write-Host "Initial configuration failed. Retrying with fresh configuration for Emscripten..." -ForegroundColor Yellow
        
        # Add --fresh and try again
        $FreshConfigCmd = $ConfigCmd + @("--fresh")
        if ($VerboseOutput) {
            Write-Host "Retry command: $($FreshConfigCmd -join ' ')" -ForegroundColor DarkGray
        }
        
        & $FreshConfigCmd[0] $FreshConfigCmd[1..($FreshConfigCmd.Length-1)]
        $ConfigResult = $LASTEXITCODE
    }
    
    if ($ConfigResult -ne 0) {
        throw "Configuration failed with exit code $ConfigResult"
    }

    # Build the project
    Write-Host "Building project..." -ForegroundColor Blue
    $BuildCmd = @("cmake", "--build", $BuildOutputDir, "--config", $Config, "--parallel", $Jobs)
    
    if ($Targets.Count -gt 0) {
        # Filter out excluded targets - ensure we maintain array structure
        $TargetsToBuild = @($Targets | Where-Object { $_ -notin $ExcludeTargets })
        
        if ($TargetsToBuild.Count -eq 0) {
            throw "No targets to build after applying exclusions"
        }
        
        if ($TargetsToBuild.Count -eq 1) {
            $BuildCmd += "--target", $TargetsToBuild[0]
            Write-Host "Target: $($TargetsToBuild[0])" -ForegroundColor Green
        } else {
            # For multiple targets, build them individually to handle failures gracefully
            Write-Host "Targets: $($TargetsToBuild -join ', ')" -ForegroundColor Green
            if ($ExcludeTargets.Count -gt 0) {
                Write-Host "Excluded: $($ExcludeTargets -join ', ')" -ForegroundColor Yellow
            }
            
            $SuccessfulTargets = @()
            $FailedTargets = @()
            
            foreach ($Target in $TargetsToBuild) {
                Write-Host "  Building $Target..." -ForegroundColor DarkCyan
                $TargetBuildCmd = @("cmake", "--build", $BuildOutputDir, "--config", $Config, "--parallel", $Jobs, "--target", $Target)
                if ($VerboseOutput) {
                    $TargetBuildCmd += "--verbose"
                }
                
                & $TargetBuildCmd[0] $TargetBuildCmd[1..($TargetBuildCmd.Length-1)]
                if ($LASTEXITCODE -eq 0) {
                    Write-Host "  | $Target" -ForegroundColor Green
                    $SuccessfulTargets += $Target
                } else {
                    Write-Host "  | $Target failed" -ForegroundColor Red
                    $FailedTargets += $Target
                }
            }
            
            Write-Host "Build summary:" -ForegroundColor Cyan
            Write-Host "  | Successful: $($SuccessfulTargets.Count)/$($TargetsToBuild.Count) ($($SuccessfulTargets -join ', '))" -ForegroundColor Green
            if ($FailedTargets.Count -gt 0) {
                Write-Host "  | Failed: $($FailedTargets.Count)/$($TargetsToBuild.Count) ($($FailedTargets -join ', '))" -ForegroundColor Red
                throw "Some targets failed to build"
            }
            
            # Skip the normal build execution since we built targets individually
            return
        }
    } elseif ($ExcludeTargets.Count -gt 0) {
        Write-Host "Excluded targets: $($ExcludeTargets -join ', ')" -ForegroundColor Yellow
        Write-Host "Building all targets except excluded ones..." -ForegroundColor Blue
    }
    
    if ($VerboseOutput) {
        $BuildCmd += "--verbose"
        Write-Host "Command: $($BuildCmd -join ' ')" -ForegroundColor DarkGray
    }
    
    & $BuildCmd[0] $BuildCmd[1..($BuildCmd.Length-1)]
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }

    Write-Host "Build completed successfully!" -ForegroundColor Green
    
    # Show build information
    $ActualBuildDir = Join-Path $ProjectRoot "$BuildDir/build/$Preset"
    if (Test-Path $ActualBuildDir) {
        $BuildSize = (Get-ChildItem -Recurse $ActualBuildDir | Measure-Object -Property Length -Sum).Sum
        $BuildSizeMB = [math]::Round($BuildSize / 1MB, 2)
        Write-Host "Build directory size: ${BuildSizeMB} MB" -ForegroundColor Cyan
    }

} catch {
    Write-Host "Build failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    Pop-Location
}
