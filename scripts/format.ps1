#!/usr/bin/env pwsh

<#
.SYNOPSIS
    Code formatting script using clang-format.

.PARAMETER Path
    Root directory path to search for source files. The script will search this
    directory for files matching the specified extensions.
    Default: "." (current directory)
    
    Examples:
    - "." searches current directory
    - "src" searches the src subdirectory
    - "C:\MyProject" searches absolute path

.PARAMETER ClangFormatPath
    Path to the clang-format executable. If not specified, assumes clang-format
    is available in the system PATH.
    Default: "clang-format"
    
    Examples:
    - "clang-format" uses PATH lookup
    - "C:\LLVM\bin\clang-format.exe" uses absolute path
    - "./tools/clang-format" uses relative path

.PARAMETER Extensions
    File extensions to process during formatting. Specify as an array of patterns.
    Default: @("*.hpp", "*.h", "*.hxx", "*.cpp", "*.c", "*.cxx")
    
    Examples:
    - @("*.cpp", "*.h") for only C++ source and headers
    - @("*.c", "*.h") for only C files
    - @("*.hpp", "*.cpp") for modern C++ only

.PARAMETER Recursive
    Enable recursive directory traversal to search subdirectories for source files.
    When disabled, only searches the specified path directory.
    Default: true
    
    Use -Recursive:$false to search only the target directory without subdirectories.

.PARAMETER WhatIf
    Enable dry-run mode that shows what files would be formatted without actually
    modifying them. Useful for previewing changes before applying formatting.
    Default: false
    
    Perfect for validating which files will be affected before running the actual formatting.

.PARAMETER Verbose
    Enable verbose output showing detailed information about each file being processed.
    Displays the full path of each file as it's being formatted.
    Default: false

.PARAMETER Jobs
    Number of parallel formatting jobs to run simultaneously. Higher values can
    speed up formatting on systems with many files and multiple CPU cores.
    Default: 1 (sequential processing)
    
    Set to 0 to use the number of CPU cores available on the system.

.PARAMETER ConfigFile
    Path to a specific .clang-format configuration file to use instead of the
    automatic discovery. If not specified, clang-format will search up the
    directory tree for configuration files.
    Default: "" (automatic discovery)

.PARAMETER ExcludePatterns
    Array of file or directory patterns to exclude from formatting. Supports
    wildcards and relative paths from the search root.
    Default: @() (no exclusions)
    
    Examples:
    - @("*test*", "third-party/*") excludes test files and third-party directory
    - @("*.generated.cpp") excludes generated source files

.EXAMPLE
    .\scripts\format.ps1
    
    Format all C/C++ files in the current directory and subdirectories using
    default settings. This is the most common usage for project-wide formatting.

.EXAMPLE
    .\scripts\format.ps1 -Path "src" -WhatIf -Verbose
    
    Preview what files in the src directory would be formatted, with detailed
    output showing each file that would be processed.

.EXAMPLE
    .\scripts\format.ps1 -Extensions @("*.cpp", "*.h") -Recursive:$false
    
    Format only .cpp and .h files in the current directory without searching
    subdirectories. Useful for formatting specific file types in a single location.

.EXAMPLE
    .\scripts\format.ps1 -Path "src" -ExcludePatterns @("*test*", "third-party/*") -Verbose
    
    Format all source files in src directory but exclude any files/directories
    containing "test" and the entire "third-party" directory.

.EXAMPLE
    .\scripts\format.ps1 -ClangFormatPath "C:\LLVM\bin\clang-format.exe" -Jobs 4
    
    Use a specific clang-format executable and process up to 4 files in parallel
    for faster formatting on multi-core systems.

.NOTES
    Requires clang-format to be installed and available. The script will verify
    clang-format availability before processing any files. Uses your project's
    .clang-format configuration file automatically.

.LINK
    https://clang.llvm.org/docs/ClangFormat.html
