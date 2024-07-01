cmake_minimum_required(VERSION 3.2)

project(ImGuiExample)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CONFIGURE_WITH_SDL3 "Configure with SDL3: ON/OFF" OFF)

# Android Project
if(CONFIGURE_WITH_SDL3)
  message(INFO ": Configuring Android example with SDL3")
  if(NOT DEFINED ENV{ANDROID_HOME})
      message(FATAL_ERROR "Set ANDROID_HOME to path of Android Sdk.")
  endif()

  if(NOT DEFINED ENV{ANDROID_NDK_HOME})
      message(FATAL_ERROR "Set ANDROID_NDK_HOME to path of Android Ndk.")
  endif()

  # Download SDL3
  set(SDL_DIR_NAME "SDL-main")
  set(DOWNLOADED_SDL_DIR "${CMAKE_BINARY_DIR}/${SDL_DIR_NAME}")
  set(SDL_MAIN_BRANCH_ZIP_URL "https://github.com/libsdl-org/SDL/archive/main.zip")
  # Package name should match the same in Gradle build files
  set(IMGUI_PKG_NAME "imgui.example.android")
  set(IMGUI_ACTIVITY "ImguiActivity")
  
  file(DOWNLOAD ${SDL_MAIN_BRANCH_ZIP_URL} SHOW_PROGRESS ${DOWNLOADED_SDL_DIR}.zip STATUS DOWNLOAD_STATUS)
  file(ARCHIVE_EXTRACT INPUT "${DOWNLOADED_SDL_DIR}.zip")

  list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
  list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)  

  # Check if download was successful.
  if(${STATUS_CODE} EQUAL 0)
      message(INFO ": ${SDL_DIR_NAME}.zip download completed successfully!")
  else()
      message(FATAL_ERROR "Error occurred during download: ${ERROR_MESSAGE}")
  endif()

  set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
  set(SDL3_OPENGL3_DIR "${IMGUI_DIR}/examples/example_sdl3_opengl3")
  set(ANDROID_OPENGL3_DIR "${IMGUI_DIR}/examples/example_android_opengl3")
  set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends")
  set(SDL3_MAIN_CPP "main_sdl3.cpp")

  # Copy Gradle files
  set(GRADLE_WRAPPER_DOWNLOAD "${DOWNLOADED_SDL_DIR}/android-project/gradlew")
  set(GRADLE_BATCH_DOWNLOAD "${DOWNLOADED_SDL_DIR}/android-project/gradlew.bat")
  set(GRADLE_WRAPPER_JAR "gradle-wrapper.jar")
  set(GRADLE_WRAPPER_DIR "${ANDROID_OPENGL3_DIR}/android/gradle/wrapper")
  set(GRADLE_WRAPPER_PROPS_FILENAME "${GRADLE_WRAPPER_DIR}/gradle-wrapper.properties")
  set(GRADLE_WRAPPER_PROPS_CONTENT "distributionBase=GRADLE_USER_HOME\n"
  "distributionUrl=https\://services.gradle.org/distributions/gradle-8.2-bin.zip\n"
  "distributionPath=wrapper/dists\n"
  "zipStorePath=wrapper/dists\n"
  "zipStoreBase=GRADLE_USER_HOME\n")
  # Check if Gradle Wrapper directory is present in repo
  if(NOT EXISTS ${GRADLE_WRAPPER_DIR})
    add_custom_command(OUTPUT CMD PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${GRADLE_WRAPPER_DIR})
    message(INFO ": Made Gradle Wrapper dir")
  else()
    message(INFO ": Found Gradle Wrapper dir")
  endif()
  file(WRITE ${GRADLE_WRAPPER_PROPS_FILENAME} ${GRADLE_WRAPPER_PROPS_CONTENT})
    
  message(INFO ": Copying Gradle wrapper files to Android project")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GRADLE_WRAPPER_DOWNLOAD}" "${ANDROID_OPENGL3_DIR}/android")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GRADLE_BATCH_DOWNLOAD}" "${ANDROID_OPENGL3_DIR}/android")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DOWNLOADED_SDL_DIR}/android-project/gradle/wrapper/${GRADLE_WRAPPER_JAR}" "${GRADLE_WRAPPER_DIR}")
  
  # Copy SDL3 Java files
  set(SDL_JAVA_SRC_MAIN "${DOWNLOADED_SDL_DIR}/android-project/app/src/main/java")
  set(SDL_JAVA_PKG_PATH "org/libsdl/app")
  set(ANDROID_JAVA_PATH "${ANDROID_OPENGL3_DIR}/android/app/src/main/java")
  set(ANDROID_SDL_JAVA_FILES "${ANDROID_JAVA_PATH}/${SDL_JAVA_PKG}")
  if(NOT EXISTS ${ANDROID_SDL_JAVA_FILES})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_SDL_JAVA_FILES})
    message(INFO ": Made dir ${ANDROID_SDL_JAVA_FILES}")
  else()
    message(INFO ": Found dir ${ANDROID_SDL_JAVA_FILES}")
  endif()
  message(INFO ": Copying SDL3 Java files to Android project")
  list(APPEND SDL_JAVA_FILES HIDDevice HIDDeviceBLESteamController HIDDeviceManager HIDDeviceUSB SDL SDLActivity SDLAudioManager SDLControllerManager SDLDummyEdit SDLInputConnection SDLSurface)
  foreach(JAVA_FILE ${SDL_JAVA_FILES})
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_JAVA_SRC_MAIN}/${SDL_JAVA_PKG_PATH}/${JAVA_FILE}.java" "${ANDROID_SDL_JAVA_FILES}/${SDL_JAVA_PKG_PATH}/${JAVA_FILE}.java")
  endforeach()
  
  # Copy SDL's AndroidManifest.xml over to the repo, rewriting the previous example's manifest
  set(SDL_MANIFEST_FILE ${DOWNLOADED_SDL_DIR}/android-project/app/src/main/AndroidManifest.xml)
  set(ANDROID_MANIFEST_PATH ${ANDROID_JAVA_PATH}/..)
  message(INFO ": Writing ${ANDROID_MANIFEST_PATH}/AndroidManifest.xml")
  file(COPY ${SDL_MANIFEST_FILE} DESTINATION ${ANDROID_MANIFEST_PATH})
  execute_process(COMMAND sed -i -e "s|name=\"SDLActivity\"|name=\"${IMGUI_ACTIVITY}\"|g" "${ANDROID_MANIFEST_PATH}/AndroidManifest.xml")
  # Copy Android resource files
  set(SDL_ANDROID_RES "${DOWNLOADED_SDL_DIR}/android-project/app/src/main/res")
  set(ANDROID_RES_PATH "${ANDROID_OPENGL3_DIR}/android/app/src/main/res")
  message(INFO ": Copying Android resources files")
  if(NOT EXISTS ${ANDROID_RES_PATH})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_RES_PATH})
    file(COPY ${SDL_ANDROID_RES} DESTINATION ${ANDROID_RES_PATH}/..)
    message(INFO ": Made dir ${ANDROID_RES_PATH}")
  else()
    message(INFO ": Found dir ${ANDROID_RES_PATH}")
  endif()
  execute_process(COMMAND sed -i -e "s|Game|${PROJECT_NAME}|g" "${ANDROID_RES_PATH}/values/strings.xml")

  # Create the Android Activity which extends SDLActivity
  set(IMGUI_ACTIVITY_PATH ${ANDROID_JAVA_PATH}/com/example/imgui)
  if(NOT EXISTS ${IMGUI_ACTIVITY_PATH})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${IMGUI_ACTIVITY_PATH})
    message(INFO ": Made ${IMGUI_ACTIVITY_PATH}")
  else()
    message(INFO ": Found ${IMGUI_ACTIVITY_PATH}")
  endif()
  message(INFO ": Writing ${IMGUI_ACTIVITY_PATH}/${IMGUI_ACTIVITY}.java")
  set(ACTIVITY_CONTENT "package ${IMGUI_PKG_NAME}\;\n"
    "import org.libsdl.app.SDLActivity\;\n"
    "public class ${IMGUI_ACTIVITY} extends SDLActivity {}\n")
  file(WRITE ${IMGUI_ACTIVITY_PATH}/${IMGUI_ACTIVITY}.java ${ACTIVITY_CONTENT})

  # Copy/Link SDL3 source files
  option(LINK_SDL_FILES "Link SDL Files" OFF)
  set(ANDROID_JNI_SDL_DIR "${ANDROID_OPENGL3_DIR}/android/app/jni/SDL")
  if(NOT EXISTS ${ANDROID_JNI_SDL_DIR})
    if(LINK_SDL_FILES)
        message(INFO ": Using symbolic links for SDL dir")
        file(CREATE_LINK "${DOWNLOADED_SDL_DIR}" ${ANDROID_JNI_SDL_DIR} SYMBOLIC)
    else()
        message(INFO ": Copying SDL source files to Android JNI dir")
        file(COPY "${DOWNLOADED_SDL_DIR}/include" DESTINATION "${ANDROID_JNI_SDL_DIR}")
        file(COPY "${DOWNLOADED_SDL_DIR}/src" DESTINATION "${ANDROID_JNI_SDL_DIR}")
        file(COPY "${DOWNLOADED_SDL_DIR}/Android.mk" DESTINATION "${ANDROID_JNI_SDL_DIR}")
    endif()
  else()
    message(INFO ": Found SDL sources in Android JNI dir")
  endif()

  # Copy Imgui files and backends
  set(ANDROID_JNI_SRC_DIR "${ANDROID_OPENGL3_DIR}/android/app/jni/src")
  list(APPEND IMGUI_FILES imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp imconfig.h imgui_internal.h imgui.h imstb_rectpack.h imstb_textedit.h imstb_truetype.h)
  list(APPEND IMGUI_BACKENDS imgui_impl_opengl3.cpp imgui_impl_sdl3.cpp imgui_impl_opengl3_loader.h imgui_impl_opengl3.h imgui_impl_sdl3.h)
  if(NOT EXISTS ${ANDROID_JNI_SRC_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_JNI_SRC_DIR})
    message(INFO ": Made Android JNI dir")
  else()
    message(INFO ": Found Android JNI dir")
  endif()
  foreach(IMGUI_FILE ${IMGUI_FILES})
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${IMGUI_DIR}/${IMGUI_FILE}" "${ANDROID_JNI_SRC_DIR}/${IMGUI_FILE}")
  endforeach()
  foreach(IMGUI_BACKEND ${IMGUI_BACKENDS})
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${IMGUI_BACKENDS_DIR}/${IMGUI_BACKEND}" "${ANDROID_JNI_SRC_DIR}/${IMGUI_BACKEND}")
  endforeach()

  # Copy the other SDL3/OpenGL3 example into the Android example (It works!)
  message(INFO ": Copying example_sdl3_opengl3/main.cpp to Android project and renaming to ${SDL3_MAIN_CPP}")
  file(COPY "${SDL3_OPENGL3_DIR}/main.cpp" DESTINATION "${ANDROID_JNI_SRC_DIR}")
  file(RENAME ${ANDROID_JNI_SRC_DIR}/main.cpp ${ANDROID_JNI_SRC_DIR}/${SDL3_MAIN_CPP})

  message(INFO ": Writing src/Android.mk")
  # Create an Android.mk file for the Android SDL3 app
  set(SRC_ANDROID_MK "LOCAL_PATH := $(call my-dir)\n"
    "include $(CLEAR_VARS)\n"
    "LOCAL_MODULE := main\n"
    "SDL_PATH := ../SDL\n"
    "LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include\n"
    "LOCAL_CXXFLAGS := -std=c++11 -Wall -DIMGUI_IMPL_OPENGL_ES2\n"
    "LOCAL_SRC_FILES :=  ${SDL3_MAIN_CPP} \\\n"
    "imgui_demo.cpp \\\n"
    "imgui_draw.cpp \\\n"
    "imgui_tables.cpp \\\n"
    "imgui_widgets.cpp \\\n"
    "imgui.cpp \\\n"
    "imgui_impl_sdl3.cpp \\\n"
    "imgui_impl_opengl3.cpp \\\n"
    "imgui.h \\\n"
    "imconfig.h \\\n"
    "imgui_internal.h \\\n"
    "imstb_rectpack.h \\\n"
    "imstb_textedit.h \\\n"
    "imstb_truetype.h \\\n"
    "imgui_impl_sdl3.h \\\n"
    "imgui_impl_opengl3.h \\\n"
    "imgui_impl_opengl3_loader.h \n"
    "LOCAL_SHARED_LIBRARIES := SDL3\n"
    "LOCAL_LDLIBS := -lEGL -lGLESv3 -lOpenSLES -llog -landroid\n"
    "include $(BUILD_SHARED_LIBRARY)\n")
    file(WRITE ${ANDROID_JNI_SRC_DIR}/Android.mk ${SRC_ANDROID_MK})
    message(INFO ": Writing Android.mk")
    set(ANDROID_MK "include $(call all-subdir-makefiles)\n")
    file(WRITE ${ANDROID_JNI_SRC_DIR}/../Android.mk ${ANDROID_MK})
    message(INFO ": Writing Application.mk")
    set(APPLICATION_MK "APP_ABI := armeabi-v7a arm64-v8a x86 x86_64\nAPP_PLATFORM=android-19\n")
    file(WRITE ${ANDROID_JNI_SRC_DIR}/../Application.mk ${APPLICATION_MK})
else()
    add_library(${CMAKE_PROJECT_NAME} SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../backends/imgui_impl_android.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../backends/imgui_impl_opengl3.cpp
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
  )

  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate"
  )

  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_ES3
  )

  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../backends
    ${ANDROID_NDK}/sources/android/native_app_glue
  )

  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    android
    EGL
    GLESv3
    log
  )

endif()
