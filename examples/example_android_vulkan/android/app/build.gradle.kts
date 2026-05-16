import java.net.HttpURLConnection
import java.net.URI
import java.util.zip.ZipFile

plugins {
    alias(libs.plugins.android.application)
}

abstract class FetchVkValidationLayersTask : DefaultTask() {

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    private val validationLayersRoot =
        project.layout.buildDirectory.dir(
            "generated/vulkanValidationLayers"
        )
    private val markerFile = validationLayersRoot.map { it.file(".version") }
    private val cachedZip = validationLayersRoot.map { it.file("validationLayers.zip") }
    private val validationAbis = listOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")

    @TaskAction
    fun run() {
        println("Resolving latest Vulkan ValidationLayers release...")

        val url = "https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/latest"
        val connection = URI(url).toURL().openConnection() as HttpURLConnection
        connection.instanceFollowRedirects = false

        // /KhronosGroup/Vulkan-ValidationLayers/releases/tag/vulkan-sdk-<version>
        val redirect = connection.getHeaderField("Location") ?: error("Failed to resolve latest release")
        val tag = redirect.substringAfterLast("/")
        val version = tag.removePrefix("vulkan-sdk-")

        println("Latest Vulkan ValidationLayers version: $version")

        val marker = markerFile.get().asFile

        if (marker.exists() && marker.readText().trim() == version) {
            println("Validation layers already up to date.")
            return
        }

        val zipUrl =
            "https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/download/$tag/android-binaries-$version.zip"
        val zipFile = cachedZip.get().asFile

        zipFile.parentFile.mkdirs()
        println("Downloading Vulkan validation layers...")

        URI(zipUrl)
            .toURL()
            .openStream()
            .use { input ->
                zipFile.outputStream().use { output ->
                    input.copyTo(output)
                }
            }

        val outDir = outputDir.get().asFile

        outDir.deleteRecursively()
        outDir.mkdirs()

        println("Extracting validation layers...")

        ZipFile(zipFile).use { zip ->
            validationAbis.forEach { abi ->
                val entryName = "$abi/libVkLayer_khronos_validation.so"
                val entry = zip.entries()
                    .asSequence()
                    .firstOrNull { it.name.endsWith(entryName) }
                    ?: error("Missing $entryName")

                val outFile = File(outDir, entryName)
                outFile.parentFile.mkdirs()

                zip.getInputStream(entry).use { input ->
                    outFile.outputStream().use { output ->
                        input.copyTo(output)
                    }
                }
            }
        }

        marker.writeText(version)
        println("Validation layers updated to $version")
    }
}

val fetchVkValidationLayers by tasks.registering(FetchVkValidationLayersTask::class) {
    outputDir.set(
        layout.buildDirectory.dir(
            "generated/vulkanValidationLayers/jniLibs"
        )
    )

    onlyIf {
        gradle.startParameter.taskNames.any {
            it.contains("Debug", ignoreCase = true)
        }
    }
}

android {
    ndkVersion = "29.0.14206865"
    namespace = "imgui.example.android.vulkan"

    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "imgui.example.android.vulkan"
        minSdk = 30
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        @Suppress("UnstableApiUsage")
        externalNativeBuild {
            cmake {
                cppFlags += "-DVK_USE_PLATFORM_ANDROID_KHR"
                arguments += "-DANDROID_STL=c++_static"
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.core.ktx)
    implementation(libs.material)
}

androidComponents {
    onVariants(selector().withBuildType("debug")) { variant ->
        variant.sources.jniLibs?.addGeneratedSourceDirectory(
            fetchVkValidationLayers,
            FetchVkValidationLayersTask::outputDir
        )
    }
}
