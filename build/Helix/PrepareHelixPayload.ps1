$flavor = "Debug"
$platform = "x86"
$payloadDir = "HelixPayload"

$repoDirectory = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "..\..\"
$nugetPackagesDir = Join-Path (Split-Path -Parent $script:MyInvocation.MyCommand.Path) "packages"
 
# Create the payload directory. Remove it if it already exists.
If(test-path $payloadDir)
{
    Remove-Item $payloadDir -Recurse
}
New-Item -ItemType Directory -Force -Path $payloadDir

# Copy files from nuget packages
Copy-Item "$nugetPackagesDir\microsoft.windows.apps.test.1.0.181203002\lib\netcoreapp2.1\*.dll" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.10.31.180822002\build\Binaries\$platform\*" $payloadDir
Copy-Item "$nugetPackagesDir\taef.redist.wlk.10.31.180822002\build\Binaries\$platform\CoreClr\*" $payloadDir
Copy-Item "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" $payloadDir
Copy-Item "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\native\*" $payloadDir
New-Item -ItemType Directory -Force -Path "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\lib\netcoreapp2.1\*" "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\runtime.win-$platform.microsoft.netcore.app.2.1.0\runtimes\win-$platform\native\*" "$payloadDir\.NETCoreApp2.1\"
Copy-Item "$nugetPackagesDir\MUXCustomBuildTasks.1.0.38\tools\$platform\WttLog.dll" $payloadDir

# Copy files from the 'drop' artifact dir
Copy-Item "$repoDirectory\Artifacts\drop\$flavor\$platform\Test\MUXControls.Test.dll" $payloadDir
Copy-Item "$repoDirectory\Artifacts\drop\$flavor\$platform\AppxPackages\MUXControlsTestApp_Test\*" $payloadDir
Copy-Item "$repoDirectory\Artifacts\drop\$flavor\$platform\AppxPackages\MUXControlsTestApp_Test\Dependencies\$platform\*" $payloadDir

# Copy files from the repo
Copy-Item "build\helix\runtests.cmd" $payloadDir
New-Item -ItemType Directory -Force -Path "$payloadDir\scripts"
Copy-Item "build\helix\ConvertWttLogToXUnit.ps1" "$payloadDir\scripts"
Copy-Item "build\helix\ConvertWttLogToXUnit.cs" "$payloadDir\scripts"