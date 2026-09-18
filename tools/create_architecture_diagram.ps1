Add-Type -AssemblyName System.Drawing

$width = 1200
$height = 620
$bmp = New-Object System.Drawing.Bitmap($width, $height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

# Theme colors
$bg = [System.Drawing.ColorTranslator]::FromHtml("#0F141C")
$cardBg = [System.Drawing.ColorTranslator]::FromHtml("#1A2333")
$border = [System.Drawing.ColorTranslator]::FromHtml("#2E3D59")
$cyan = [System.Drawing.ColorTranslator]::FromHtml("#38BDF8")
$pink = [System.Drawing.ColorTranslator]::FromHtml("#F472B6")
$green = [System.Drawing.ColorTranslator]::FromHtml("#4ADE80")
$yellow = [System.Drawing.ColorTranslator]::FromHtml("#FBBF24")
$purple = [System.Drawing.ColorTranslator]::FromHtml("#C084FC")
$textWhite = [System.Drawing.ColorTranslator]::FromHtml("#F1F5F9")
$textDim = [System.Drawing.ColorTranslator]::FromHtml("#94A3B8")

$g.Clear($bg)

# Fonts
$fontTitle = New-Object System.Drawing.Font("Segoe UI", 16, [System.Drawing.FontStyle]::Bold)
$fontSub = New-Object System.Drawing.Font("Segoe UI", 10)
$fontCardTitle = New-Object System.Drawing.Font("Segoe UI", 12, [System.Drawing.FontStyle]::Bold)
$fontItem = New-Object System.Drawing.Font("Segoe UI", 9)
$fontMono = New-Object System.Drawing.Font("Consolas", 9)

# Title Banner
$g.DrawString("Formulaic Architecture & Rendering Pipeline", $fontTitle, (New-Object System.Drawing.SolidBrush($textWhite)), 40, 24)
$g.DrawString("End-to-End Modern C++20 High-Performance Mathematical Parsing & Visual Computing Engine", $fontSub, (New-Object System.Drawing.SolidBrush($cyan)), 40, 58)

# Function to draw a stylish stage card
function Draw-StageCard($x, $y, $w, $h, $title, $accentColor, $items) {
    $cardBrush = New-Object System.Drawing.SolidBrush($cardBg)
    $borderPen = New-Object System.Drawing.Pen($border, 1.5)
    $accentPen = New-Object System.Drawing.Pen($accentColor, 3)
    $accentBrush = New-Object System.Drawing.SolidBrush($accentColor)
    $whiteBrush = New-Object System.Drawing.SolidBrush($textWhite)
    $dimBrush = New-Object System.Drawing.SolidBrush($textDim)

    $g.FillRectangle($cardBrush, $x, $y, $w, $h)
    $g.DrawRectangle($borderPen, $x, $y, $w, $h)
    $g.DrawLine($accentPen, $x, $y, $x + $w, $y)

    $g.DrawString($title, $fontCardTitle, $accentBrush, $x + 16, $y + 16)

    $curY = $y + 50
    foreach ($item in $items) {
        $g.FillEllipse($accentBrush, $x + 18, $curY + 6, 6, 6)
        $g.DrawString($item, $fontItem, $whiteBrush, $x + 32, $curY)
        $curY += 25
    }
}

# 4 Pipeline Stage Columns
$cardW = 255
$cardH = 460
$yPos = 110

# Column 1: Math Input & Equations
Draw-StageCard 40 $yPos $cardW $cardH "1. Mathematical Inputs" $yellow @(
    "Explicit Curves: y = f(x, t)",
    "Equation Syntax: LHS = RHS",
    "Implicit Surfaces: f(x, y) = 0",
    "Parametric Curves: x(t), y(t)",
    "2D Scalar Fields: z(x, y)",
    "Scripting: let / var Variables",
    "Implicit Multiplications: 2x(y+1)",
    "55+ Built-in Math Functions",
    "Precise Syntax Diagnostics"
)

# Column 2: Parser & Math Core
Draw-StageCard 325 $yPos $cardW $cardH "2. Parser & Math Engines" $cyan @(
    "Lexer & Pratt Recursive Descent",
    "AST Syntax Validation",
    "Bytecode Compiler & Stack VM",
    ">15,000,000 evals/sec (Zero Alloc)",
    "Numerical Calculus Engine:",
    "  • diff_step / Second Deriv",
    "  • Gradient & Curvature kappa",
    "  • Simpson & Adaptive Quad",
    "Fast Fourier Transform (FFT):",
    "  • 1D & 2D Radix-2 Cooley-Tukey",
    "  • Hann / Blackman / Flat-Top",
    "  • Spectral Magnitude Peak"
)

# Column 3: Raster Engine & Shading
Draw-StageCard 610 $yPos $cardW $cardH "3. Raster & AA Engine" $pink @(
    "Marching Squares Contouring:",
    "  • Subpixel Linear Interpolation",
    "  • Zero-Level Set Extraction",
    "Xiaolin Wu Subpixel Anti-Aliasing",
    "Singularity & Asymptote Culling",
    "2D Heatmaps & Colormaps:",
    "  • Viridis, Plasma, Jet, Coolwarm",
    "Continuous FrameBuffer Surface",
    "Alpha Blending & Compositing",
    "Vectorized Coordinate Viewport"
)

# Column 4: Presentation & Windowing
Draw-StageCard 895 $yPos $cardW $cardH "4. Presentation & HWND" $green @(
    "Win32 HWND Native Binding",
    "Zero-Flicker Double Buffering:",
    "  • Top-Down 32bpp GDI DIB",
    "  • StretchDIBits Blit (<0.6 ms)",
    "Decoupled 60 FPS Animation Timer",
    "Interactive Gesture Support:",
    "  • Left-Drag Pan & Zoom",
    "  • Dynamic Curve Hover Highlight",
    "  • Anti-Aliased Inspection HUD",
    "Pipeline Hooks (Pre/Post/Shader)",
    "Image Exporters (BMP/PPM/Raw)",
    "FrameStream 60fps Video Export"
)

# Connecting Arrows
$arrowPen = New-Object System.Drawing.Pen($cyan, 2)
$arrowBrush = New-Object System.Drawing.SolidBrush($cyan)
$arrowY = $yPos + 220

function Draw-FlowArrow($x1, $x2, $y) {
    $g.DrawLine($arrowPen, [float]$x1, [float]$y, [float]$x2, [float]$y)
    $p1 = New-Object System.Drawing.PointF([float]$x2, [float]$y)
    $p2 = New-Object System.Drawing.PointF([float]($x2 - 8), [float]($y - 5))
    $p3 = New-Object System.Drawing.PointF([float]($x2 - 8), [float]($y + 5))
    $points = [System.Drawing.PointF[]]@($p1, $p2, $p3)
    $g.FillPolygon($arrowBrush, $points)
}

Draw-FlowArrow 297 323 $arrowY
Draw-FlowArrow 582 608 $arrowY
Draw-FlowArrow 867 893 $arrowY

$outImg = "F:\Formulaic\assets\pipeline_architecture.png"
$bmp.Save($outImg, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
$g.Dispose()

$sz = (Get-Item $outImg).Length
Write-Output "Generated architecture pipeline diagram: $outImg ($sz bytes)"
