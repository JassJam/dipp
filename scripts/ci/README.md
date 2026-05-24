## CDash Submission Scripts

### `submit-cdash.ps1` (PowerShell)

Production script for submitting test results to CDash in CI environments.

**Parameters:**
- `-BuildDir`* - Path to build directory
- `-SourceDir`* - Path to source directory  
- `-BuildName`* - Build name for CDash
- `-CdashSite` - CDash site hostname (from `CTEST_DASHBOARD_SITE` env var)
- `-CdashLocation` - CDash location path (from `CTEST_DASHBOARD_LOCATION` env var)
- `-AuthToken` - Authentication token (from `CDASH_AUTH_TOKEN` env var)
- `-DropMethod` - HTTP method (from `CTEST_DROP_METHOD` env var, default: https)

**Usage in CI:**
```powershell
./scripts/ci/submit-cdash.ps1 `
  -BuildDir "build" `
  -SourceDir "project" `
  -BuildName "windows-msvc-debug"
```
