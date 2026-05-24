#!/usr/bin/env pwsh
# CI CDash Submission Script

param(
    [Parameter(Mandatory=$true)]
    [string]$BuildDir,
    
    [Parameter(Mandatory=$true)]
    [string]$SourceDir,
    
    [Parameter(Mandatory=$true)]
    [string]$BuildName,
    
    [Parameter(Mandatory=$true)]
    [string]$Preset,
    
    [string]$CdashSite = $env:CTEST_DASHBOARD_SITE,
    [string]$CdashLocation = $env:CTEST_DASHBOARD_LOCATION,
    [string]$AuthToken = $env:CDASH_AUTH_TOKEN,
    [string]$DropMethod = $env:CTEST_DROP_METHOD,
    [string]$DashboardModel = $env:CTEST_DASHBOARD_MODEL
)

# Validate required parameters
if ([string]::IsNullOrEmpty($CdashSite)) {
    Write-Warning "CTEST_DASHBOARD_SITE not set - skipping CDash submission"
    exit 0
}

if ([string]::IsNullOrEmpty($CdashLocation)) {
    Write-Warning "CTEST_DASHBOARD_LOCATION not set - skipping CDash submission"
    exit 0
}

# Set defaults
if ([string]::IsNullOrEmpty($DropMethod)) {
    $DropMethod = "https"
}

if ([string]::IsNullOrEmpty($DashboardModel)) {
    $DashboardModel = "Experimental"
}

# Derive build configuration from preset name
if ($Preset -match "debug") {
    $BuildConfig = "Debug"
} elseif ($Preset -match "release") {
    $BuildConfig = "Release"
} else {
    # Default fallback
    $BuildConfig = "Release"
}

Write-Host "=== CI CDash Submission ==="
Write-Host "Build Directory: $BuildDir"
Write-Host "Source Directory: $SourceDir"
Write-Host "Build Name: $BuildName"
Write-Host "Preset: $Preset"
Write-Host "Build Configuration: $BuildConfig"
Write-Host "CDash Site: $CdashSite"
Write-Host "CDash Location: $CdashLocation"
Write-Host "Dashboard Model: $DashboardModel"
Write-Host "Auth Token: $($AuthToken.Length -gt 0 ? 'Present' : 'Not provided')"

# Validate directories exist
if (-not (Test-Path $BuildDir)) {
    Write-Error "Build directory does not exist: $BuildDir"
    exit 1
}

if (-not (Test-Path $SourceDir)) {
    Write-Error "Source directory does not exist: $SourceDir"
    exit 1
}

# Resolve to absolute paths
$BuildDir = Resolve-Path $BuildDir
$SourceDir = Resolve-Path $SourceDir

# Set up environment variables for CTest script
$env:CTEST_SOURCE_DIRECTORY = $SourceDir
$env:CTEST_BINARY_DIRECTORY = $BuildDir
$env:CTEST_BUILD_NAME = $BuildName
$env:CTEST_SITE = $env:RUNNER_NAME ?? $env:COMPUTERNAME ?? "CI"
$env:CTEST_DROP_SITE = $CdashSite
$env:CTEST_DROP_LOCATION = $CdashLocation
$env:CTEST_DROP_METHOD = $DropMethod
$env:CTEST_DASHBOARD_MODEL = $DashboardModel

# Set auth token if provided
if ($AuthToken.Length -gt 0) {
    $env:CTEST_CDASH_AUTH_TOKEN = $AuthToken
    Write-Host "CDash authentication configured"
} else {
    Write-Host "No authentication token provided - submitting without auth"
}

# Change to build directory and run CTest script
Push-Location $BuildDir -StackName "CDashSubmission"
$CTestScript = Join-Path $SourceDir "CTestScript.cmake"

if (-not (Test-Path $CTestScript)) {
    Write-Error "CTest script not found: $CTestScript"
    exit 1
}

Write-Host "`n=== Running CTest Submission ==="

# Check if any tests exist before running dashboard submission
Write-Host "🔍 Checking for available tests..." -ForegroundColor Blue

# Use ctest --show-only to check if tests exist
$TestCheckCmd = @("ctest", "--test-dir", $BuildDir, "--build-config", $BuildConfig, "--show-only=json-v1")

try {
    $TestOutput = & $TestCheckCmd[0] $TestCheckCmd[1..($TestCheckCmd.Length-1)] 2>$null
    $TestCheckResult = $LASTEXITCODE
    
    if ($TestCheckResult -eq 0 -and $TestOutput) {
        $TestInfo = $TestOutput | ConvertFrom-Json -ErrorAction SilentlyContinue
        $TestCount = if ($TestInfo.tests) { $TestInfo.tests.Count } else { 0 }
        
        if ($TestCount -eq 0) {
            Write-Host "No tests found - skipping CDash test submission" -ForegroundColor Yellow
            Write-Host "This usually means BUILD_TESTING=OFF or no test targets were defined" -ForegroundColor Yellow
            Write-Host "CDash submission completed (no tests to submit)" -ForegroundColor Green
            exit 0
        } else {
            Write-Host "Found $TestCount test(s) - proceeding with CDash submission" -ForegroundColor Green
        }
    } else {
        Write-Host "Could not determine test count, proceeding with submission..." -ForegroundColor Yellow
    }
} catch {
    Write-Host "Could not check for tests, proceeding with submission..." -ForegroundColor Yellow
}

# Set environment variable for the CTest script
$env:CTEST_CONFIGURATION_TYPE = $BuildConfig

# Always include --build-config parameter - it's ignored by single-config generators
$ctestArgs = @("-S", $CTestScript, "--build-config", $BuildConfig, "--verbose", "--output-on-failure")
Write-Host "Command: ctest -S $CTestScript --build-config $BuildConfig --verbose --output-on-failure" -ForegroundColor Gray

ctest @ctestArgs
$ctestExitCode = $LASTEXITCODE

Pop-Location -StackName "CDashSubmission"

if ($ctestExitCode -eq 0) {
    Write-Host "CDash submission completed successfully"
    exit 0
} else {
    Write-Error "CTest execution failed with exit code: $ctestExitCode"
    Write-Error "This indicates that tests failed or there was an issue with the test execution"
    exit $ctestExitCode
}
