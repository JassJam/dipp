#!/usr/bin/env pwsh

<#
.SYNOPSIS
    Test script for project to run unit and integration tests.

.PARAMETER Config
    Build configuration to test. Must be either 'Debug' or 'Release'.
    Default: Release
    
    Debug builds may include additional test assertions and debugging information,
    while Release builds test the optimized code paths that will be deployed.

.PARAMETER Preset
    CMake preset to use for testing. If not specified, automatically determined based on
    platform and configuration:
    - Windows: test-windows-msvc-debug/release, test-windows-clang-debug/release
    - Unix-like: test-unixlike-gcc-debug/release, test-unixlike-clang-debug/release

.PARAMETER Compiler
    Specific compiler to use for testing. Must be one of: 'msvc', 'clang', 'gcc'.
    If not specified, uses platform default (MSVC on Windows, GCC on Unix-like).
    
    This parameter changes the CMake preset to use the specified compiler for testing.
    - msvc: Only available on Windows, uses Visual Studio compiler
    - clang: Uses Clang/Clang-cl compiler
    - gcc: Uses GNU Compiler Collection

.PARAMETER BuildDir
    Build directory containing the test executables. Should match the directory
    used during the build process.
    Default: "out" (matches cmake-initializer preset structure)

.PARAMETER Parallel
    Number of parallel test jobs to run simultaneously. Higher values can speed up
    test execution on multi-core systems but may cause resource contention.
    Default: Number of CPU cores detected on the system

.PARAMETER Filter
    Regular expression pattern to filter which tests to run. Only tests matching
    the pattern will be executed. Useful for running specific test suites or
    categories during development.
    Default: (empty - run all tests)
    
    Examples:
    - "Unit.*" - Run only unit tests
    - ".*Integration.*" - Run only integration tests
    - "MyClass.*" - Run tests for specific class

.PARAMETER Timeout
    Maximum time in seconds to wait for each individual test to complete.
    Tests exceeding this limit will be terminated and marked as failed.
    Default: 300 (5 minutes)

.PARAMETER Repeat
    Number of times to repeat the test suite. Useful for detecting flaky tests
    or performance regressions. Each iteration runs the complete test suite.
    Default: 1

.PARAMETER Output
    Output format for test results. Supported formats:
    - 'default': Standard CTest output
    - 'verbose': Detailed test output with individual test results
    - 'junit': JUnit XML format for CI/CD integration
    - 'json': JSON format for programmatic processing
    Default: default

.PARAMETER Coverage
    Enable code coverage reporting. Requires gcov/llvm-cov to be available.
    Generates coverage reports showing which parts of the code are tested.
    Default: false
    
    Coverage reports are generated in HTML format in the build directory.

.PARAMETER Valgrind
    Run tests under Valgrind for memory error detection (Linux/macOS only).
    Helps detect memory leaks, buffer overflows, and other memory-related issues.
    Default: false
    
    Note: Significantly increases test execution time but provides valuable
    debugging information for memory-related issues.

.PARAMETER StopOnFailure
    Stop test execution immediately when the first test fails. Useful during
    development to get quick feedback on test failures.
    Default: false (continue running all tests)

.PARAMETER Verbose
    Enable verbose test output showing detailed execution information, including
    individual test results, timing information, and system details.
    Default: false

.PARAMETER ExtraArgs
    Additional arguments to pass directly to CTest commands. Useful for passing
    custom variables or options that aren't covered by other parameters.
    Example: @("--rerun-failed", "--extra-verbose")

.EXAMPLE
    .\scripts\test.ps1
    
    Run all tests with default settings (Release configuration, auto-detected preset).
    This is the most common usage for validating the project.

.EXAMPLE
    .\scripts\test.ps1 -Config Debug -Verbose
    
    Run all tests in Debug configuration with verbose output.
    Useful during development to see detailed test execution information.

.EXAMPLE
    .\scripts\test.ps1 -Filter "Unit.*" -Parallel 4
    
    Run only unit tests using 4 parallel jobs.
    Good for focused testing of specific components during development.

.EXAMPLE
    .\scripts\test.ps1 -Coverage -Output junit
    
    Run tests with code coverage and generate JUnit XML output.
    Ideal for CI/CD pipelines that need coverage reports and test result integration.

.EXAMPLE
    .\scripts\test.ps1 -Repeat 10 -StopOnFailure
    
    Run the test suite 10 times, stopping at the first failure.
    Useful for detecting intermittent test failures or race conditions.

.NOTES
    Requires CMake 3.21+ and CTest. Test executables must be built before running this script.
    For best results, ensure tests are built with the same configuration being tested.

.LINK
    https://github.com/Justjam/cmake-initializer
#>
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [Parameter(Mandatory=$true)]
    [string]$Preset,
    [string[]]$Targets = @(),
    [string[]]$ExcludeTargets = @(),
    [string]$BuildDir = "out",
    [int]$Parallel = 0,
    [string]$Filter = "",
    [int]$Timeout = 300,
    [int]$Repeat = 1,
    [ValidateSet("default", "verbose", "junit", "json")]
    [string]$Output = "default",
    [switch]$Coverage,
    [switch]$Valgrind,
    [switch]$StopOnFailure,
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

