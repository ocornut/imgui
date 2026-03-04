# ============================================================
# */
# * XDKToolchain.cmake 
# * 
# * Copyright (C) 2026 Guilherme M. Ladario - Guila767
# * Licensed under the GNU General Public License v3.0
# * 
# * @author Guilherme M. Ladario - Guila767
# * @license GPL-3.0
# */
#
#  Toolchain file for minimal cmake support for the
#  XDK for the xbox 360
# ============================================================

cmake_minimum_required(VERSION 3.20)

# ----------------------------------------------------------
# 1. Target platform
# ----------------------------------------------------------
set(CMAKE_SYSTEM_NAME      "Xbox360")
set(CMAKE_SYSTEM_VERSION   1)
set(CMAKE_SYSTEM_PROCESSOR "PowerPC")
set(CMAKE_CROSSCOMPILING   TRUE)

# ----------------------------------------------------------
# 2. Locate XDK
# ----------------------------------------------------------
if(NOT DEFINED XEDK_DIR)
    if(DEFINED ENV{XEDK})
        string(REGEX REPLACE "[/\\\\]+$" "" XEDK_DIR "$ENV{XEDK}")
    else()
        message(FATAL_ERROR "Defina XEDK no ambiente ou passe -DXEDK_DIR=<path>")
    endif()
endif()
file(TO_CMAKE_PATH "${XEDK_DIR}" XEDK_DIR)
message(STATUS "XDK: ${XEDK_DIR}")

set(XDK_BIN_DIR     "${XEDK_DIR}/bin/win32")
set(XDK_INCLUDE_DIR "${XEDK_DIR}/include/xbox")
set(XDK_LIB_DIR     "${XEDK_DIR}/lib/xbox")

