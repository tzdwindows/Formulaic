$ErrorActionPreference = "Stop"

$nugetDir = "F:\Formulaic\nuget"
if (Test-Path $nugetDir) {
    Remove-Item -Recurse -Force $nugetDir
}

$buildNativeDir = Join-Path $nugetDir "build\native"
$buildDir = Join-Path $nugetDir "build"
$toolsDir = Join-Path $nugetDir "tools"

# Create directories
$platforms = @("x64", "x86")
$configs = @("Release", "Debug")

foreach ($p in $platforms) {
    foreach ($c in $configs) {
        New-Item -ItemType Directory -Force -Path (Join-Path $buildNativeDir "lib\$p\$c") | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $buildNativeDir "bin\$p\$c") | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $toolsDir "$p\$c") | Out-Null
    }
}
New-Item -ItemType Directory -Force -Path (Join-Path $buildNativeDir "include") | Out-Null

# 1. Copy headers
Copy-Item -Recurse "F:\Formulaic\include\Formulaic" (Join-Path $buildNativeDir "include")

# 2. Copy libs and dlls
# x64 Release
Copy-Item "F:\Formulaic\build\Release\Formulaic.lib" (Join-Path $buildNativeDir "lib\x64\Release")
Copy-Item "F:\Formulaic\build\Release\Formulaic_static.lib" (Join-Path $buildNativeDir "lib\x64\Release")
Copy-Item "F:\Formulaic\build\Release\Formulaic.dll" (Join-Path $buildNativeDir "bin\x64\Release")
# x64 Debug
Copy-Item "F:\Formulaic\build\Debug\Formulaic.lib" (Join-Path $buildNativeDir "lib\x64\Debug")
Copy-Item "F:\Formulaic\build\Debug\Formulaic_static.lib" (Join-Path $buildNativeDir "lib\x64\Debug")
Copy-Item "F:\Formulaic\build\Debug\Formulaic.dll" (Join-Path $buildNativeDir "bin\x64\Debug")

# x86 Release
Copy-Item "F:\Formulaic\build_x86\Release\Formulaic.lib" (Join-Path $buildNativeDir "lib\x86\Release")
Copy-Item "F:\Formulaic\build_x86\Release\Formulaic_static.lib" (Join-Path $buildNativeDir "lib\x86\Release")
Copy-Item "F:\Formulaic\build_x86\Release\Formulaic.dll" (Join-Path $buildNativeDir "bin\x86\Release")
# x86 Debug
Copy-Item "F:\Formulaic\build_x86\Debug\Formulaic.lib" (Join-Path $buildNativeDir "lib\x86\Debug")
Copy-Item "F:\Formulaic\build_x86\Debug\Formulaic_static.lib" (Join-Path $buildNativeDir "lib\x86\Debug")
Copy-Item "F:\Formulaic\build_x86\Debug\Formulaic.dll" (Join-Path $buildNativeDir "bin\x86\Debug")

# 3. Copy tools
Copy-Item "F:\Formulaic\build\test\Release\test_editor_window.exe" (Join-Path $toolsDir "x64\Release")
Copy-Item "F:\Formulaic\build\examples\Release\example_interactive_window.exe" (Join-Path $toolsDir "x64\Release")
Copy-Item "F:\Formulaic\build\test\Debug\test_editor_window.exe" (Join-Path $toolsDir "x64\Debug")
Copy-Item "F:\Formulaic\build\examples\Debug\example_interactive_window.exe" (Join-Path $toolsDir "x64\Debug")

Copy-Item "F:\Formulaic\build_x86\test\Release\test_editor_window.exe" (Join-Path $toolsDir "x86\Release")
Copy-Item "F:\Formulaic\build_x86\examples\Release\example_interactive_window.exe" (Join-Path $toolsDir "x86\Release")
Copy-Item "F:\Formulaic\build_x86\test\Debug\test_editor_window.exe" (Join-Path $toolsDir "x86\Debug")
Copy-Item "F:\Formulaic\build_x86\examples\Debug\example_interactive_window.exe" (Join-Path $toolsDir "x86\Debug")

# Default root tools (x64 Release)
Copy-Item "F:\Formulaic\build\test\Release\test_editor_window.exe" $toolsDir
Copy-Item "F:\Formulaic\build\examples\Release\example_interactive_window.exe" $toolsDir

# 4. Generate Formulaic.targets for MSBuild in Visual Studio
$nativeTargetsContent = @"
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <FormulaicPlatform Condition="'`$(Platform)' == 'Win32' Or '`$(Platform)' == 'x86'">x86</FormulaicPlatform>
    <FormulaicPlatform Condition="'`$(Platform)' == 'x64'">x64</FormulaicPlatform>
    <FormulaicConfig Condition="'`$(Configuration)' == 'Debug'">Debug</FormulaicConfig>
    <FormulaicConfig Condition="'`$(Configuration)' != 'Debug'">Release</FormulaicConfig>
  </PropertyGroup>

  <ItemDefinitionGroup Condition="'`$(FormulaicPlatform)' != ''">
    <ClCompile>
      <AdditionalIncludeDirectories>`$(MSBuildThisFileDirectory)include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <LanguageStandard Condition="'`$(LanguageStandard)' == '' Or '`$(LanguageStandard)' &lt; 'stdcpp20'">stdcpp20</LanguageStandard>
    </ClCompile>
    <Link Condition="'`$(FormulaicUseStatic)' != 'true'">
      <AdditionalDependencies>`$(MSBuildThisFileDirectory)lib\`$(FormulaicPlatform)\`$(FormulaicConfig)\Formulaic.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
    <Link Condition="'`$(FormulaicUseStatic)' == 'true'">
      <AdditionalDependencies>`$(MSBuildThisFileDirectory)lib\`$(FormulaicPlatform)\`$(FormulaicConfig)\Formulaic_static.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>

  <ItemGroup Condition="'`$(FormulaicPlatform)' != '' And '`$(FormulaicUseStatic)' != 'true'">
    <ReferenceCopyLocalPaths Include="`$(MSBuildThisFileDirectory)bin\`$(FormulaicPlatform)\`$(FormulaicConfig)\Formulaic.dll" />
  </ItemGroup>
</Project>
"@
[System.IO.File]::WriteAllText((Join-Path $buildNativeDir "Formulaic.targets"), $nativeTargetsContent, [System.Text.Encoding]::UTF8)

# Root build targets forwarding to native targets
$rootTargetsContent = @"
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <Import Project="`$(MSBuildThisFileDirectory)native\Formulaic.targets" Condition="Exists('`$(MSBuildThisFileDirectory)native\Formulaic.targets')" />
</Project>
"@
[System.IO.File]::WriteAllText((Join-Path $buildDir "Formulaic.targets"), $rootTargetsContent, [System.Text.Encoding]::UTF8)

# 5. Output directory
$distDir = "F:\Formulaic\dist"
if (-not (Test-Path $distDir)) {
    New-Item -ItemType Directory -Force -Path $distDir | Out-Null
}

# 6. Execute nuget pack
$nugetExe = "F:\Formulaic\tools\nuget.exe"
& $nugetExe pack "F:\Formulaic\Formulaic.nuspec" -OutputDirectory $distDir -BasePath "F:\Formulaic"

$nupkg = Get-ChildItem -Path $distDir -Filter "Formulaic.*.nupkg" | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName
if ($nupkg -and (Test-Path $nupkg)) {
    $sz = (Get-Item $nupkg).Length
    Write-Output "Successfully generated NuGet package: $nupkg ($sz bytes)"
} else {
    throw "Failed to find generated package in $distDir"
}
