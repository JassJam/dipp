#!/usr/bin/env pwsh

<#
.SYNOPSIS
    Install script for project to install built artifacts.

.PARAMETER Config
    Build configuration to install. Must be either 'Debug' or 'Release'.
    The configuration must match what was previously built.
    Default: Release
    
    This determines which build artifacts to install - debug versions include
    debug symbols while release versions are optimized.

.PARAMETER Prefix
    Installation prefix directory where files will be installed. Can be an absolute
    or relative path. If not specified, defaults to './install' in the project directory.
    Default: ./install (relative to project directory)
    
    Examples:
    - Windows: "C:\Program Files\MyProject"
    - Linux: "/usr/local" or "~/myproject"
    - macOS: "/Applications/MyProject"

.PARAMETER Component
    Specific component to install instead of installing everything. This allows
    selective installation of only certain parts of the project.
    Default: (empty - install all components)
    
    Common components include 'Runtime', 'Development', 'Documentation'.
    Available components depend on how the project configures CMake install rules.

.PARAMETER BuildDir
    Build directory containing the artifacts to install. Must contain a valid
    CMake build with install rules configured.
    Default: "out"
    
    This should match the build directory used during compilation.

.PARAMETER Verbose
    Enable verbose installation output showing detailed file operations, sizes,
    and installation paths. Useful for debugging installation issues.
    Default: false

.PARAMETER DryRun
    Show what would be installed without actually installing anything. This is
    useful for previewing installation operations and verifying paths.
    Default: false
    
    When enabled, shows detailed information about files that would be installed
    and their destination paths without making any changes to the system.

.PARAMETER Force
    Force installation even if target files already exist. This will overwrite
    existing files without confirmation prompts.
    Default: false (shows confirmation for conflicts)

.PARAMETER ExtraArgs
    Additional arguments to pass directly to CMake install commands. Useful for passing
    custom variables or options that aren't covered by other parameters.
    Example: @("--verbose", "--parallel 4")

.EXAMPLE
    .\scripts\install.ps1
    
    Install with default settings (Release configuration to ./install directory).
    This is the most common usage for local development installations.

.EXAMPLE
    .\scripts\install.ps1 -Prefix "C:\Program Files\MyProject" -Verbose
    
    Install to a system location with detailed output. Useful for creating
    system-wide installations with full visibility into the process.

.EXAMPLE
    .\scripts\install.ps1 -DryRun -Verbose
    
    Show what would be installed with detailed information without actually
    installing. Perfect for previewing installation operations.

.EXAMPLE
    .\scripts\install.ps1 -Config Debug -Component Runtime -Force
    
    Install only the runtime component from a debug build, overwriting any
    existing files. Useful for selective updates of specific components.

.NOTES
    Requires a successful build to be completed first. The script verifies that
    build artifacts exist before attempting installation.
    
    Installation paths and permissions may require administrator/sudo privileges
    depending on the target directory chosen.
    
    The script provides detailed reporting of installation size and file counts
    after successful completion.

.LINK
    https://github.com/JustJam/cmake-initializer
#>
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [Parameter(Mandatory=$true)]
    [string]$Preset,
    [string[]]$Targets = @(),
    [string[]]$ExcludeTargets = @(),
    [string]$Prefix = "",
    [string]$Component = "",
    [string]$BuildDir = "out",
    [switch]$VerboseOutput,
    [switch]$DryRun,
    [switch]$Force,
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
} elseif (-not (Test-Path (Join-Path $ProjectRoot "CMakePresets.json"))) {
    throw "Could not find CMakePresets.json in $ProjectRoot or $ProjectRoot\project"
}

# Platform detection
$Platform = if ($PSVersionTable.PSVersion.Major -ge 6) {
    if ($IsWindows) { "Windows" }
    elseif ($IsLinux) { "Linux" }
    else { "macOS" }
} else {
    if ($env:OS -eq "Windows_NT") { "Windows" } else { "Unix" }
}

Write-Host "cmake-initializer Install Script" -ForegroundColor Cyan
Write-Host "Platform: $Platform" -ForegroundColor Green

# Derive build configuration from preset name if preset was provided
if ($Preset -match "debug") {
    $Config = "Debug"
} elseif ($Preset -match "release") {
    $Config = "Release"
}

Write-Host "Configuration: $Config" -ForegroundColor Green
Write-Host "Preset: $Preset" -ForegroundColor Green

# Change to project directory
Push-Location $ProjectDir

