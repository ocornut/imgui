
$platformtoolset=$args[0]   #"v142"
$targetplatform=$args[1]    #"10.0.18362.0"

gci -recurse -filter "*.vcxproj" | ForEach-Object {
    # Fix SDK and toolset
    (Get-Content $_.FullName) -Replace "<PlatformToolset>v\d{3}</PlatformToolset>","<PlatformToolset>$platformtoolset</PlatformToolset>" | Set-Content -Path $_.FullName
    (Get-Content $_.FullName) -Replace "<WindowsTargetPlatformVersion>[\d\.]+</WindowsTargetPlatformVersion>","<WindowsTargetPlatformVersion>$targetplatform</WindowsTargetPlatformVersion>" | Set-Content -Path $_.FullName
}
