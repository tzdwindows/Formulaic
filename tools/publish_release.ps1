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
$releaseName = "v1.2.0: Interactive 3D Surface Preview, Bidirectional LaTeXLive Engine, Vector LaTeX Visual Renderer & GMP Multi-Precision"

$releaseBody = @"
# Formulaic v1.2.0 Release Notes

We are thrilled to announce **Formulaic v1.2.0**, a major milestone release featuring **interactive 3D surface mathematical visualization**, full bidirectional **LaTeXLive** mathematical formula conversion, academic-grade **vector LaTeX visual math rendering**, portable multi-precision arithmetic via **GNU MP (GMP / mini-gmp)**, and major upgrades to the interactive mathematical studio!

---

### Highlights & New Features in v1.2.0

#### 1. Interactive 3D Surface Visualization Engine (`Formulaic::RasterEngine::plot_surface_3d`)
* **3D Mathematical Mesh Generation**: Real-time rendering of bivariate scalar surfaces $z = f(x, y)$, including the classic Sombrero / 2D sinc wave `$z = \frac{\sin\left(\sqrt{x^2 + y^2}\right)}{\sqrt{x^2 + y^2}}$`.
* **High-Performance Software Raster Pipeline**:
  - Full 3D camera model with configurable Azimuth (yaw), Elevation (pitch), and zoom.
  - Triangle-based Painter's algorithm with centroid depth sorting for occlusion handling.
  - Dual-sided diffuse Lambertian lighting model (`$I = 0.52 + 0.48 |\vec{N} \cdot \vec{L}|$`).
  - Per-vertex height-normalized colormap shading (Viridis colormap) with wireframe mesh overlays.
  - 3D coordinate bounding box and labeled Cartesian axes (+X, +Y, +Z).
* **Interactive Studio Control in `test_editor_window.exe`**:
  - Left-click drag to rotate 3D view smoothly in 360 degrees.
  - Mouse wheel to zoom in and out.
  - Double-click to reset view to default isometric perspective.
  - Automatic detection of `z = ...` formulas and one-click Preset 5 (`3D Sombrero z=sinc(r)`).

#### 2. Standard Bidirectional LaTeXLive Converter (`Formulaic::LatexConverter`)
* **Forward Converter (`to_latex`)**: Automatically translates Formulaic mathematical expressions, implicit equations, and multi-line assignment scripts into standard LaTeXLive display code.
  - Automatically translates fractions (`\frac{numerator}{denominator}`) without extraneous outer parentheses.
  - Converts Greek symbols (`\alpha`, `\theta`, `\nu`, `\pi`) and mathematical constants.
  - Formats multi-character subscripts (`u0` -> `u_{0}`, `lap_u` -> `\text{lap}_{u}`).
  - Formats power functions (e.g. `\sin^{2}\left(x\right)`).
  - Translates multi-line scripts into vertically aligned LaTeX `\begin{aligned} ... \end{aligned}` blocks.
* **Reverse Compiler (`to_script`)**: Effortlessly compiles raw LaTeX code back into executable Formulaic scripts.
  - Automatically translates `\begin{aligned}` and `\begin{cases}` environments.
  - Infers and declares free parameters (e.g. automatically declaring `let a = 3.0;` for three-petal rose parametric systems).
  - Directly evaluate LaTeX strings via `Expression::parse_latex("\\sin(x) + \\cos(y)")`.

#### 3. Vector-Grade LaTeXLive Visual Math Rendering Engine (`Formulaic::LatexRenderModule`)
* **Dynamic Scaled Delimiters (`DelimitedBox`)**: Automatically computes smooth cubic Bezier curvature for `\left( ... \right)`, `\left[ ... \right]`, `\left\{ ... \right\}`, and `\left| ... \right|` scaled to match the exact vertical height and centered on the mathematical axis.
* **Fraction Baseline & Descender Alignment**: Precise vertical micro-spacing calibration preventing numerator descenders from overlapping the fraction bar.
* **Optical Kerning & Escaped Character Cleanup**: Natural compact spacing for variable groups like $(u)$ and $(x, y)$, and clean unescaping for `\operatorname{tri\_wave}`.
* **High-Resolution PNG / BMP Image Export**: Export beautiful mathematical formulas at 36pt / 48pt vector resolution with transparent or solid background.
* **Canvas HUD Overlay**: `RasterEngine::plot_latex_card` renders real-time floating mathematical formula HUD cards on top of interactive viewports.

#### 4. Portable Multi-Precision Arithmetic (`Formulaic::math::BigInt`, `Rational`, `GmpEvaluator`)
* Seamless integration with hardware-accelerated **GNU MP (GMP)** on x64, with built-in bundled cross-platform fallback to **mini-gmp / mini-mpq** on Win32 (x86).
* High-precision arbitrary integer and exact rational fraction evaluation.
* Eliminates asymptotic pole jump artifacts during complex implicit curve rendering (e.g. $\ln(\sin(x))$).

#### 5. Upgraded Interactive Split-Window Studio (`test_editor_window.exe`)
* **Real-Time LaTeXLive Panel**: Live standard LaTeXLive code generation synchronized with the formula editor.
* **Copy LaTeX**: One-click clipboard export ready for Overleaf, academic papers, and Markdown.
* **LaTeX -> Script**: One-click reverse compilation button allowing users to paste raw LaTeX into the editor and visualize it immediately.
* **Canvas LaTeX HUD**: Beautiful floating vector LaTeX card rendered directly on the 60 FPS viewport.

#### 6. Complete Multi-Architecture & Multi-Configuration Matrix
* 64-bit (`x64`) and 32-bit (`x86`) binaries, in both `Release` and `Debug` configurations.
* Both dynamic libraries (`Formulaic.dll` + `Formulaic.lib`) and static libraries (`Formulaic_static.lib`).
* Automated MSBuild switching via NuGet package `Formulaic 1.2.0`.

---

### Pre-built Artifacts Included in this Release:
1. `Formulaic-v1.2.0-windows-x64.zip` (64-bit Windows Release & Debug DLLs, static libs, executables, C++20 headers)
2. `Formulaic-v1.2.0-windows-x86.zip` (32-bit Windows Release & Debug DLLs, static libs, executables, C++20 headers)
3. `Formulaic-v1.2.0-windows-all.zip` (Unified multi-platform distribution package)
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
    "F:\Formulaic\dist\Formulaic-v1.2.0-windows-x64.zip",
    "F:\Formulaic\dist\Formulaic-v1.2.0-windows-x86.zip",
    "F:\Formulaic\dist\Formulaic-v1.2.0-windows-all.zip"
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
