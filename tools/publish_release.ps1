$token = $env:GITHUB_TOKEN
if (-not $token) {
    $credOutput = "protocol=https`nhost=github.com" | git credential fill
    if ($credOutput -match "password=(.*)") {
        $token = $matches[1].Trim()
    }
}
if (-not $token) {
    throw "GitHub token not found. Please set GITHUB_TOKEN environment variable."
}
$repo = "tzdwindows/Formulaic"
$tag = "v1.1.0"
$releaseName = "v1.1.0: Mathematical Equation Parsing, Split-Window Visual Editor, Numerical Calculus & FFT Engine"

$releaseBody = @"
# Formulaic v1.1.0 Release Notes

We are thrilled to announce **Formulaic v1.1.0**, bringing a major leap forward in mathematical capability, interactive visual toolsets, equation parsing, and developer experience.

---

### Highlights & New Features in v1.1.0

#### 1. Mathematical Equation Parsing (`LHS = RHS`)
* Full first-class support for mathematical equations containing single `=` or `==` (e.g., `x^2 + y^2 = 4`, `x^2 - y^2 = 1`, `y = sin(x)`).
* `Expression::parse_equation(...)` transforms equations into zero-set level surfaces $F(x, y) = (\text{LHS}) - (\text{RHS}) = 0$.
* Seamlessly connects to Marching Squares contour extraction.
* Informative diagnostics for degenerate equations like $x^2 + y^2 = 0$.

#### 2. Split-Screen Interactive Mathematical Studio (`test_editor_window.exe`)
* Built-in desktop studio featuring dual-panel architecture:
  * **Left Panel**: Multi-line script editor, preset buttons, live syntax validation with error line/column pointers, and automatic mode switching.
  * **Right Panel**: Real-time Win32 HWND viewport running at 60 FPS with Xiaolin Wu subpixel anti-aliasing.
* Mouse interaction: left-click drag to pan, wheel to zoom centered on cursor, and curve hover inspection.

#### 3. Numerical Calculus Engine (`Formulaic::math::Calculus`)
* Derivatives: forward, backward, and 5-point central differences; arbitrary higher-order derivatives.
* Vector calculus: Gradient $\nabla f$, 2D Laplacian $\nabla^2 f$, 2D Curvature $\kappa(x)$.
* Numerical integration: Composite Trapezoidal and Simpson's rules, adaptive Gaussian quadrature.
* Root finding: Newton-Raphson, Secant, and Bisection solvers.

#### 4. Fast Fourier Transform Engine (`Formulaic::math::FFT`)
* 1D & 2D Radix-2 Cooley-Tukey FFT / IFFT with high numeric precision.
* Spectral windowing: Hann, Hamming, Blackman, Flat-Top, and Welch windows.
* Synthetic waveform generation: square waves, sawtooth waves, triangle waves, and chirps.
* Automatic spectral magnitude peak detection.

#### 5. Variable Declarations (`let` / `var`) & Multi-Statement Scripts
* Define reusable intermediate expressions to eliminate redundant calculations.
* Full support for nested mathematical blocks.

#### 6. Extended Implicit Multiplications
* Support for variable-to-parenthesis and constant-to-parenthesis multiplications (e.g. `x(y + 1)`, `pi(x + 1)`, `2x(y + 3)`).

#### 7. Decoupled 60 FPS Win32 Message Pump & Anti-Aliasing
* Decoupled mouse input processing from animation rendering timer in `example_interactive_window.exe` to guarantee zero-stutter continuous animations.
* Subpixel anti-aliased line rendering and dynamic curve thickness hover highlighting.

---

### Pre-built Artifacts Included:
* `Formulaic-v1.1.0-windows-x64.zip`:
  * Pre-compiled 64-bit binaries: `Formulaic.dll`, `example_interactive_window.exe`, `example_static.exe`, `test_editor_window.exe`, `test_custom_render.exe`
  * Development libraries: `Formulaic.lib` (import stub), `Formulaic_static.lib` (static library)
  * C++20 Header files: `include/Formulaic/...`
  * Architecture guide and documentation
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
$response = Invoke-RestMethod -Uri $url -Method Post -Headers $headers -Body $payload -ContentType "application/json; charset=utf-8"

$releaseId = $response.id
$uploadUrlTemplate = $response.upload_url
Write-Output "Created release successfully with ID: $releaseId"
Write-Output "Upload URL template: $uploadUrlTemplate"

# 2. Upload asset zip file
$zipFile = "F:\Formulaic\dist\Formulaic-v1.1.0-windows-x64.zip"
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

Write-Output "Release process complete!"
