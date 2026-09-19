$ErrorActionPreference = "Stop"

$token = $env:GITHUB_TOKEN
if (-not $token) {
    $credInput = "protocol=https`nhost=github.com`n`n"
    $credOutput = $credInput | git credential fill
    foreach ($line in ($credOutput -split "`n")) {
        if ($line -match "^password=(.*)$") {
            $token = $matches[1].Trim()
        }
    }
}
if (-not $token) {
    throw "GitHub token not found. Please set GITHUB_TOKEN environment variable or configure git credentials."
}

$repo = "tzdwindows/Formulaic"
$tag = "v1.1.3"
$releaseName = "v1.1.3: Multi-Architecture (x64 / x86), Debug & Release Matrix, High-Precision Implicit Rendering & NuGet MSBuild Support"

$releaseBody = @"
# Formulaic v1.1.3 Release Notes

We are thrilled to announce **Formulaic v1.1.3**, featuring complete multi-architecture (x64 / x86 / 32-bit), multi-configuration (Release / Debug) binary matrices, high-precision implicit curve rendering, and full NuGet MSBuild auto-linking.

---

### Highlights & New Features in v1.1.3

#### 1. Full Multi-Architecture & Multi-Configuration Matrix
* **Architectures**: 64-bit (`x64`) and 32-bit (`x86` / `Win32`).
* **Configurations**: Optimized `Release` and symbol-rich `Debug` binaries for every target.
* **Libraries**: Both dynamic libraries (`Formulaic.dll` + `Formulaic.lib`) and static libraries (`Formulaic_static.lib`).
* **Tools**: All interactive studios (`test_editor_window.exe`, `example_interactive_window.exe`, `test_custom_render.exe`) compiled for both x64 and x86 in Release & Debug.

#### 2. High-Precision Implicit Curve Solver & Sub-Pixel Axis Splitting
* **12-Step Bisection Root Solver**: Replaced single-step secant extrapolation with high-precision interval bisection. Resolves true roots with residual $< 10^{-4}$ while eliminating asymptotic jump poles.
* **Axis-Crossing Micro-Cell Splitting (Axis Splitting)**: Automatically detects when marching cells cross $x=0$ or $y=0$. Accurately captures ultra-narrow asymptotic hyperbola branches (e.g. $1/x + 1/y = 50$, where the branch width is $< 0.3\text{px}$ under $[-20, 20]$ zoom-out) without disappearing.
* **Translucent Anti-Aliased Coordinate Axes**: Coordinate grid lines and axes rendered with Xiaolin Wu anti-aliasing and soft translucent slate gray (`Color::AxisGray`), ensuring curves hugging or crossing axes remain vividly visible.

#### 3. Enhanced NuGet Package (`Formulaic 1.1.3`)
* Complete MSBuild property normalization (`x64`, `x86`, `Win32`, `Release`, `Debug`).
* Dynamic `.targets` rules that auto-select and link the exact matching `.lib` and auto-copy the corresponding `.dll` on build.
* Available directly on [NuGet.org](https://www.nuget.org/packages/Formulaic/1.1.3).

---

### Pre-built Artifacts Included in this Release:
1. `Formulaic-v1.1.3-windows-x64.zip` (64-bit Windows Release & Debug DLLs, static libs, executables, C++20 headers)
2. `Formulaic-v1.1.3-windows-x86.zip` (32-bit Windows Release & Debug DLLs, static libs, executables, C++20 headers)
3. `Formulaic-v1.1.3-windows-all.zip` (Unified multi-platform distribution package)
"@

$headers = @{
    "Authorization" = "Bearer $token"
    "Accept" = "application/vnd.github+json"
    "User-Agent" = "Formulaic-Release-Tool"
    "X-GitHub-Api-Version" = "2022-11-28"
}

# 1. Create GitHub Release
$payload = @{
    tag_name = $tag
    target_commitish = "main"
    name = $releaseName
    body = $releaseBody
    draft = $false
    prerelease = $false
} | ConvertTo-Json -Compress

Write-Output "Creating release $tag on GitHub..."
$url = "https://api.github.com/repos/$repo/releases"
$response = Invoke-RestMethod -Uri $url -Method Post -Headers $headers -Body ([System.Text.Encoding]::UTF8.GetBytes($payload)) -ContentType "application/json; charset=utf-8"

$releaseId = $response.id
$uploadUrlTemplate = $response.upload_url
Write-Output "Created release successfully with ID: $releaseId"
Write-Output "Upload URL template: $uploadUrlTemplate"

# 2. Upload asset zip files
$zipFiles = @(
    "F:\Formulaic\dist\Formulaic-v1.1.3-windows-x64.zip",
    "F:\Formulaic\dist\Formulaic-v1.1.3-windows-x86.zip",
    "F:\Formulaic\dist\Formulaic-v1.1.3-windows-all.zip"
)

foreach ($zipFile in $zipFiles) {
    if (Test-Path $zipFile) {
        $fileName = [System.IO.Path]::GetFileName($zipFile)
        $uploadUrl = $uploadUrlTemplate -replace '\{.*\}', "?name=$fileName"
        Write-Output "Uploading asset $fileName to $uploadUrl..."
        
        $fileBytes = [System.IO.File]::ReadAllBytes($zipFile)
        $uploadHeaders = @{
            "Authorization" = "Bearer $token"
            "User-Agent" = "Formulaic-Release-Tool"
            "Content-Type" = "application/zip"
            "X-GitHub-Api-Version" = "2022-11-28"
        }

        $assetResp = Invoke-RestMethod -Uri $uploadUrl -Method Post -Headers $uploadHeaders -Body $fileBytes
        Write-Output "Asset uploaded successfully: $($assetResp.browser_download_url)"
    } else {
        Write-Warning "Zip file not found at $zipFile"
    }
}

Write-Output "GitHub Release process complete!"
