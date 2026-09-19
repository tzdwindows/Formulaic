$ErrorActionPreference = "Stop"

$version = "v1.2.0"
$distBase = "F:\Formulaic\dist"
if (-not (Test-Path $distBase)) {
    New-Item -ItemType Directory -Force -Path $distBase | Out-Null
}

function Build-Platform-Archive([string]$arch, [string]$buildRoot) {
    $dirName = "Formulaic-$version-windows-$arch"
    $stageDir = Join-Path $distBase $dirName
    if (Test-Path $stageDir) {
        Remove-Item -Recurse -Force $stageDir
    }

    $configs = @("Release", "Debug")
    foreach ($cfg in $configs) {
        New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "bin\$cfg") | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "lib\$cfg") | Out-Null

        # Copy DLL and executables
        Copy-Item (Join-Path $buildRoot "$cfg\Formulaic.dll") (Join-Path $stageDir "bin\$cfg")
        Copy-Item (Join-Path $buildRoot "examples\$cfg\example_interactive_window.exe") (Join-Path $stageDir "bin\$cfg")
        Copy-Item (Join-Path $buildRoot "examples\$cfg\example_static.exe") (Join-Path $stageDir "bin\$cfg")
        Copy-Item (Join-Path $buildRoot "test\$cfg\test_editor_window.exe") (Join-Path $stageDir "bin\$cfg")
        Copy-Item (Join-Path $buildRoot "test\$cfg\test_custom_render.exe") (Join-Path $stageDir "bin\$cfg")

        # Copy libs
        Copy-Item (Join-Path $buildRoot "$cfg\Formulaic.lib") (Join-Path $stageDir "lib\$cfg")
        Copy-Item (Join-Path $buildRoot "$cfg\Formulaic_static.lib") (Join-Path $stageDir "lib\$cfg")
    }

    # Copy headers and docs
    New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "include") | Out-Null
    Copy-Item -Recurse "F:\Formulaic\include\Formulaic" (Join-Path $stageDir "include")
    Copy-Item "F:\Formulaic\README.md" $stageDir
    Copy-Item "F:\Formulaic\LICENSE" $stageDir

    $zipPath = Join-Path $distBase "$dirName.zip"
    if (Test-Path $zipPath) {
        Remove-Item -Force $zipPath
    }
    Compress-Archive -Path "$stageDir\*" -DestinationPath $zipPath
    $sz = (Get-Item $zipPath).Length
    Write-Output "Created: $zipPath ($sz bytes)"
    return $zipPath
}

# 1. Build x64 archive
Build-Platform-Archive "x64" "F:\Formulaic\build"

# 2. Build x86 archive
Build-Platform-Archive "x86" "F:\Formulaic\build_x86"

# 3. Build unified windows-all archive
$allDir = Join-Path $distBase "Formulaic-$version-windows-all"
if (Test-Path $allDir) {
    Remove-Item -Recurse -Force $allDir
}
New-Item -ItemType Directory -Force -Path $allDir | Out-Null
Copy-Item -Recurse (Join-Path $distBase "Formulaic-$version-windows-x64") (Join-Path $allDir "x64")
Copy-Item -Recurse (Join-Path $distBase "Formulaic-$version-windows-x86") (Join-Path $allDir "x86")
Copy-Item -Recurse "F:\Formulaic\include\Formulaic" (Join-Path $allDir "include")
Copy-Item "F:\Formulaic\README.md" $allDir
Copy-Item "F:\Formulaic\LICENSE" $allDir

$allZip = Join-Path $distBase "Formulaic-$version-windows-all.zip"
if (Test-Path $allZip) {
    Remove-Item -Force $allZip
}
Compress-Archive -Path "$allDir\*" -DestinationPath $allZip
$szAll = (Get-Item $allZip).Length
Write-Output "Created: $allZip ($szAll bytes)"

Write-Output "Packaging complete for all targets!"
