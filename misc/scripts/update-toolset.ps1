# WARNING: This will need updating if toolset/sdk change in project files!
gci -recurse -filter "*.vcxproj" | ForEach-Object {
    # Fix SDK and toolset for most samples.
    (Get-Content $_.FullName) -Replace "<PlatformToolset>v110</PlatformToolset>","<PlatformToolset>v142</PlatformToolset>" | Set-Content -Path $_.FullName
    (Get-Content $_.FullName) -Replace "<WindowsTargetPlatformVersion>8.1</WindowsTargetPlatformVersion>","<WindowsTargetPlatformVersion>10.0.18362.0</WindowsTargetPlatformVersion>" | Set-Content -Path $_.FullName
    # Fix SDK and toolset for samples that require newer SDK/toolset. At the moment it is only dx12.
    (Get-Content $_.FullName) -Replace "<PlatformToolset>v140</PlatformToolset>","<PlatformToolset>v142</PlatformToolset>" | Set-Content -Path $_.FullName
    (Get-Content $_.FullName) -Replace "<WindowsTargetPlatformVersion>10.0.14393.0</WindowsTargetPlatformVersion>","<WindowsTargetPlatformVersion>10.0.18362.0</WindowsTargetPlatformVersion>" | Set-Content -Path $_.FullName
}