# ----------------------------------------------------------
# 3. Tooling
# ----------------------------------------------------------
set(CMAKE_C_COMPILER   "${XDK_BIN_DIR}/cl.exe"  CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${XDK_BIN_DIR}/cl.exe"  CACHE FILEPATH "" FORCE)
set(CMAKE_LINKER       "${XDK_BIN_DIR}/link.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_AR           "${XDK_BIN_DIR}/lib.exe"  CACHE FILEPATH "" FORCE)

set(CMAKE_C_COMPILER_WORKS   1 CACHE BOOL "" FORCE)
set(CMAKE_CXX_COMPILER_WORKS 1 CACHE BOOL "" FORCE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ----------------------------------------------------------
# 4. Compiler rules:
# ----------------------------------------------------------
set(CMAKE_CXX_COMPILE_OBJECT
    "<CMAKE_CXX_COMPILER> <FLAGS> <DEFINES> <INCLUDES> /nologo /c /Fo<OBJECT> <SOURCE>"
    CACHE STRING "" FORCE)
set(CMAKE_C_COMPILE_OBJECT
    "<CMAKE_C_COMPILER> <FLAGS> <DEFINES> <INCLUDES> /nologo /c /Fo<OBJECT> <SOURCE>"
    CACHE STRING "" FORCE)

# ----------------------------------------------------------
# 5. Liker rules
# ----------------------------------------------------------
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> /OUT:<TARGET> <LINK_LIBRARIES>"
    CACHE STRING "" FORCE)
set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> /OUT:<TARGET> <LINK_LIBRARIES>"
    CACHE STRING "" FORCE)

set(CMAKE_CXX_CREATE_STATIC_LIBRARY
    "<CMAKE_AR> /OUT:<TARGET> <OBJECTS>"
    CACHE STRING "" FORCE)
set(CMAKE_C_CREATE_STATIC_LIBRARY
    "<CMAKE_AR> /OUT:<TARGET> <OBJECTS>"
    CACHE STRING "" FORCE)

# ----------------------------------------------------------
# 6. Linker flags
#      Debug:   LinkIncremental=true,  GenerateDebugInformation=true
#      Release: LinkIncremental=false, OPT:REF, OPT:ICF, SetChecksum
#
#    /MACHINE:PPCBE = PowerPC Big Endian (Xbox 360)
# ----------------------------------------------------------
file(TO_NATIVE_PATH "${XDK_LIB_DIR}" _XDK_LIB_NATIVE)

set(CMAKE_EXE_LINKER_FLAGS
    "/MACHINE:PPCBE /DEBUG \"/LIBPATH:${_XDK_LIB_NATIVE}\""
    CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEBUG
    "/INCREMENTAL"
    CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_RELEASE
    "/INCREMENTAL:NO /OPT:REF /OPT:ICF /RELEASE"
    CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
    "/INCREMENTAL:NO /OPT:REF /RELEASE"
    CACHE STRING "" FORCE)

# ----------------------------------------------------------
# 7. Compiler flags
#
#    Debug:
#      Optimization=Disabled (/Od)
#      DebugInformationFormat=ProgramDatabase (/Zi)
#      RuntimeLibrary=MultiThreadedDebug (/MTd)
#      BufferSecurityCheck=false (/GS-)
#      ExceptionHandling=false (/EHs-c-)
#      MinimalRebuild=true (/Gm) — omitido, incompatível com Ninja
#      PreprocessorDefinitions=_DEBUG;_XBOX
#
#    Release:
#      Optimization=Full (/O2)
#      FavorSizeOrSpeed=Size (/Os)
#      FunctionLevelLinking=true (/Gy)
#      StringPooling=true (/GF)
#      DebugInformationFormat=ProgramDatabase (/Zi)
#      RuntimeLibrary=MultiThreaded (/MT)
#      BufferSecurityCheck=false (/GS-)
#      ExceptionHandling=false (/EHs-c-)
#      PreprocessorDefinitions=NDEBUG;_XBOX
# ----------------------------------------------------------
set(_XDK_COMMON "/W3 /GS- /EHs-c- /D_XBOX /D_MSC_VER=1600 /nologo")

set(CMAKE_C_FLAGS_DEBUG
    "/Od /Zi /MTd /D_DEBUG ${_XDK_COMMON}"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
    "/Od /Zi /MTd /D_DEBUG ${_XDK_COMMON}"
    CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS_RELEASE
    "/O2 /Os /Gy /GF /Zi /MT /DNDEBUG ${_XDK_COMMON}"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
    "/O2 /Os /Gy /GF /Zi /MT /DNDEBUG ${_XDK_COMMON}"
    CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS_RELWITHDEBINFO
    "/O2 /Os /Gy /GF /Zi /MT /DNDEBUG /DPROFILE ${_XDK_COMMON}"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "/O2 /Os /Gy /GF /Zi /MT /DNDEBUG /DPROFILE ${_XDK_COMMON}"
    CACHE STRING "" FORCE)

# ----------------------------------------------------------
# 8. Includes
# ----------------------------------------------------------
set(CMAKE_FIND_ROOT_PATH "${XEDK_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include_directories(SYSTEM "${XDK_INCLUDE_DIR}")

# ----------------------------------------------------------
# 9. XDK Libraries
#    Libries included in the XDK
#
#    Debug:
#      xapilibd.lib;d3d9d.lib;d3dx9d.lib;xgraphicsd.lib;
#      xboxkrnl.lib;xnetd.lib;xaudiod2.lib;xactd3.lib;
#      x3daudiod.lib;xmcored.lib;xbdm.lib;vcompd.lib
#
#    Release:
#      xapilib.lib;d3d9.lib;d3dx9.lib;xgraphics.lib;
#      xboxkrnl.lib;xnet.lib;xaudio2.lib;xact3.lib;
#      x3daudio.lib;xmcore.lib;vcomp.lib
# ----------------------------------------------------------
set(XDK_LIBS_DEBUG
    xapilibd.lib d3d9d.lib d3dx9d.lib xgraphicsd.lib
    xboxkrnl.lib xnetd.lib xaudiod2.lib xactd3.lib
    x3daudiod.lib xmcored.lib xbdm.lib vcompd.lib
    CACHE STRING "Libs XDK (Debug)")

set(XDK_LIBS_RELEASE
    xapilib.lib d3d9.lib d3dx9.lib xgraphics.lib
    xboxkrnl.lib xnet.lib xaudio2.lib xact3.lib
    x3daudio.lib xmcore.lib vcomp.lib
    CACHE STRING "Libs XDK (Release)")

# ----------------------------------------------------------
# 10. imagexex
# ----------------------------------------------------------
set(XDK_IMAGEXEX "${XDK_BIN_DIR}/imagexex.exe" CACHE FILEPATH "imagexex")

# ----------------------------------------------------------
#  FUNCTION: xdk_target_link_libs
#   > links the libraries based on the current configuration
#
#  Interface:
#    xdk_target_link_libs(<target>
#        [DEBUG   lib1 lib2 ...]   # overwrite XDK_LIBS_DEBUG
#        [RELEASE lib1 lib2 ...]   # overwrite XDK_LIBS_RELEASE
#    )
#
#  No args -> uses the default from the toolchain.
#
#  Examples:
#    # toolchain default (most common):
#    xdk_target_link_libs(MyTarget)
#
#    # No graphics on Debug, default on Release:
#    xdk_target_link_libs(MyTarget
#        DEBUG xapilibd.lib xboxkrnl.lib xnetd.lib
#    )
#
#    # Overriding both configs:
#    xdk_target_link_libs(MyTarget
#        DEBUG   xapilibd.lib xboxkrnl.lib d3d9d.lib xbdm.lib
#        RELEASE xapilib.lib  xboxkrnl.lib d3d9.lib
#    )
# ----------------------------------------------------------
function(xdk_target_link_libs TARGET_NAME)
    cmake_parse_arguments(_XDK "" "" "DEBUG;RELEASE" ${ARGN})

    if(_XDK_DEBUG)
        set(_libs_dbg ${_XDK_DEBUG})
    else()
        set(_libs_dbg ${XDK_LIBS_DEBUG})
    endif()

    if(_XDK_RELEASE)
        set(_libs_rel ${_XDK_RELEASE})
    else()
        set(_libs_rel ${XDK_LIBS_RELEASE})
    endif()

    target_link_options(${TARGET_NAME} PRIVATE
        "$<$<CONFIG:Debug>:${_libs_dbg}>"
        "$<$<NOT:$<CONFIG:Debug>>:${_libs_rel}>"
    )

    message(STATUS "[XDK] ${TARGET_NAME} libs Debug  : ${_libs_dbg}")
    message(STATUS "[XDK] ${TARGET_NAME} libs Release: ${_libs_rel}")
endfunction()

# ----------------------------------------------------------
# FUNCTION: dk_add_xe: 
#  > generates the .xex (xbox executable)
#     default TitleID FFFF0000 ()
# ----------------------------------------------------------
function(xdk_add_xex TARGET_NAME)
    cmake_parse_arguments(XEX "" "TITLE_ID;XEX_CONFIG" "" ${ARGN})
    if(NOT XEX_TITLE_ID)
        set(XEX_TITLE_ID "FFFF0000")
    endif()

    set(_XEX "$<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}.xex")
    set(_FLAGS /titleid ${XEX_TITLE_ID} /nologo)
    if(XEX_XEX_CONFIG AND EXISTS "${XEX_XEX_CONFIG}")
        list(APPEND _FLAGS /config "${XEX_XEX_CONFIG}")
    endif()

    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${XDK_IMAGEXEX}" ${_FLAGS} "/OUT:${_XEX}" "$<TARGET_FILE:${TARGET_NAME}>"
        COMMENT "imagexex -> ${TARGET_NAME}.xex (TitleID=${XEX_TITLE_ID})"
        VERBATIM
    )
endfunction()