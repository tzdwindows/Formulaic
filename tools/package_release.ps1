$ErrorActionPreference = "Stop"

$dist = "F:\Formulaic\dist\Formulaic-v1.1.0-windows-x64"
if (Test-Path $dist) {
    Remove-Item -Recurse -Force $dist
}

New-Item -ItemType Directory -Force -Path (Join-Path $dist "bin") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $dist "lib") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $dist "include") | Out-Null

Copy-Item "F:\Formulaic\build\Release\Formulaic.dll" (Join-Path $dist "bin")
Copy-Item "F:\Formulaic\build\examples\Release\example_interactive_window.exe" (Join-Path $dist "bin")
Copy-Item "F:\Formulaic\build\examples\Release\example_static.exe" (Join-Path $dist "bin")
Copy-Item "F:\Formulaic\build\test\Release\test_editor_window.exe" (Join-Path $dist "bin")
Copy-Item "F:\Formulaic\build\test\Release\test_custom_render.exe" (Join-Path $dist "bin")

Copy-Item "F:\Formulaic\build\Release\Formulaic.lib" (Join-Path $dist "lib")
Copy-Item "F:\Formulaic\build\Release\Formulaic_static.lib" (Join-Path $dist "lib")

Copy-Item -Recurse "F:\Formulaic\include\Formulaic" (Join-Path $dist "include")
Copy-Item "F:\Formulaic\README.md" $dist
Copy-Item "F:\Formulaic\LICENSE" $dist

$zipPath = "F:\Formulaic\dist\Formulaic-v1.1.0-windows-x64.zip"
if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}

Compress-Archive -Path "$dist\*" -DestinationPath $zipPath
$sz = (Get-Item $zipPath).Length
Write-Output "Created release archive: $zipPath ($sz bytes)"
