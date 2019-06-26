cmake_minimum_required (VERSION 3.6)
project (Imogen)
# Add project's cmake modules to path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/.)
message(${CMAKE_MODULE_PATH})
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Configuration types" FORCE)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)
file(GLOB EXT_FILES
    ${CMAKE_SOURCE_DIR}/ext/*.h
    ${CMAKE_SOURCE_DIR}/ext/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/gl3w/GL/*.c
    ${CMAKE_SOURCE_DIR}/ext/cmft/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/cmft/common/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/enkiTS-C-11/src/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/NativeFileDialog/src/nfd_common.c
    ${CMAKE_SOURCE_DIR}/ext/Nvidia-SBVH/*.cpp
    ${CMAKE_SOURCE_DIR}/ext/SOIL/src/*.c
    ${CMAKE_SOURCE_DIR}/ext/GLSL_Pathtracer/*.cpp
)

if(WIN32)
set(NFD_FILES ${CMAKE_SOURCE_DIR}/ext/NativeFileDialog/src/nfd_win.cpp)
set(PLATFORM_LIBS "libtcc")
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
set(PLATFORM_LIBS dl pthread ${GTK3_LIBRARIES} libtcc.a)
endif()


if(WIN32)

set(FFMPEG_LIBRARIES avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib swscale.lib swresample.lib postproc.lib)
set(FFMPEG_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/ffmpeg/lib)
set(FFMPEG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/ffmpeg/include)

set(PYTHON37_LIBRARIES python3.lib python37.lib)
set(PYTHON37_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/python37/include)
set(PYTHON37_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/python37/libs)


set(SDL2_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2-2.0.8/include)
set(SDL2_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/SDL2-2.0.8/lib/x64)

set(TCC_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/tcc-0.9.27/)
set(TCC_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/tcc-0.9.27/libtcc)

else()

PKG_CHECK_MODULES(PYTHON37 REQUIRED python3)
PKG_CHECK_MODULES(SDL2_LIBRARIES REQUIRED sdl2)
PKG_CHECK_MODULES(FFMPEG REQUIRED libavcodec libavdevice libavfilter libavformat libavutil libswscale libswresample libpostproc)
#${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${AVFORMAT_LIBRARY} ${SWSCALE_LIBRARY} ${PYTHON_LIBRARIES}
#TODO: TCC
set(TCC_INCLUDE_DIRS /usr/local/Cellar/tcc/0.9.27_1/)
set(TCC_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/tcc-0.9.27/libtcc)

endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/ext
  ${SDL2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/gl3w
  ${TCC_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/NativeFileDialog/src/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/enkiTS-C-11/src/
  ${FFMPEG_INCLUDE_DIRS}
  ${PYTHON37_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/glm/
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/SOIL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/Nvidia-SBVH
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/GLSL_Pathtracer/
)

link_directories(
  ${SDL2_LIBDIR}
  ${FFMPEG_LIBRARY_DIRS}
  ${PYTHON37_LIBRARY_DIRS}
  ${TCC_LIBRARY_DIRS}
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

set(SDL2_LIBRARIES "SDL2" "SDL2main")

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
elseif(APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)
endif()

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()


SET(LINK_OPTIONS " ")
SET(EXE_NAME "Imogen")

ADD_EXECUTABLE(${EXE_NAME} ${SRC_FILES} ${EXT_FILES} ${NFD_FILES})

TARGET_LINK_LIBRARIES(${EXE_NAME} ${PYTHON37_LIBRARIES} ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${PLATFORM_LIBS} ${FFMPEG_LIBRARIES})

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
if(MSVC)
add_definitions(-DWIN32)
add_definitions(-D_WIN32)
add_definitions(-DNOMINMAX)
endif()
add_definitions(-DUSE_DL_PREFIX)

#--------------------------------------------------------------------
# output dirs
#--------------------------------------------------------------------

set_target_properties("Imogen" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin )
set_target_properties("Imogen" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin )
set_target_properties("Imogen" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin )
set_target_properties("Imogen" PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties("Imogen" PROPERTIES RELWITHDEBINFO_POSTFIX "RelWithDebInfo")
set_target_properties("Imogen" PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_link_libraries("Imogen" ${SDL2_LIBRARIES})

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
set_target_properties("Imogen" PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
set_target_properties("Imogen" PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
endif()

if(ENABLE_HIDECONSOLE_BUILD)
MESSAGE(STATUS "Console is hidden")
else()
MESSAGE(STATUS "Console is visible")
endif()