#>
param(
    [string]$Path = ".",
    [string]$ClangFormatPath = "clang-format",
    [string[]]$Extensions = @("*.hpp", "*.h", "*.hxx", "*.cpp", "*.c", "*.cxx"),
    [switch]$Recursive = $true,
    [switch]$WhatIf,
    [switch]$Verbose,
    [int]$Jobs = 1,
    [string]$ConfigFile = "",
    [string[]]$ExcludePatterns = @()
)

# Set error action preference
$ErrorActionPreference = "Stop"

# Get script directory and project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Determine number of jobs if set to auto-detect
if ($Jobs -eq 0) {
    $Jobs = [Environment]::ProcessorCount
}

# Platform detection for consistent behavior
$Platform = if ($PSVersionTable.PSVersion.Major -ge 6) {
    if ($IsWindows) { "Windows" }
    elseif ($IsLinux) { "Linux" }
    else { "macOS" }
} else {
    if ($env:OS -eq "Windows_NT") { "Windows" } else { "Unix" }
}

# Color coding functions for consistent output
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Write-Success {
    param([string]$Message)
    Write-ColorOutput "$Message" "Green"
}

function Write-Warning {
    param([string]$Message)
    Write-ColorOutput "$Message" "Yellow"
}

function Write-Error {
    param([string]$Message)
    Write-ColorOutput "$Message" "Red"
}

function Write-Info {
    param([string]$Message)
    Write-ColorOutput "$Message" "Cyan"
}

# Verify clang-format availability
function Test-ClangFormat {
    param([string]$ClangFormatPath)
    
    try {
        $versionOutput = & $ClangFormatPath --version 2>$null
        if ($LASTEXITCODE -eq 0) {
            return @{
                Available = $true
                Version = $versionOutput
            }
        }
        return @{ Available = $false; Version = "" }
    }
    catch {
        return @{ Available = $false; Version = "" }
    }
}

# Check if file should be excluded based on patterns
function Test-ShouldExclude {
    param(
        [string]$FilePath,
        [string[]]$ExcludePatterns,
        [string]$BasePath
    )
    
    if ($ExcludePatterns.Count -eq 0) {
        return $false
    }
    
    # Get relative path for pattern matching
    $RelativePath = [System.IO.Path]::GetRelativePath($BasePath, $FilePath)
    
    foreach ($pattern in $ExcludePatterns) {
        if ($RelativePath -like $pattern -or (Split-Path -Leaf $FilePath) -like $pattern) {
            return $true
        }
    }
    
    return $false
}

# Format a single file with error handling
function Format-File {
    param(
        [string]$FilePath,
        [string]$ClangFormatPath,
        [string]$ConfigFile,
        [switch]$WhatIf,
        [switch]$Verbose
    )
    
    try {
        if ($WhatIf) {
            Write-ColorOutput "Would format: $FilePath" "Yellow"
            return @{ Success = $true; Message = "Dry run" }
        }
        
        if ($Verbose) {
            Write-Info "Formatting: $FilePath"
        }
        
        # Build clang-format command
        $FormatCmd = @($ClangFormatPath, "-i")
        
        if ($ConfigFile -and (Test-Path $ConfigFile)) {
            $FormatCmd += "--style=file:$ConfigFile"
        }
        
        $FormatCmd += $FilePath
        
        # Apply clang-format
        & $FormatCmd[0] $FormatCmd[1..($FormatCmd.Length-1)]
        
        if ($LASTEXITCODE -eq 0) {
            if (-not $WhatIf) {
                Write-Success "Formatted: $(Split-Path -Leaf $FilePath)"
            }
            return @{ Success = $true; Message = "Formatted successfully" }
        }
        else {
            $errorMsg = "clang-format exited with code $LASTEXITCODE"
            Write-Error "Failed to format $(Split-Path -Leaf $FilePath): $errorMsg"
            return @{ Success = $false; Message = $errorMsg }
        }
    }
    catch {
        $errorMsg = $_.Exception.Message
        Write-Error "Error formatting $(Split-Path -Leaf $FilePath): $errorMsg"
        return @{ Success = $false; Message = $errorMsg }
    }
}

