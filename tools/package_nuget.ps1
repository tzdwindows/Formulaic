$ErrorActionPreference = "Stop"

$nugetDir = "F:\Formulaic\nuget"
if (Test-Path $nugetDir) {
    Remove-Item -Recurse -Force $nugetDir
}

$buildNativeDir = Join-Path $nugetDir "build\native"
$buildDir = Join-Path $nugetDir "build"
$toolsDir = Join-Path $nugetDir "tools"

New-Item -ItemType Directory -Force -Path (Join-Path $buildNativeDir "include") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $buildNativeDir "lib\x64\Release") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $buildNativeDir "bin\x64\Release") | Out-Null
New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null

# 1. Copy headers
Copy-Item -Recurse "F:\Formulaic\include\Formulaic" (Join-Path $buildNativeDir "include")

# 2. Copy libs and dll
Copy-Item "F:\Formulaic\build\Release\Formulaic.lib" (Join-Path $buildNativeDir "lib\x64\Release")
Copy-Item "F:\Formulaic\build\Release\Formulaic_static.lib" (Join-Path $buildNativeDir "lib\x64\Release")
Copy-Item "F:\Formulaic\build\Release\Formulaic.dll" (Join-Path $buildNativeDir "bin\x64\Release")

# 3. Copy tools
Copy-Item "F:\Formulaic\build\test\Release\test_editor_window.exe" $toolsDir
Copy-Item "F:\Formulaic\build\examples\Release\example_interactive_window.exe" $toolsDir

# 4. Generate Formulaic.targets for MSBuild in Visual Studio
$nativeTargetsContent = @"
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>`$(MSBuildThisFileDirectory)include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <LanguageStandard Condition="'`$(LanguageStandard)' == '' Or '`$(LanguageStandard)' &lt; 'stdcpp20'">stdcpp20</LanguageStandard>
    </ClCompile>
    <Link Condition="'`$(Platform)' == 'x64' And '`$(FormulaicUseStatic)' != 'true'">
      <AdditionalDependencies>`$(MSBuildThisFileDirectory)lib\x64\Release\Formulaic.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
    <Link Condition="'`$(Platform)' == 'x64' And '`$(FormulaicUseStatic)' == 'true'">
      <AdditionalDependencies>`$(MSBuildThisFileDirectory)lib\x64\Release\Formulaic_static.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>

  <ItemGroup Condition="'`$(Platform)' == 'x64' And '`$(FormulaicUseStatic)' != 'true'">
    <ReferenceCopyLocalPaths Include="`$(MSBuildThisFileDirectory)bin\x64\Release\Formulaic.dll" />
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
