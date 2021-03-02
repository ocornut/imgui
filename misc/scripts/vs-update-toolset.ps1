
# This script searches for all visual studio project files (*.vcxproj),
# and update the target platform (sdk version) and used toolset (compiler) version
# This is used in the github build script (.github/workflows/build.yml),
# and can be used by developers to batch-update all project to their preferred versions.

$platformtoolset=$args[0]   #"v142"
$targetplatform=$args[1]    #"10.0.18362.0"

gci -recurse -filter "*.vcxproj" | ForEach-Object {
    # Fix SDK and toolset
    (Get-Content $_.FullName) -Replace "<PlatformToolset>v\d{3}</PlatformToolset>","<PlatformToolset>$platformtoolset</PlatformToolset>" | Set-Content -Path $_.FullName
    (Get-Content $_.FullName) -Replace "<WindowsTargetPlatformVersion>[\d\.]+</WindowsTargetPlatformVersion>","<WindowsTargetPlatformVersion>$targetplatform</WindowsTargetPlatformVersion>" | Set-Content -Path $_.FullName
}
