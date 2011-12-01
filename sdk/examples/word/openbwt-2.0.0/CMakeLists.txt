### cmake file for building openbwt Package ###
cmake_minimum_required(VERSION 2.6.4)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(AppendCompilerFlags)

## Project information ##
project(openbwt C CXX)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" PROJECT_VERSION_FULL)
string(REGEX REPLACE "[\n\r]" "" PROJECT_VERSION_FULL "${PROJECT_VERSION_FULL}")
string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+$" "\\1" PROJECT_VERSION_MAJOR "${PROJECT_VERSION_FULL}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+$" "\\1" PROJECT_VERSION_MINOR "${PROJECT_VERSION_FULL}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" PROJECT_VERSION_PATCH "${PROJECT_VERSION_FULL}")
math(EXPR LIBRARY_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(LIBRARY_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(LIBRARY_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(LIBRARY_VERSION "${LIBRARY_VERSION_MAJOR}.${LIBRARY_VERSION_MINOR}")
set(LIBRARY_VERSION_FULL "${LIBRARY_VERSION}.${LIBRARY_VERSION_PATCH}")

## CPack configuration ##
set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR "ZIP")
include(ProjectCPack)

## Project options ##
option(BUILD_SHARED_LIBS "Set to OFF to build static libraries" OFF)
option(BUILD_SAMPLES "Build samples" ON)
option(USE_OPENMP "Use OpenMP for parallelization" OFF)
option(WITH_LFS "Enable Large File Support" ON)

## Build type ##
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
#  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_VERBOSE_MAKEFILE ON)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

## Compiler options ##
if(MSVC)
#  append_c_compiler_flags("/W4" "VC" CMAKE_C_FLAGS)
  append_c_compiler_flags("/Oi;/Ot;/Ox;/Oy" "VC" CMAKE_C_FLAGS_RELEASE)
elseif(BORLAND)
  append_c_compiler_flags("-w;-w-8071;-w-8072" "BCC" CMAKE_C_FLAGS)
  append_c_compiler_flags("-Oi;-Og;-Os;-Ov;-Ox" "BCC" CMAKE_C_FLAGS_RELEASE)
else(MSVC)
  if(CMAKE_COMPILER_IS_GNUCC)
    append_c_compiler_flags("-Wall" "GCC" CMAKE_C_FLAGS)
    append_c_compiler_flags("-fomit-frame-pointer" "GCC" CMAKE_C_FLAGS_RELEASE)
  else(CMAKE_COMPILER_IS_GNUCC)
    append_c_compiler_flags("-Wall" "UNKNOWN" CMAKE_C_FLAGS)
    append_c_compiler_flags("-fomit-frame-pointer" "UNKNOWN" CMAKE_C_FLAGS_RELEASE)
  endif(CMAKE_COMPILER_IS_GNUCC)
endif(MSVC)
if(USE_OPENMP)
  find_package(OpenMP)
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  endif(OPENMP_FOUND)
endif(USE_OPENMP)

## Add definitions ##
add_definitions(-DHAVE_CONFIG_H=1 -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS)

## Add subdirectories ##
add_subdirectory(include)
add_subdirectory(src)
if(BUILD_SAMPLES)
  add_subdirectory(samples)
endif(BUILD_SAMPLES)

## Add 'uninstall' target ##
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/cmake_uninstall.cmake")