# Determine number of parallel jobs if not specified
if ($Parallel -eq 0) {
    $Parallel = [Environment]::ProcessorCount
}

# Platform detection
$Platform = if ($PSVersionTable.PSVersion.Major -ge 6) {
    if ($IsWindows) { "Windows" }
    elseif ($IsLinux) { "Linux" }
    else { "macOS" }
} else {
    if ($env:OS -eq "Windows_NT") { "Windows" } else { "Unix" }
}

Write-Host "cmake-initializer Test Script" -ForegroundColor Cyan
Write-Host "Platform: $Platform" -ForegroundColor Green

# Derive build configuration from preset name
if ($Preset -match "debug") {
    $Config = "Debug"
} elseif ($Preset -match "release") {
    $Config = "Release"
}

Write-Host "Configuration: $Config" -ForegroundColor Green
Write-Host "Test Preset: $Preset" -ForegroundColor Green

# Change to project directory
Push-Location $ProjectDir

try {
    # Determine build directory from preset - use workspace root with configurable BuildDir
    $FullBuildDir = Join-Path $ProjectRoot "$BuildDir/build/$Preset"
    
    if (-not (Test-Path $FullBuildDir)) {
        throw "Build directory not found: $FullBuildDir. Please run build script first."
    }

    Write-Host "Build directory: $FullBuildDir" -ForegroundColor DarkGray

    # Check if tests are available
    $TestFiles = Get-ChildItem -Path $FullBuildDir -Recurse -Include "*.exe", "*test*" -File | Where-Object { $_.Name -match "test" }
    if ($TestFiles.Count -eq 0) {
        Write-Warning "No test executables found in build directory. Make sure tests are built with BUILD_TESTING=ON."
    }

    # If targets are specified, build them first
    if ($Targets.Count -gt 0) {
        # Filter targets based on exclusions - ensure we maintain array structure
        $TargetsToBuild = @($Targets | Where-Object { $_ -notin $ExcludeTargets })
        
        if ($TargetsToBuild.Count -eq 0) {
            Write-Warning "All specified targets were excluded. No targets to build before testing."
        } else {
            Write-Host "Building test targets before running tests..." -ForegroundColor Blue
            Write-Host "Targets to build: $($TargetsToBuild -join ', ')" -ForegroundColor Green
            if ($ExcludeTargets.Count -gt 0) {
                Write-Host "Excluded targets: $($ExcludeTargets -join ', ')" -ForegroundColor Red
            }
            
            $BuildResults = @()
            foreach ($Target in $TargetsToBuild) {
                Write-Host "Building target: $Target" -ForegroundColor Yellow
                $BuildCmd = @("cmake", "--build", $FullBuildDir, "--config", $Config, "--target", $Target)
                
                if ($VerboseOutput) {
                    Write-Host "Build command: $($BuildCmd -join ' ')" -ForegroundColor DarkGray
                }
                
                & $BuildCmd[0] $BuildCmd[1..($BuildCmd.Length-1)]
                if ($LASTEXITCODE -eq 0) {
                    Write-Host "Successfully built target: $Target" -ForegroundColor Green
                    $BuildResults += @{Target = $Target; Success = $true}
                } else {
                    Write-Host "Failed to build target: $Target (exit code $LASTEXITCODE)" -ForegroundColor Red
                    $BuildResults += @{Target = $Target; Success = $false}
                }
            }
            
            # Report build summary
            $SuccessfulBuilds = $BuildResults | Where-Object { $_.Success }
            $FailedBuilds = $BuildResults | Where-Object { -not $_.Success }
            
            Write-Host "`nBuild Summary:" -ForegroundColor Cyan
            Write-Host "  | Successful: $($SuccessfulBuilds.Count)" -ForegroundColor Green
            Write-Host "  | Failed: $($FailedBuilds.Count)" -ForegroundColor Red
            
            if ($FailedBuilds.Count -gt 0) {
                $FailedTargetNames = ($FailedBuilds | ForEach-Object { $_.Target }) -join ', '
                throw "Build failed for the following test targets: $FailedTargetNames"
            }
        }
    }

    # Check if any tests exist before running CTest
    Write-Host "Checking for available tests..." -ForegroundColor Blue
    
    # Use ctest --show-only to check if tests exist without running them
    $TestCheckCmd = @("ctest", "--test-dir", $FullBuildDir, "--build-config", $Config, "--show-only=json-v1")
    
    try {
        $TestOutput = & $TestCheckCmd[0] $TestCheckCmd[1..($TestCheckCmd.Length-1)] 2>$null
        $TestCheckResult = $LASTEXITCODE
        
        if ($TestCheckResult -eq 0 -and $TestOutput) {
            # Parse the JSON to count tests
            $TestInfo = $TestOutput | ConvertFrom-Json -ErrorAction SilentlyContinue
            $TestCount = if ($TestInfo.tests) { $TestInfo.tests.Count } else { 0 }
            
            if ($TestCount -eq 0) {
                Write-Host "No tests were found to run" -ForegroundColor Yellow
                Write-Host "This usually means BUILD_TESTING=OFF or no test targets were defined" -ForegroundColor Yellow
                Write-Host "Test execution completed (no tests to run)" -ForegroundColor Green
                exit 0
            } else {
                Write-Host "Found $TestCount test(s) to run" -ForegroundColor Green
            }
        } else {
            Write-Host "Could not determine test count, proceeding with test execution..." -ForegroundColor Yellow
        }
    } catch {
        Write-Host "Could not check for tests, proceeding with test execution..." -ForegroundColor Yellow
    }

    # Build CTest command
    $CTestCmd = @("ctest", "--test-dir", $FullBuildDir, "--build-config", $Config)
    
    # Add extra arguments if provided
    if ($ExtraArgs -and $ExtraArgs.Count -gt 0) {
        $CTestCmd += $ExtraArgs
        Write-Host "Extra CTest args: $($ExtraArgs -join ' ')" -ForegroundColor Yellow
    }
    
    # Add parallel execution
    $CTestCmd += @("--parallel", $Parallel)
    
    # Add timeout
    $CTestCmd += @("--timeout", $Timeout)
    
    # Add test filter if specified
    if ($Filter) {
        $CTestCmd += @("-R", $Filter)
        Write-Host "Test filter: $Filter" -ForegroundColor Yellow
    }
    
    # Add repeat count
    if ($Repeat -gt 1) {
        $CTestCmd += @("--repeat", "until-pass:$Repeat")
        Write-Host "Repeat count: $Repeat" -ForegroundColor Yellow
    }
    
    # Configure output format
    switch ($Output) {
        "verbose" {
            $CTestCmd += @("--verbose", "--output-on-failure")
        }
        "junit" {
            $JUnitFile = Join-Path $FullBuildDir "test-results.xml"
            $CTestCmd += @("--output-junit", $JUnitFile)
            Write-Host "JUnit output: $JUnitFile" -ForegroundColor DarkGray
        }
        "json" {
            $JsonFile = Join-Path $FullBuildDir "test-results.json"
            $CTestCmd += @("--output-json", $JsonFile)
            Write-Host "JSON output: $JsonFile" -ForegroundColor DarkGray
        }
        "default" {
            $CTestCmd += @("--output-on-failure")
        }
    }
    
    # Add stop on failure
    if ($StopOnFailure) {
        $CTestCmd += @("--stop-on-failure")
    }
    
    # Add coverage support
    if ($Coverage) {
        Write-Host "Enabling code coverage..." -ForegroundColor Blue
        $CTestCmd += @("-T", "Coverage")
    }
    
    # Add Valgrind support
    if ($Valgrind) {
        if ($Platform -eq "Windows") {
            Write-Warning "Valgrind is not available on Windows. Skipping memory check."
        } else {
            Write-Host "Enabling Valgrind memory check..." -ForegroundColor Blue
            $CTestCmd += @("-T", "MemCheck")
        }
    }
    
    # Add verbose output
    if ($VerboseOutput) {
        $CTestCmd += @("--verbose")
        Write-Host "Command: $($CTestCmd -join ' ')" -ForegroundColor DarkGray
    }
    
    # Run tests
    Write-Host "Running tests..." -ForegroundColor Blue
    $StartTime = Get-Date
    
    & $CTestCmd[0] $CTestCmd[1..($CTestCmd.Length-1)]
    $TestExitCode = $LASTEXITCODE
    
    $EndTime = Get-Date
    $Duration = $EndTime - $StartTime
    
    # Report results
    if ($TestExitCode -eq 0) {
        Write-Host "All tests passed!" -ForegroundColor Green
    } else {
        Write-Host "Some tests failed (exit code: $TestExitCode)" -ForegroundColor Red
    }
    
    Write-Host "Test duration: $($Duration.ToString('mm\:ss'))" -ForegroundColor Cyan
    
    # Show coverage results if enabled
    if ($Coverage) {
        $CoverageDir = Join-Path $FullBuildDir "Coverage"
        if (Test-Path $CoverageDir) {
            Write-Host "Coverage report generated in: $CoverageDir" -ForegroundColor Cyan
        }
    }
    
    # Show memory check results if enabled
    if ($Valgrind -and $Platform -ne "Windows") {
        $MemCheckDir = Join-Path $FullBuildDir "DynamicAnalysis"
        if (Test-Path $MemCheckDir) {
            Write-Host "Memory check report generated in: $MemCheckDir" -ForegroundColor Cyan
        }
    }
    
    # Exit with the same code as CTest
    exit $TestExitCode

} catch {
    Write-Host "Test execution failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    Pop-Location
}