# Main script execution
Write-ColorOutput "clang-format Code Formatter" "Magenta"
Write-ColorOutput "Platform: $Platform" "Green"

# Resolve and validate paths
$ResolvedPath = Resolve-Path -Path $Path -ErrorAction SilentlyContinue
if (-not $ResolvedPath) {
    Write-Error "Path not found: $Path"
    exit 1
}

$SearchPath = $ResolvedPath.Path
Write-Info "Search path: $SearchPath"
Write-Info "Extensions: $($Extensions -join ', ')"
Write-Info "Recursive: $Recursive"

if ($WhatIf) {
    Write-Warning "DRY RUN MODE - No files will be modified"
}

if ($ExcludePatterns.Count -gt 0) {
    Write-Info "Exclude patterns: $($ExcludePatterns -join ', ')"
}

# Verify clang-format availability
Write-Info "Verifying clang-format availability..."
$ClangFormatInfo = Test-ClangFormat $ClangFormatPath

if (-not $ClangFormatInfo.Available) {
    Write-Error "clang-format not found at '$ClangFormatPath'"
    Write-Error "Please ensure clang-format is installed and in your PATH, or specify the path with -ClangFormatPath"
    exit 1
}

Write-Success "Using: $($ClangFormatInfo.Version)"

# Validate config file if specified
if ($ConfigFile) {
    if (Test-Path $ConfigFile) {
        Write-Info "Using config file: $ConfigFile"
    } else {
        Write-Error "Config file not found: $ConfigFile"
        exit 1
    }
} else {
    # Look for .clang-format in search path, project subdirectory, or project root
    $PossibleConfigs = @(
        (Join-Path $SearchPath ".clang-format"),
        (Join-Path $ProjectRoot "project" ".clang-format")
    )
    
    foreach ($Config in $PossibleConfigs) {
        if (Test-Path $Config) {
            Write-Info "Found config file: $Config"
            break
        }
    }
}

Write-ColorOutput "" "White"

# Discover source files
Write-Info "Discovering source files..."

$SearchParams = @{
    Path = $SearchPath
    Include = $Extensions
    File = $true
}

if ($Recursive) {
    $SearchParams.Recurse = $true
}

