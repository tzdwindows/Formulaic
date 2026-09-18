Add-Type -AssemblyName System.Drawing

$width = 1240
$height = 720
$bmp = New-Object System.Drawing.Bitmap($width, $height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

# Colors (Modern Catppuccin Mocha theme)
$bgDark      = [System.Drawing.ColorTranslator]::FromHtml("#181825")
$panelLeftBg = [System.Drawing.ColorTranslator]::FromHtml("#1E1E2E")
$borderCol   = [System.Drawing.ColorTranslator]::FromHtml("#313244")
$textCol     = [System.Drawing.ColorTranslator]::FromHtml("#CDD6F4")
$accentCyan  = [System.Drawing.ColorTranslator]::FromHtml("#89DCEB")
$accentPink  = [System.Drawing.ColorTranslator]::FromHtml("#F5C2E7")
$accentGreen = [System.Drawing.ColorTranslator]::FromHtml("#A6E3A1")
$accentYellow= [System.Drawing.ColorTranslator]::FromHtml("#F9E2AF")
$accentBlue  = [System.Drawing.ColorTranslator]::FromHtml("#89B4FA")
$accentMauve = [System.Drawing.ColorTranslator]::FromHtml("#CBA6F7")
$btnBg       = [System.Drawing.ColorTranslator]::FromHtml("#313244")
$editorBg    = [System.Drawing.ColorTranslator]::FromHtml("#11111B")
$acBg        = [System.Drawing.ColorTranslator]::FromHtml("#1E1E2E")
$acSelBg     = [System.Drawing.ColorTranslator]::FromHtml("#45475A")

# Window Background & Title Bar
$g.Clear($bgDark)

# Window Titlebar
$brushTitleBar = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#11111B"))
$g.FillRectangle($brushTitleBar, 0, 0, $width, 36)
$fontWinTitle = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Bold)
$g.DrawString("Formulaic - Live Expression Editor & Real-Time Math Visualizer (Win32 HWND)", $fontWinTitle, [System.Drawing.Brushes]::LightGray, 16, 8)

