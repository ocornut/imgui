# This is current meant to be run manually, occasionally:
# - Run then zip the contents of vulkanArtifact/ into e.g. vulkan_windows_libs_1.4.326
# - Upload as an attachment to https://github.com/ocornut/imgui/pull/8925 then change filename in build.yml
# - There is a build_windows_vulkan_libs.yml in commit history that we removed thinking this is run so rarely we don't need to pollute CI UI with it.

# Set default vulkan version if none provided
if (-not $env:VULKAN_TAG) { $env:VULKAN_TAG = "1.4.326" }

# Create output folder
mkdir vulkanArtifact

# Download Vulkan Headers

Invoke-WebRequest -Uri "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v$($env:VULKAN_TAG).zip" -OutFile Vulkan-Headers-$($env:VULKAN_TAG).zip
Expand-Archive -Path Vulkan-Headers-$($env:VULKAN_TAG).zip

# Copy Vulkan Headers to artifact folder

cp -R Vulkan-Headers-$($env:VULKAN_TAG)\Vulkan-Headers-$($env:VULKAN_TAG)\include vulkanArtifact\Include

# Download Vulkan Loader

Invoke-WebRequest -Uri "https://github.com/KhronosGroup/Vulkan-Loader/archive/refs/tags/v$($env:VULKAN_TAG).zip" -OutFile Vulkan-Loader-$($env:VULKAN_TAG).zip
Expand-Archive -Path Vulkan-Loader-$($env:VULKAN_TAG).zip

# Build Vulkan Loader x64

cmake -S Vulkan-Loader-$($env:VULKAN_TAG)\Vulkan-Loader-$($env:VULKAN_TAG) -B VulkanLoader-build64 -D UPDATE_DEPS=On -A x64
cmake --build VulkanLoader-build64
mkdir vulkanArtifact\Lib
copy VulkanLoader-build64\loader\Debug\vulkan-1.lib vulkanArtifact\Lib

# Build Vulkan Loader win32

cmake -S Vulkan-Loader-$($env:VULKAN_TAG)\Vulkan-Loader-$($env:VULKAN_TAG) -B VulkanLoader-build32 -D UPDATE_DEPS=On -A Win32
cmake --build VulkanLoader-build32
mkdir vulkanArtifact\Lib32
copy VulkanLoader-build32\loader\Debug\vulkan-1.lib vulkanArtifact\Lib32
