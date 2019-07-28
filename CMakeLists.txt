cmake_minimum_required (VERSION 3.6)
project (Imogen)
# Add project's cmake modules to path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/.)
message(${CMAKE_MODULE_PATH})
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Configuration types" FORCE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
file(GLOB EXT_FILES
    ${CMAKE_SOURCE_DIR}/ext/*.h
    ${CMAKE_SOURCE_DIR}/ext/*.cpp
	${CMAKE_SOURCE_DIR}/ext/duktape/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/Nvidia-SBVH/*.cpp
)

if (NOT EMSCRIPTEN)
file(GLOB EXT_FILES_DESKTOP
    ${CMAKE_SOURCE_DIR}/ext/gl3w/GL/*.c
    ${CMAKE_SOURCE_DIR}/ext/enkiTS-C-11/src/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/NativeFileDialog/src/nfd_common.c
    ${CMAKE_SOURCE_DIR}/ext/SOIL/src/*.c
    ${CMAKE_SOURCE_DIR}/ext/ffmpegCodec/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/GLSL_Pathtracer/*.cpp
)
set(SDL2_LIBRARIES "SDL2" "SDL2main")
endif()

if(WIN32)
set(NFD_FILES ${CMAKE_SOURCE_DIR}/ext/NativeFileDialog/src/nfd_win.cpp)
elseif (EMSCRIPTEN)
else()
set(NFD_FILES ${CMAKE_SOURCE_DIR}/ext/NativeFileDialog/src/nfd_gtk.c
)
# Use the package PkgConfig to detect GTK+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

#find_package(FFmpeg REQUIRED)

find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(AVDEVICE_LIBRARY avdevice)
find_library(SWSCALE_LIBRARY swscale)
find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})
set(PLATFORM_LIBS dl pthread ${GTK3_LIBRARIES})
endif()

set(IMOGEN_BGFX_DIR ${CMAKE_SOURCE_DIR}/ext/bgfx)

if(WIN32)

set(FFMPEG_LIBRARIES avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib swscale.lib swresample.lib postproc.lib)
set(FFMPEG_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/ffmpeg/lib)
set(FFMPEG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/ffmpeg/include)

set(PYTHON37_LIBRARIES python3.lib python37.lib)
set(PYTHON37_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/python37/include)
set(PYTHON37_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/python37/libs)


set(SDL2_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2/include)
set(SDL2_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2/lib/x64)

elseif (EMSCRIPTEN)
else()

PKG_CHECK_MODULES(PYTHON37 REQUIRED python3)
PKG_CHECK_MODULES(SDL2_LIBRARIES REQUIRED sdl2)
PKG_CHECK_MODULES(FFMPEG REQUIRED libavcodec libavdevice libavfilter libavformat libavutil libswscale libswresample libpostproc)
#${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${AVFORMAT_LIBRARY} ${SWSCALE_LIBRARY} ${PYTHON_LIBRARIES}

endif()

set(BGFX_INCLUDE_DIRS "${IMOGEN_BGFX_DIR}/bgfx/include" "${IMOGEN_BGFX_DIR}/bimg/include" "${IMOGEN_BGFX_DIR}/bx/include")
if(WIN32)
set(BGFX_INCLUDE_DIRS "${BGFX_INCLUDE_DIRS}" "${IMOGEN_BGFX_DIR}/bx/include/compat/msvc")
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/ext
  ${SDL2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/gl3w
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/NativeFileDialog/src/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/enkiTS-C-11/src/
  ${FFMPEG_INCLUDE_DIRS}
  ${PYTHON37_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/glm/
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/SOIL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/Nvidia-SBVH
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/GLSL_Pathtracer/
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/ffmpegCodec/
  ${BGFX_INCLUDE_DIRS}
)

link_directories(
  ${SDL2_LIBDIR}
  ${FFMPEG_LIBRARY_DIRS}
  ${PYTHON37_LIBRARY_DIRS}
  bx bimg bgfx
)
find_package(OpenGL)

#######################################################################################

set (USE_MSVC_RUNTIME_LIBRARY_DLL true)
IF (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
    #We statically link to reduce dependancies
    FOREACH(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RETAIL)
        IF(${flag_var} MATCHES "/MD")
            STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF(${flag_var} MATCHES "/MD")
        IF(${flag_var} MATCHES "/MDd")
            STRING(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
        ENDIF(${flag_var} MATCHES "/MDd")
    ENDFOREACH(flag_var)
ENDIF (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)

#######################################################################################

foreach(f ${SRC_FILES})
    # Get the path of the file relative to ${DIRECTORY},
    # then alter it (not compulsory)
    file(RELATIVE_PATH SRCGR ${CMAKE_SOURCE_DIR} ${f})
    set(SRCGR "Sources/${SRCGR}")

    # Extract the folder, ie remove the filename part
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})

    # Source_group expects \\ (double antislash), not / (slash)
    string(REPLACE / \\ SRCGR ${SRCGR})
    source_group("${SRCGR}" FILES ${f})
endforeach()

set(ENABLE_HIDECONSOLE_BUILD TRUE CACHE BOOL "TRUE to hide console for Windows sys")

if(WINDOWS)
  set(GUI_TYPE WIN32)
elseif (EMSCRIPTEN)
elseif(APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)
endif()

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()


SET(LINK_OPTIONS " ")
SET(EXE_NAME "Imogen")

ADD_EXECUTABLE(${EXE_NAME} ${SRC_FILES} ${EXT_FILES} ${NFD_FILES} ${EXT_FILES_DESKTOP})

if (EMSCRIPTEN)
add_definitions(-DEMSCRIPTEN)
add_definitions(-D_X86_)
set(EMS_OPTS "-s USE_SDL=2 -s USE_WEBGL2=1 -s WASM=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s BINARYEN_TRAP_MODE=clamp --bind -O2 --no-heap-copy -g4 --source-map-base http://localhost:8000/")
add_definitions(${EMS_OPTS})
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(EMS_PRELOAD " --preload-file DefaultLibrary.imogen --preload-file imgui.ini --preload-file Nodes --preload-file Stock")
set(EMS_LINK "${EMS_OPTS} ${EMS_PRELOAD}")
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS ${EMS_LINK})
#set_target_properties(${EXE_NAME} PROPERTIES COMPILE_FLAGS ${EMS_OPTS})

add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html
)

# copy files and directories because packer doesn't know where to place then
add_custom_command(TARGET ${EXE_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/bin/DefaultLibrary.imogen ${CMAKE_CURRENT_BINARY_DIR}/
)
add_custom_command(TARGET ${EXE_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/bin/imgui.ini ${CMAKE_CURRENT_BINARY_DIR}/
)

add_custom_command(TARGET ${EXE_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/Nodes ${CMAKE_CURRENT_BINARY_DIR}/Nodes
)

add_custom_command(TARGET ${EXE_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/Stock ${CMAKE_CURRENT_BINARY_DIR}/Stock
)

endif()

set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "Build the BGFX examples.")
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "Build the BGFX tools.")
add_subdirectory(${IMOGEN_BGFX_DIR}/)
set_property(TARGET astc PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET astc-codec PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET edtaa3 PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET etc1 PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET etc2 PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET iqa PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET nvtt PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET pvrtc PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET squish PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx/3rdparty)
set_property(TARGET bgfx PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx)
set_property(TARGET bimg PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx)
set_property(TARGET bx PROPERTY FOLDER ${IMOGEN_BGFX_DIR}/bgfx)


TARGET_LINK_LIBRARIES(${EXE_NAME} ${PYTHON37_LIBRARIES} ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${PLATFORM_LIBS} ${FFMPEG_LIBRARIES} bx bimg bgfx)

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(${EXE_NAME} ${CORE_FOUNDATION})
endif()

#--------------------------------------------------------------------
# preproc
#--------------------------------------------------------------------
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-DBX_CONFIG_ENABLE_MSVC_LEVEL4_WARNINGS=1)
add_definitions(-D__STDC_LIMIT_MACROS)
add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)
add_definitions(-DDUK_USE_CPP_EXCEPTIONS)
if(MSVC)
add_definitions(-DWIN32)
add_definitions(-D_WIN32)
add_definitions(-DNOMINMAX)
endif()
add_definitions(-DUSE_DL_PREFIX)

#--------------------------------------------------------------------
# output dirs
#--------------------------------------------------------------------

set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin )
set_target_properties(${EXE_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${EXE_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX "RelWithDebInfo")
set_target_properties(${EXE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

#--------------------------------------------------------------------
# Hide the console window in visual studio projects
#--------------------------------------------------------------------
if(ENABLE_HIDECONSOLE_BUILD)
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()
endif()

if(WINDOWS)
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
endif()

if(ENABLE_HIDECONSOLE_BUILD)
MESSAGE(STATUS "Console is hidden")
else()
MESSAGE(STATUS "Console is visible")
endif()
