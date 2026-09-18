Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force -Path 'F:\Formulaic\assets' | Out-Null

$map = @{
    'test_implicit_circle.bmp' = 'demo_implicit_circle.png'
    'test_explicit_sin_x.bmp' = 'demo_explicit_curve.png'
    'test_parametric_lissajous.bmp' = 'demo_parametric_curve.png'
    'test_scalar_field.bmp' = 'demo_scalar_field.png'
    'test_hover_indicator.bmp' = 'demo_hover_inspection.png'
    'custom_render_variables.bmp' = 'demo_variables_script.png'
    'custom_render_calculus.bmp' = 'demo_calculus_derivative.png'
    'custom_render_fft_wave.bmp' = 'demo_fft_windowing.png'
    'custom_render_circle_equation.bmp' = 'demo_circle_equation.png'
    'custom_cli_render.bmp' = 'demo_custom_cli_render.png'
}

foreach ($k in $map.Keys) {
    $src = Join-Path 'F:\Formulaic\test_output' $k
    if (Test-Path $src) {
        $dest = Join-Path 'F:\Formulaic\assets' $map[$k]
        $bmp = [System.Drawing.Bitmap]::new($src)
        $bmp.Save($dest, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp.Dispose()
        $sz = (Get-Item $dest).Length
        Write-Output "Converted $k -> $($map[$k]) ($sz bytes)"
    } else {
        Write-Output "Missing $k"
    }
}