try {
    $AllFiles = Get-ChildItem @SearchParams | Sort-Object FullName
    
    # Apply exclusion patterns
    $FilesToFormat = @()
    $ExcludedFiles = @()
    
    foreach ($File in $AllFiles) {
        if (Test-ShouldExclude -FilePath $File.FullName -ExcludePatterns $ExcludePatterns -BasePath $SearchPath) {
            $ExcludedFiles += $File
        } else {
            $FilesToFormat += $File
        }
    }
    
    if ($AllFiles.Count -eq 0) {
        Write-Warning "No matching files found in '$SearchPath'"
        exit 0
    }
    
    Write-Success "Found $($AllFiles.Count) file(s) matching criteria"
    
    if ($ExcludedFiles.Count -gt 0) {
        Write-Warning "Excluded $($ExcludedFiles.Count) file(s) based on patterns"
        if ($Verbose) {
            foreach ($ExcludedFile in $ExcludedFiles) {
                Write-ColorOutput "  Excluded: $($ExcludedFile.FullName)" "DarkYellow"
            }
        }
    }
    
    if ($FilesToFormat.Count -eq 0) {
        Write-Warning "No files to format after applying exclusions"
        exit 0
    }
    
    Write-Info "Processing $($FilesToFormat.Count) file(s) to format"
    Write-ColorOutput "" "White"
    
    # Process files
    $StartTime = Get-Date
    $SuccessCount = 0
    $FailureCount = 0
    $FailedFiles = @()
    
    if ($Jobs -gt 1 -and $FilesToFormat.Count -gt 1 -and -not $WhatIf) {
        # Parallel processing for multiple files
        Write-Info "Using parallel processing with $Jobs job(s)"
        
        $FilesToFormat | ForEach-Object -Parallel {
            $File = $_
            $ClangFormatPath = $using:ClangFormatPath
            $ConfigFile = $using:ConfigFile
            $Verbose = $using:Verbose
            
            # Import the Format-File function into the parallel runspace
            function Format-File {
                param(
                    [string]$FilePath,
                    [string]$ClangFormatPath,
                    [string]$ConfigFile,
                    [switch]$WhatIf,
                    [switch]$Verbose
                )
                
                try {
                    $FormatCmd = @($ClangFormatPath, "-i")
                    
                    if ($ConfigFile -and (Test-Path $ConfigFile)) {
                        $FormatCmd += "--style=file:$ConfigFile"
                    }
                    
                    $FormatCmd += $FilePath
                    
                    & $FormatCmd[0] $FormatCmd[1..($FormatCmd.Length-1)]
                    
                    if ($LASTEXITCODE -eq 0) {
                        return @{ Success = $true; Message = "Formatted successfully"; File = $FilePath }
                    }
                    else {
                        return @{ Success = $false; Message = "clang-format exited with code $LASTEXITCODE"; File = $FilePath }
                    }
                }
                catch {
                    return @{ Success = $false; Message = $_.Exception.Message; File = $FilePath }
                }
            }
            
            Format-File -FilePath $File.FullName -ClangFormatPath $ClangFormatPath -ConfigFile $ConfigFile -Verbose:$Verbose
            
        } -ThrottleLimit $Jobs | ForEach-Object {
            if ($_.Success) {
                Write-Success "Formatted: $(Split-Path -Leaf $_.File)"
                $SuccessCount++
            } else {
                Write-Error "Failed: $(Split-Path -Leaf $_.File) - $($_.Message)"
                $FailedFiles += $_.File
                $FailureCount++
            }
        }
    } else {
        # Sequential processing
        foreach ($File in $FilesToFormat) {
            $Result = Format-File -FilePath $File.FullName -ClangFormatPath $ClangFormatPath -ConfigFile $ConfigFile -WhatIf:$WhatIf -Verbose:$Verbose
            
            if ($Result.Success) {
                $SuccessCount++
            } else {
                $FailureCount++
                $FailedFiles += $File.FullName
            }
        }
    }
    
    $EndTime = Get-Date
    $Duration = $EndTime - $StartTime
    
    # Summary report
    Write-ColorOutput "" "White"
    Write-ColorOutput "Formatting Summary" "Magenta"
    Write-ColorOutput "Total files processed: $($FilesToFormat.Count)" "White"
    Write-Success "Successfully formatted: $SuccessCount"
    
    if ($FailureCount -gt 0) {
        Write-Error "Failed to format: $FailureCount"
        if ($Verbose -and $FailedFiles.Count -gt 0) {
            Write-ColorOutput "Failed files:" "Red"
            foreach ($FailedFile in $FailedFiles) {
                Write-ColorOutput "  - $FailedFile" "Red"
            }
        }
    }
    
    Write-Info "Processing time: $($Duration.TotalSeconds.ToString("F2")) seconds"
    
    if ($WhatIf) {
        Write-Warning "This was a dry run - no files were actually modified"
        Write-Info "Remove -WhatIf parameter to apply formatting"
    }
    
    # Exit with appropriate code
    if ($FailureCount -gt 0) {
        exit 1
    } else {
        if (-not $WhatIf) {
            Write-Success "All files formatted successfully!"
        }
        exit 0
    }
    
} catch {
    Write-Error "Unexpected error: $($_.Exception.Message)"
    exit 1
}