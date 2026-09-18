Add-Type -AssemblyName System.Drawing

$width = 1200
$height = 700
$bmp = New-Object System.Drawing.Bitmap($width, $height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

# Colors
$bgDark = [System.Drawing.ColorTranslator]::FromHtml("#181825")
$panelLeftBg = [System.Drawing.ColorTranslator]::FromHtml("#1E1E2E")
$borderCol = [System.Drawing.ColorTranslator]::FromHtml("#313244")
$textCol = [System.Drawing.ColorTranslator]::FromHtml("#CDD6F4")
$accentCyan = [System.Drawing.ColorTranslator]::FromHtml("#89DCEB")
$accentPink = [System.Drawing.ColorTranslator]::FromHtml("#F5C2E7")
$accentGreen = [System.Drawing.ColorTranslator]::FromHtml("#A6E3A1")
$accentYellow = [System.Drawing.ColorTranslator]::FromHtml("#F9E2AF")
$accentBlue = [System.Drawing.ColorTranslator]::FromHtml("#89B4FA")
$btnBg = [System.Drawing.ColorTranslator]::FromHtml("#313244")
$editorBg = [System.Drawing.ColorTranslator]::FromHtml("#11111B")

# Window Background & Title Bar
$g.Clear($bgDark)

# Window Titlebar
$brushTitleBar = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#11111B"))
$g.FillRectangle($brushTitleBar, 0, 0, $width, 36)
$fontWinTitle = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Bold)
$g.DrawString("Formulaic - Split-Window Math Studio & Real-Time Engine (Win32 HWND)", $fontWinTitle, [System.Drawing.Brushes]::LightGray, 16, 8)

# Left Panel (Editor)
$leftWidth = 440
$brushLeft = New-Object System.Drawing.SolidBrush($panelLeftBg)
$g.FillRectangle($brushLeft, 0, 36, $leftWidth, $height - 36)

# Left Panel Header
$fontHeader = New-Object System.Drawing.Font("Segoe UI", 12, [System.Drawing.FontStyle]::Bold)
$fontSub = New-Object System.Drawing.Font("Segoe UI", 9)
$fontCode = New-Object System.Drawing.Font("Consolas", 10)
$brushCyan = New-Object System.Drawing.SolidBrush($accentCyan)
$brushText = New-Object System.Drawing.SolidBrush($textCol)
$brushYellow = New-Object System.Drawing.SolidBrush($accentYellow)
$brushGreen = New-Object System.Drawing.SolidBrush($accentGreen)
$brushPink = New-Object System.Drawing.SolidBrush($accentPink)
$brushBlue = New-Object System.Drawing.SolidBrush($accentBlue)

$g.DrawString("Expression & Script Editor", $fontHeader, $brushCyan, 18, 50)
$g.DrawString("Supports let/var, calculus, FFT, and LHS = RHS equations", $fontSub, [System.Drawing.Brushes]::Gray, 18, 76)

# Mode Selector Label & Box
$g.DrawString("Render Mode:", $fontSub, $brushText, 18, 105)
$penBorder = New-Object System.Drawing.Pen($borderCol, 1)
$brushBtn = New-Object System.Drawing.SolidBrush($btnBg)
$g.FillRectangle($brushBtn, 110, 100, 310, 28)
$g.DrawRectangle($penBorder, 110, 100, 310, 28)
$g.DrawString("Implicit2D: Equation Zero-Set (LHS = RHS)", $fontSub, $brushPink, 120, 105)

# Editor Text Area
$g.FillRectangle((New-Object System.Drawing.SolidBrush($editorBg)), 18, 140, 404, 210)
$g.DrawRectangle($penBorder, 18, 140, 404, 210)

$codeLines = @(
    "// 1. Declare intermediate variables",
    "let r = hypot(x, y);",
    "let theta = atan2(y, x);",
    "",
    "// 2. Implicit equation (Marching Squares)",
    "x^2 + y^2 = 4",
    "",
    "// Evaluates as f(x, y) = (x^2 + y^2) - 4 = 0"
)

$yOff = 150
foreach ($line in $codeLines) {
    $br = $brushText
    if ($line.StartsWith("//")) { $br = [System.Drawing.Brushes]::Gray }
    elseif ($line.StartsWith("let")) { $br = $brushYellow }
    elseif ($line.Contains("=")) { $br = $brushPink }
    $g.DrawString($line, $fontCode, $br, 28, $yOff)
    $yOff += 22
}

# Quick Preset Buttons
$g.DrawString("Presets & Quick Demonstrations:", $fontSub, $brushText, 18, 365)
$presets = @("let/var Script", "Calculus diff", "FFT Windowing", "x^2 + y^2 = 4")
$px = 18
$py = 390
for ($i = 0; $i -lt 4; $i++) {
    $bx = 18 + ($i % 2) * 205
    $by = 390 + [int]($i / 2) * 36
    $g.FillRectangle($brushBtn, $bx, $by, 195, 30)
    $g.DrawRectangle($penBorder, $bx, $by, 195, 30)
    $g.DrawString($presets[$i], $fontSub, $brushCyan, $bx + 12, $by + 6)
}

# Action Buttons
$g.FillRectangle((New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#A6E3A1"))), 18, 475, 404, 36)
$fontBtnBold = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Bold)
$g.DrawString("Render & Update Viewport [Ctrl+Enter]", $fontBtnBold, [System.Drawing.Brushes]::Black, 70, 482)

# Status & Diagnostics Box
$g.FillRectangle((New-Object System.Drawing.SolidBrush($editorBg)), 18, 525, 404, 150)
$g.DrawRectangle($penBorder, 18, 525, 404, 150)
$g.DrawString("Diagnostics & Live Telemetry", $fontHeader, $brushGreen, 28, 535)
$g.DrawString("STATUS: Ready (0 errors, 0 warnings)", $fontSub, $brushGreen, 28, 565)
$g.DrawString("Parsed AST: Sub(Add(Pow(x,2), Pow(y,2)), 4)", $fontSub, $brushText, 28, 590)
$g.DrawString("Viewport: X[-3.0, 3.0] Y[-3.0, 3.0] | Pan & Zoom Active", $fontSub, [System.Drawing.Brushes]::Gray, 28, 615)
$g.DrawString("Crosshair: (x = 1.414, y = 1.414) | dist = 2.000", $fontSub, $brushYellow, 28, 640)

# Right Panel Divider
$g.DrawLine((New-Object System.Drawing.Pen($borderCol, 2)), $leftWidth, 36, $leftWidth, $height)

# Right Panel (Render Canvas)
# Load demo_circle_equation.png or render right viewport
$rightBmpPath = "F:\Formulaic\assets\demo_circle_equation.png"
if (Test-Path $rightBmpPath) {
    $srcCanvas = [System.Drawing.Bitmap]::new($rightBmpPath)
    $rx = [int]($leftWidth + 2)
    $ry = 36
    $rw = [int]($width - $leftWidth - 2)
    $rh = [int]($height - 36)
    $g.DrawImage($srcCanvas, $rx, $ry, $rw, $rh)
    $srcCanvas.Dispose()
}

$outImg = "F:\Formulaic\assets\demo_editor_window.png"
$bmp.Save($outImg, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
$g.Dispose()

$sz = (Get-Item $outImg).Length
Write-Output "Generated editor window diagram: $outImg ($sz bytes)"
