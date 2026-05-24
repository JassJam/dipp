#!/usr/bin/env pwsh

<#
.SYNOPSIS
    Clean script for project to remove build artifacts.

.PARAMETER All
    Remove all build artifacts including install directory and testing artifacts.
    This performs the most comprehensive cleanup, removing:
    - Build directory (out/)
    - Install directory (install/)
    - Testing directory (Testing/)
    - Cache directory (.cache/)
    Default: false (only removes build directory)

.PARAMETER BuildDir
    Specific build directory to clean. By default targets the 'out' directory
    which is used by cmake-initializer presets.
    Default: "out"
    
    You can specify a different directory if using custom build locations.

.PARAMETER Cache
    Only clean CMake cache files without removing build artifacts. This is useful
    when you want to reconfigure the project without losing compiled objects.
    Removes:
    - CMakeCache.txt
    - CMakeFiles/ directory
    - cmake_install.cmake
    - CTestTestfile.cmake
    - cpm-package-lock.cmake
    Default: false

.PARAMETER Force
    Force removal without confirmation prompts. Use with caution as this will
    immediately delete files without asking for confirmation.
    Default: false (shows confirmation prompt)

.PARAMETER Verbose
    Enable verbose output showing exactly what files and directories are being
    removed, including their sizes. Useful for understanding what's taking up
    space in your build directory.
    Default: false

.EXAMPLE
    .\scripts\clean.ps1
    
    Clean the default build directory with confirmation prompt. This is the
    most common usage for routine cleanup of build artifacts.

.EXAMPLE
    .\scripts\clean.ps1 -All -Force
    
    Remove all build artifacts including install directory without confirmation.
    Use this for complete project cleanup before major rebuilds or releases.

.EXAMPLE
    .\scripts\clean.ps1 -Cache -Verbose
    
    Only clean CMake cache files with verbose output showing what's being removed.
    Useful when CMake configuration is corrupted but you want to keep build objects.

.EXAMPLE
    .\scripts\clean.ps1 -BuildDir "custom-build" -Verbose
    
    Clean a custom build directory with detailed output. Use when working with
    non-standard build directory layouts.

.NOTES
    The script calculates and reports disk space freed after cleaning operations.
    All operations can be safely interrupted with Ctrl+C if needed.
    
    Safety features include confirmation prompts and detailed reporting of what
    will be removed before actual deletion occurs.

.LINK
    https://github.com/JustJam/cmake-initializer
#>
param(
    [switch]$All,
    [string]$BuildDir = "out",
    [switch]$Cache,
    [switch]$Force,
    [switch]$VerboseOutput
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
} elseif (-not (Test-Path (Join-Path $ProjectRoot "CMakePresets.json"))) {
    # No CMakePresets.json found, but we can still clean build directories
    Write-Host "No CMakePresets.json found, using root directory" -ForegroundColor DarkGray
}

# Platform detection for display
$Platform = if ($PSVersionTable.PSVersion.Major -ge 6) {
    if ($IsWindows) { "Windows" }
    elseif ($IsLinux) { "Linux" }
    else { "macOS" }
} else {
    if ($env:OS -eq "Windows_NT") { "Windows" } else { "Unix" }
}

Write-Host "cmake-initializer Clean Script" -ForegroundColor Cyan
Write-Host "Platform: $Platform" -ForegroundColor Green

# Change to project directory
Push-Location $ProjectDir

try {
    # Define paths to clean - use workspace root with configurable BuildDir
    $PathsToClean = @()
    $BuildPath = Join-Path $ProjectRoot $BuildDir
    
    if ($Cache) {
        # Only clean cache files
        $CachePaths = @(
            (Join-Path $BuildPath "CMakeCache.txt"),
            (Join-Path $BuildPath "CMakeFiles"),
            (Join-Path $BuildPath "cmake_install.cmake"),
            (Join-Path $BuildPath "CTestTestfile.cmake"),
            (Join-Path $BuildPath "cpm-package-lock.cmake")
        )
        
        foreach ($CachePath in $CachePaths) {
            if (Test-Path $CachePath) {
                $PathsToClean += $CachePath
            }
        }
        
        Write-Host "Cleaning CMake cache files..." -ForegroundColor Yellow
    } else {
        # Clean build directory
        if (Test-Path $BuildPath) {
            $PathsToClean += $BuildPath
        }
        
        if ($All) {
            # Also clean install directory and other artifacts - use workspace root with configurable BuildDir
            $AdditionalPaths = @(
                (Join-Path $ProjectRoot "install"),
                (Join-Path $ProjectRoot "Testing"),
                (Join-Path $ProjectRoot ".cache")
            )
            
            foreach ($AdditionalPath in $AdditionalPaths) {
                if (Test-Path $AdditionalPath) {
                    $PathsToClean += $AdditionalPath
                }
            }
            
            Write-Host "Cleaning all build artifacts..." -ForegroundColor Yellow
        } else {
            Write-Host "Cleaning build directory..." -ForegroundColor Yellow
        }
    }
    
    # Check if there's anything to clean
    if ($PathsToClean.Count -eq 0) {
        Write-Host "Nothing to clean - project is already clean!" -ForegroundColor Green
        return
    }
    
    # Show what will be cleaned
    if ($VerboseOutput -or -not $Force) {
        Write-Host "The following items will be removed:" -ForegroundColor Yellow
        foreach ($Path in $PathsToClean) {
            $RelativePath = Resolve-Path -Relative $Path
            if (Test-Path $Path -PathType Container) {
                Write-Host "  | $RelativePath/" -ForegroundColor DarkYellow
            } else {
                Write-Host "  | $RelativePath" -ForegroundColor DarkYellow
            }
        }
        Write-Host ""
    }
    
    # Confirm removal if not forced
    if (-not $Force) {
        $Confirmation = Read-Host "Do you want to proceed? (y/N)"
        if ($Confirmation -notmatch "^[Yy]") {
            Write-Host "Operation cancelled by user" -ForegroundColor Yellow
            return
        }
    }
    
    # Remove the paths
    $RemovedCount = 0
    $TotalSize = 0
    
    foreach ($Path in $PathsToClean) {
        if (Test-Path $Path) {
            try {
                # Calculate size before removal
                if (Test-Path $Path -PathType Container) {
                    $Size = (Get-ChildItem -Recurse $Path | Measure-Object -Property Length -Sum).Sum
                } else {
                    $Size = (Get-Item $Path).Length
                }
                $TotalSize += $Size
                
                if ($VerboseOutput) {
                    $RelativePath = Resolve-Path -Relative $Path
                    $SizeMB = [math]::Round($Size / 1MB, 2)
                    Write-Host "  Removing $RelativePath (${SizeMB} MB)..." -ForegroundColor DarkGray
                }
                
                Remove-Item -Recurse -Force $Path
                $RemovedCount++
            } catch {
                Write-Warning "Failed to remove ${Path}: $($_.Exception.Message)"
            }
        }
    }
    
    # Show summary
    $TotalSizeMB = [math]::Round($TotalSize / 1MB, 2)
    Write-Host "Cleaned $RemovedCount item$(if ($RemovedCount -ne 1) { 's' })" -ForegroundColor Green
    if ($TotalSizeMB -gt 0) {
        Write-Host "Freed ${TotalSizeMB} MB of disk space" -ForegroundColor Cyan
    }

} catch {
    Write-Host "Clean failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    Pop-Location
}