# Window Control Buttons (Min, Max, Close)
$g.FillEllipse((New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#F38BA8"))), $width - 28, 12, 12, 12)
$g.FillEllipse((New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#F9E2AF"))), $width - 48, 12, 12, 12)
$g.FillEllipse((New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#A6E3A1"))), $width - 68, 12, 12, 12)

# Left Panel (Editor)
$leftWidth = 460
$brushLeft = New-Object System.Drawing.SolidBrush($panelLeftBg)
$g.FillRectangle($brushLeft, 0, 36, $leftWidth, $height - 36)

# Left Panel Header
$fontHeader = New-Object System.Drawing.Font("Segoe UI", 12, [System.Drawing.FontStyle]::Bold)
$fontSub    = New-Object System.Drawing.Font("Segoe UI", 9)
$fontCode   = New-Object System.Drawing.Font("Consolas", 10)
$fontCodeSm = New-Object System.Drawing.Font("Consolas", 9)
$brushCyan  = New-Object System.Drawing.SolidBrush($accentCyan)
$brushText  = New-Object System.Drawing.SolidBrush($textCol)
$brushYellow= New-Object System.Drawing.SolidBrush($accentYellow)
$brushGreen = New-Object System.Drawing.SolidBrush($accentGreen)
$brushPink  = New-Object System.Drawing.SolidBrush($accentPink)
$brushBlue  = New-Object System.Drawing.SolidBrush($accentBlue)
$brushMauve = New-Object System.Drawing.SolidBrush($accentMauve)

$g.DrawString("Expression Script Editor", $fontHeader, $brushCyan, 18, 48)
$g.DrawString("Syntax Highlighting & Intelligent Autocomplete Active", $fontSub, $brushMauve, 18, 72)

# Mode Selector Label & Box
$penBorder = New-Object System.Drawing.Pen($borderCol, 1)
$brushBtn  = New-Object System.Drawing.SolidBrush($btnBg)
$g.DrawString("Render Mode:", $fontSub, $brushText, 18, 98)
$g.FillRectangle($brushBtn, 110, 94, 332, 26)
$g.DrawRectangle($penBorder, 110, 94, 332, 26)
$g.DrawString("Auto / Implicit2D (Contour / Marching Squares)", $fontSub, $brushGreen, 118, 98)

# Editor Text Area (RichEdit Simulated with Syntax Colors)
$g.FillRectangle((New-Object System.Drawing.SolidBrush($editorBg)), 18, 128, 424, 210)
$g.DrawRectangle($penBorder, 18, 128, 424, 210)

$editorTokens = @(
    @{ text = "let"; color = $brushMauve; x = 28; y = 140 },
    @{ text = " r = "; color = $brushText; x = 52; y = 140 },
    @{ text = "hypot"; color = $brushCyan; x = 90; y = 140 },
    @{ text = "(x, y);"; color = $brushText; x = 132; y = 140 },

    @{ text = "let"; color = $brushMauve; x = 28; y = 162 },
    @{ text = " theta = "; color = $brushText; x = 52; y = 162 },
    @{ text = "atan2"; color = $brushCyan; x = 110; y = 162 },
    @{ text = "(y, x);"; color = $brushText; x = 152; y = 162 },

    @{ text = "let"; color = $brushMauve; x = 28; y = 184 },
    @{ text = " envelope = "; color = $brushText; x = 52; y = 184 },
    @{ text = "exp"; color = $brushCyan; x = 142; y = 184 },
    @{ text = "(-"; color = $brushText; x = 166; y = 184 },
    @{ text = "0.35"; color = $brushYellow; x = 180; y = 184 },
    @{ text = " * r);"; color = $brushText; x = 212; y = 184 },

    @{ text = "r - "; color = $brushText; x = 28; y = 206 },
    @{ text = "3.0"; color = $brushYellow; x = 58; y = 206 },
    @{ text = " - "; color = $brushText; x = 82; y = 206 },
    @{ text = "0.4"; color = $brushYellow; x = 104; y = 206 },
    @{ text = " * "; color = $brushText; x = 128; y = 206 },
    @{ text = "sin"; color = $brushCyan; x = 150; y = 206 },
    @{ text = "("; color = $brushText; x = 174; y = 206 },
    @{ text = "7.0"; color = $brushYellow; x = 182; y = 206 },
    @{ text = " * theta + "; color = $brushText; x = 206; y = 206 },
    @{ text = "2.0"; color = $brushYellow; x = 288; y = 206 },
    @{ text = " * t) = "; color = $brushText; x = 312; y = 206 },
    @{ text = "0"; color = $brushYellow; x = 368; y = 206 },

    @{ text = "// Autocomplete popup demonstration: typing 'di'"; color = [System.Drawing.Brushes]::Gray; x = 28; y = 236 },
    @{ text = "di"; color = $brushCyan; x = 28; y = 260 }
)

foreach ($tok in $editorTokens) {
    $g.DrawString($tok.text, $fontCode, $tok.color, $tok.x, $tok.y)
}

# Autocomplete Floating Popup (ListBox simulation)
$acX = 46
$acY = 280
$acW = 270
$acH = 110
$g.FillRectangle((New-Object System.Drawing.SolidBrush($acBg)), $acX, $acY, $acW, $acH)
$g.DrawRectangle((New-Object System.Drawing.Pen($accentMauve, 1.5)), $acX, $acY, $acW, $acH)

# Selected item background
$g.FillRectangle((New-Object System.Drawing.SolidBrush($acSelBg)), $acX + 2, $acY + 4, $acW - 4, 22)
$g.DrawString("diff_step(fp, fm, h)  [calc]", $fontCodeSm, $brushCyan, $acX + 8, $acY + 8)
$g.DrawString("diff2_step(fp, f0, fm) [calc]", $fontCodeSm, $brushText, $acX + 8, $acY + 30)
$g.DrawString("diff_forward(fx_h, fx) [calc]", $fontCodeSm, $brushText, $acX + 8, $acY + 52)
$g.DrawString("diff_backward(fx, fx)  [calc]", $fontCodeSm, $brushText, $acX + 8, $acY + 74)

# Quick Preset Buttons
$g.DrawString("Presets & Script Templates:", $fontSub, $brushText, 18, 410)
$presets = @("Preset 1: let/var Script", "Preset 2: Calculus diff", "Preset 3: FFT Windowing", "Preset 4: Implicit 2D")
for ($i = 0; $i -lt 4; $i++) {
    $bx = 18 + ($i % 2) * 216
    $by = 432 + [int]($i / 2) * 36
    $g.FillRectangle($brushBtn, $bx, $by, 208, 30)
    $g.DrawRectangle($penBorder, $bx, $by, 208, 30)
    $g.DrawString($presets[$i], $fontSub, $brushCyan, $bx + 12, $by + 6)
}

# Action Buttons
$g.FillRectangle((New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml("#89B4FA"))), 18, 514, 424, 34)
$fontBtnBold = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Bold)
$g.DrawString("Render & Update Viewport  [Enter / Auto-compile]", $fontBtnBold, [System.Drawing.Brushes]::Black, 68, 520)

# Status & Diagnostics Box
$g.FillRectangle((New-Object System.Drawing.SolidBrush($editorBg)), 18, 558, 424, 142)
$g.DrawRectangle($penBorder, 18, 558, 424, 142)
$g.DrawString("Diagnostics & Live Telemetry", $fontHeader, $brushGreen, 28, 568)
$g.DrawString("STATUS: Expression Valid | Highlighting & Autocomplete Ready", $fontSub, $brushGreen, 28, 596)
$g.DrawString("Referenced Variables: x y t | Animation Active (60 FPS)", $fontSub, $brushText, 28, 620)
$g.DrawString("Viewport: X[-4.0, 4.0] Y[-4.0, 4.0] | Marching Squares Grid: 160x120", $fontSub, [System.Drawing.Brushes]::Gray, 28, 644)
$g.DrawString("Active HUD: Mouse hover curve detection with real-time value tooltip", $fontSub, $brushYellow, 28, 668)

# Right Panel Divider
$g.DrawLine((New-Object System.Drawing.Pen($borderCol, 2)), $leftWidth, 36, $leftWidth, $height)

# Right Panel (Render Canvas)
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
Write-Output "Generated modern editor illustration: $outImg ($sz bytes)"