try {
    # Determine the actual build directory based on preset structure - use workspace root with configurable BuildDir
    $ActualBuildPath = Join-Path $ProjectRoot "$BuildDir/build/$Preset"
    
    # Configure project if build directory doesn't exist or isn't configured
    if (-not (Test-Path $ActualBuildPath) -or -not (Test-Path (Join-Path $ActualBuildPath "CMakeCache.txt"))) {
        Write-Host "Build directory not found or not configured. Configuring project first..." -ForegroundColor Blue
        
        # Determine configuration command
        if ($Preset) {
            $ConfigureCmd = @("cmake", "-S", $ProjectDir, "-B", $ActualBuildPath, "--preset", $Preset)
        } else {
            throw "No preset specified and build directory not found. Please specify a preset or run build script first."
        }
        
        if ($VerboseOutput) {
            Write-Host "Configure command: $($ConfigureCmd -join ' ')" -ForegroundColor DarkGray
        }
        
        & $ConfigureCmd[0] $ConfigureCmd[1..($ConfigureCmd.Length-1)]
        if ($LASTEXITCODE -ne 0) {
            throw "Project configuration failed with exit code $LASTEXITCODE"
        }
        
        Write-Host "Project configured successfully" -ForegroundColor Green
    }

    # Determine default prefix if not specified
    if (-not $Prefix) {
        # Use the same pattern as build directory: derive from preset structure
        # Build uses: $ProjectRoot/$BuildDir/build/$Preset
        # Install uses: $ProjectRoot/$BuildDir/install/$Preset (following preset's installDir pattern)
        $Prefix = Join-Path $ProjectRoot "$BuildDir/install/$Preset"
    }

    Write-Host "Installation prefix: $Prefix" -ForegroundColor Green

    # If targets are specified, build them first
    if ($Targets.Count -gt 0) {
        # Filter out excluded targets - ensure we maintain array structure
        $TargetsToBuild = @($Targets | Where-Object { $_ -notin $ExcludeTargets })
        
        if ($TargetsToBuild.Count -eq 0) {
            throw "No targets to build after applying exclusions"
        }
        
        Write-Host "Building targets before installation..." -ForegroundColor Blue
        Write-Host "Targets: $($TargetsToBuild -join ', ')" -ForegroundColor Green
        if ($ExcludeTargets.Count -gt 0) {
            Write-Host "Excluded: $($ExcludeTargets -join ', ')" -ForegroundColor Yellow
        }
        
        $SuccessfulTargets = @()
        foreach ($Target in $TargetsToBuild) {
            Write-Host "  Building $Target..." -ForegroundColor DarkCyan
            $BuildCmd = @("cmake", "--build", $ActualBuildPath, "--config", $Config, "--target", $Target)
            
            if ($VerboseOutput) {
                Write-Host "Build command: $($BuildCmd -join ' ')" -ForegroundColor DarkGray
            }
            
            & $BuildCmd[0] $BuildCmd[1..($BuildCmd.Length-1)]
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  | $Target" -ForegroundColor Green
                $SuccessfulTargets += $Target
            } else {
                Write-Host "  | $Target failed to build" -ForegroundColor Yellow
            }
        }
        
        if ($SuccessfulTargets.Count -eq 0) {
            throw "No targets were built successfully"
        }
        
        Write-Host "Built $($SuccessfulTargets.Count)/$($TargetsToBuild.Count) targets successfully" -ForegroundColor Green
    }

    # Build install command - use cmake --install with error handling for Emscripten compatibility
    $InstallArgs = @("--install", $ActualBuildPath, "--config", $Config)

    # Add extra arguments if provided
    if ($ExtraArgs -and $ExtraArgs.Count -gt 0) {
        $InstallArgs += $ExtraArgs
        Write-Host "Extra CMake install args: $($ExtraArgs -join ' ')" -ForegroundColor Yellow
    }

    if ($Prefix) {
        $InstallArgs += "--prefix"
        $InstallArgs += $Prefix
    }

    if ($Component) {
        $InstallArgs += "--component"
        $InstallArgs += $Component
        Write-Host "Component: $Component" -ForegroundColor Green
    }

    if ($VerboseOutput) {
        $InstallArgs += "--verbose"
    }

    # Show what will be installed
    if ($DryRun -or $VerboseOutput) {
        Write-Host "Installation command:" -ForegroundColor Yellow
        Write-Host "  cmake $($InstallArgs -join ' ')" -ForegroundColor DarkGray
        Write-Host ""
    }

    if ($DryRun) {
        Write-Host "Dry run mode - showing what would be installed:" -ForegroundColor Yellow
        
        # Try to get install manifest
        $ManifestPath = Join-Path $ActualBuildPath "install_manifest.txt"
        if (Test-Path $ManifestPath) {
            $Manifest = Get-Content $ManifestPath
            Write-Host "Files that would be installed:" -ForegroundColor Cyan
            foreach ($File in $Manifest) {
                Write-Host "  | $File" -ForegroundColor DarkCyan
            }
        } else {
            Write-Host "Install manifest not found. Run a build first to see detailed install list." -ForegroundColor Yellow
        }
        
        return
    }

    # Check if prefix directory exists and handle conflicts
    if ((Test-Path $Prefix) -and -not $Force) {
        $ExistingFiles = Get-ChildItem -Recurse $Prefix -ErrorAction SilentlyContinue
        if ($ExistingFiles) {
            Write-Host "Installation prefix '$Prefix' already contains files." -ForegroundColor Yellow
            $Confirmation = Read-Host "Continue with installation? This may overwrite existing files. (y/N)"
            if ($Confirmation -notmatch "^[Yy]") {
                Write-Host "Installation cancelled by user" -ForegroundColor Yellow
                return
            }
        }
    }

    # Create prefix directory if it doesn't exist
    if (-not (Test-Path $Prefix)) {
        Write-Host "Creating installation directory: $Prefix" -ForegroundColor Blue
        New-Item -ItemType Directory -Path $Prefix -Force | Out-Null
    }

    # Run the installation
    Write-Host "Installing project..." -ForegroundColor Blue
    $InstallCmd = @("cmake") + $InstallArgs
    
    if ($VerboseOutput) {
        Write-Host "Command: $($InstallCmd -join ' ')" -ForegroundColor DarkGray
    }
    
    # For Emscripten presets, try installing individual components to avoid global install failures
    if ($Preset -like "*emscripten*" -and -not $Target) {
        Write-Host "Using component-based installation for Emscripten compatibility..." -ForegroundColor Blue
        
        # Try to install specific sample directories that have built targets
        $InstallSuccess = $false
        
        # First install the base components (always work)
        Write-Host "  Installing base components..." -ForegroundColor DarkCyan
        $BaseInstallCmd = @("cmake", "--install", $ActualBuildPath, "--config", $Config, "--component", "Unspecified")
        if ($Prefix) {
            $BaseInstallCmd += "--prefix"
            $BaseInstallCmd += $Prefix
        }
        if ($VerboseOutput) {
            $BaseInstallCmd += "--verbose"
        }
        
        & $BaseInstallCmd[0] $BaseInstallCmd[1..($BaseInstallCmd.Length-1)] 2>$null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  | Base components installed" -ForegroundColor Green
            $InstallSuccess = $true
        }
        
        if (-not $InstallSuccess) {
            throw "No components could be installed successfully"
        }
        
        $InstallExitCode = 0  # Set success since we handled errors individually
    } else {
        # Standard installation for non-Emscripten or when specific targets are specified
        if ($Targets.Count -gt 0) {
            # When specific targets are specified, try to install only those targets
            Write-Host "Installing specific targets: $($TargetsToBuild -join ', ')" -ForegroundColor Blue
            
            $InstallSuccess = $false
            $InstallErrors = @()
            
            foreach ($Target in $TargetsToBuild) {
                Write-Host "  Installing $Target..." -ForegroundColor DarkCyan
                
                $TargetInstalled = $false
                
                # First try directory-based install (more reliable for specific targets)
                $TargetPatterns = @("*$Target*", "*$($Target.ToLower())*", "*$($Target.Replace('Hello', 'hello_'))*")
                $TargetDirs = @()
                
                foreach ($Pattern in $TargetPatterns) {
                    $FoundDirs = Get-ChildItem -Path $ActualBuildPath -Recurse -Directory | Where-Object { $_.Name -like $Pattern }
                    $TargetDirs += $FoundDirs
                }
                
                # Remove duplicates and filter for directories with cmake_install.cmake
                $TargetDirs = $TargetDirs | Sort-Object FullName -Unique | Where-Object { Test-Path (Join-Path $_.FullName "cmake_install.cmake") }
                
                foreach ($TargetDir in $TargetDirs) {
                    Write-Host "    Installing from target directory: $($TargetDir.Name)" -ForegroundColor DarkYellow
                    $DirInstallCmd = @("cmake", "--install", $TargetDir.FullName, "--config", $Config)
                    if ($Prefix) {
                        $DirInstallCmd += "--prefix", $Prefix
                    }
                    if ($VerboseOutput) {
                        $DirInstallCmd += "--verbose"
                    }
                    
                    & $DirInstallCmd[0] $DirInstallCmd[1..($DirInstallCmd.Length-1)]
                    if ($LASTEXITCODE -eq 0) {
                        Write-Host "  | $Target installed via directory" -ForegroundColor Green
                        $InstallSuccess = $true
                        $TargetInstalled = $true
                        break
                    }
                }
                
                # Fallback to component-based install if directory approach fails
                if (-not $TargetInstalled) {
                    Write-Host "    Directory-based install failed, trying component-based..." -ForegroundColor DarkYellow
                    
                    # Try installing with "Unspecified" component first (most common for targets)
                    $TargetInstallCmd = @("cmake", "--install", $ActualBuildPath, "--config", $Config, "--component", "Unspecified")
                    if ($Prefix) {
                        $TargetInstallCmd += "--prefix", $Prefix
                    }
                    if ($VerboseOutput) {
                        $TargetInstallCmd += "--verbose"
                    }
                    
                    & $TargetInstallCmd[0] $TargetInstallCmd[1..($TargetInstallCmd.Length-1)] 2>$null
                    if ($LASTEXITCODE -eq 0) {
                        Write-Host "  | $Target installed successfully" -ForegroundColor Green
                        $InstallSuccess = $true
                        $TargetInstalled = $true
                    } else {
                        # Try installing with target name as component
                        $TargetInstallCmd = @("cmake", "--install", $ActualBuildPath, "--config", $Config, "--component", $Target)
                        if ($Prefix) {
                            $TargetInstallCmd += "--prefix", $Prefix
                        }
                        if ($VerboseOutput) {
                            $TargetInstallCmd += "--verbose"
                        }
                        
                        & $TargetInstallCmd[0] $TargetInstallCmd[1..($TargetInstallCmd.Length-1)] 2>$null
                        if ($LASTEXITCODE -eq 0) {
                            Write-Host "  | $Target installed successfully" -ForegroundColor Green
                            $InstallSuccess = $true
                            $TargetInstalled = $true
                        }
                    }
                }
                
                if (-not $TargetInstalled) {
                    Write-Host "  | Failed to install $Target" -ForegroundColor Yellow
                    $InstallErrors += $Target
                }
            }
            
            if ($InstallSuccess) {
                $InstallExitCode = 0
                if ($InstallErrors.Count -gt 0) {
                    Write-Host "Some targets failed to install: $($InstallErrors -join ', ')" -ForegroundColor Yellow
                }
            } else {
                $InstallExitCode = 1
            }
        } else {
            # Standard full project installation
            $InstallCmd = @("cmake") + $InstallArgs
            
            if ($VerboseOutput) {
                Write-Host "Command: $($InstallCmd -join ' ')" -ForegroundColor DarkGray
            }
            
            & $InstallCmd[0] $InstallCmd[1..($InstallCmd.Length-1)]
            $InstallExitCode = $LASTEXITCODE
        }
    }
    
    # For Emscripten presets, be more lenient with installation failures
    # since some targets may fail to build but others succeed
    if ($InstallExitCode -ne 0) {
        if ($Preset -like "*emscripten*") {
            Write-Host "Installation had errors, but this is common with Emscripten due to incompatible targets" -ForegroundColor Yellow
            Write-Host "Checking what was successfully installed..." -ForegroundColor Blue
            
            # Check if any files were actually installed
            if (Test-Path $Prefix) {
                $InstalledFiles = Get-ChildItem -Recurse $Prefix -ErrorAction SilentlyContinue | Where-Object { -not $_.PSIsContainer }
                if ($InstalledFiles -and $InstalledFiles.Count -gt 0) {
                    Write-Host "Partial installation completed successfully! ($($InstalledFiles.Count) files installed)" -ForegroundColor Green
                } else {
                    throw "Installation failed with exit code $InstallExitCode - no files were installed"
                }
            } else {
                throw "Installation failed with exit code $InstallExitCode - installation directory was not created"
            }
        } else {
            throw "Installation failed with exit code $InstallExitCode"
        }
    } else {
        Write-Host "Installation completed successfully!" -ForegroundColor Green
    }
    
    # Show installation summary
    if (Test-Path $Prefix) {
        $InstalledFiles = Get-ChildItem -Recurse $Prefix | Where-Object { -not $_.PSIsContainer }
        $InstalledSize = ($InstalledFiles | Measure-Object -Property Length -Sum).Sum
        $InstalledSizeMB = [math]::Round($InstalledSize / 1MB, 2)
        
        Write-Host "Installation summary:" -ForegroundColor Cyan
        Write-Host "  Location: $Prefix" -ForegroundColor Cyan
        Write-Host "  Files: $($InstalledFiles.Count)" -ForegroundColor Cyan
        Write-Host "  Size: ${InstalledSizeMB} MB" -ForegroundColor Cyan
        
        if ($VerboseOutput) {
            Write-Host "  Installed files:" -ForegroundColor Cyan
            foreach ($File in $InstalledFiles | Select-Object -First 10) {
                $RelativePath = $File.FullName.Replace($Prefix, "")
                Write-Host "    | $RelativePath" -ForegroundColor DarkCyan
            }
            if ($InstalledFiles.Count -gt 10) {
                Write-Host "    ... and $($InstalledFiles.Count - 10) more files" -ForegroundColor DarkCyan
            }
        }
    }

} catch {
    Write-Host "Installation failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    Pop-Location
}
