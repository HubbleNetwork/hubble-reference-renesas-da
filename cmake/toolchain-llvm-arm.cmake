# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ---------------------------------------------------------------------------
# Locate the toolchain
# ---------------------------------------------------------------------------
if(NOT LLVM_ARM_TOOLCHAIN_PATH AND DEFINED ENV{LLVM_ARM_TOOLCHAIN_PATH})
    set(LLVM_ARM_TOOLCHAIN_PATH "$ENV{LLVM_ARM_TOOLCHAIN_PATH}")
endif()

# Unpacked under external/. Newest wins. Otherwise a `clang` on PATH, or set
# LLVM_ARM_TOOLCHAIN_PATH -- see the error below.
if(NOT LLVM_ARM_TOOLCHAIN_PATH)
    get_filename_component(_tc_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

    file(GLOB _tc_candidates
         "${_tc_repo_root}/external/LLVMEmbeddedToolchainForArm-*")
    list(SORT _tc_candidates)
    list(REVERSE _tc_candidates)

    foreach(_tc_candidate ${_tc_candidates})
        if(EXISTS "${_tc_candidate}/bin")
            set(LLVM_ARM_TOOLCHAIN_PATH "${_tc_candidate}")
            break()
        endif()
    endforeach()
endif()

# CMake re-reads this file inside the throwaway project it builds to identify
# the compiler, and that project does not inherit -D cache variables. Without
# this, a toolchain given on the command line is invisible during compiler
# detection, which then fails reporting that clang cannot be found.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES LLVM_ARM_TOOLCHAIN_PATH)

if(LLVM_ARM_TOOLCHAIN_PATH)
    file(TO_CMAKE_PATH "${LLVM_ARM_TOOLCHAIN_PATH}" LLVM_ARM_TOOLCHAIN_PATH)
    set(_llvm_bin "${LLVM_ARM_TOOLCHAIN_PATH}/bin")
    if(NOT EXISTS "${_llvm_bin}")
        message(FATAL_ERROR
            "LLVM_ARM_TOOLCHAIN_PATH is '${LLVM_ARM_TOOLCHAIN_PATH}', but "
            "'${_llvm_bin}' does not exist. It should name the toolchain root "
            "-- the directory containing bin/ -- not bin/ itself.")
    endif()
    set(_find_args NO_DEFAULT_PATH PATHS "${_llvm_bin}")
else()
    set(_find_args)
endif()

find_program(CMAKE_C_COMPILER   NAMES clang        ${_find_args})

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR
        "No clang found for the LLVM Embedded Toolchain for Arm.
"
        "Either unpack it into the repository as
"
        "  external/LLVMEmbeddedToolchainForArm-<version>/
"
        "or point at an install elsewhere, by cache variable or environment:
"
        "  -DLLVM_ARM_TOOLCHAIN_PATH=<the directory containing bin/>
"
        "e2 studio ships this exact toolchain under toolchains/llvm_arm/, if "
        "you already have it installed.")
endif()

find_program(CMAKE_OBJCOPY      NAMES llvm-objcopy ${_find_args} REQUIRED)
find_program(CMAKE_SIZE         NAMES llvm-size    ${_find_args} REQUIRED)
find_program(CMAKE_AR           NAMES llvm-ar      ${_find_args})
find_program(CMAKE_RANLIB       NAMES llvm-ranlib  ${_find_args})

# ---------------------------------------------------------------------------
# Architecture
# ---------------------------------------------------------------------------
# Here rather than on each target: CMake needs these during compiler
# identification and ABI detection, before any target exists. Without them it
# probes as if building for the host and gets pointer size and endianness
# wrong. The rest of the profile comes from cmake/sdk6-sources.cmake.
set(CMAKE_C_FLAGS_INIT "--target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m")

# A compile-and-link test cannot succeed: an image needs the generated linker
# script and the Renesas system library, neither of which exists yet.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")

# ---------------------------------------------------------------------------
# Keep CMake out of the flags
# ---------------------------------------------------------------------------
# An empty build type means CMake contributes no CMAKE_C_FLAGS_<CONFIG>. With
# one set it would append its own -O, -g and -DNDEBUG after the recovered
# profile, overriding the -Oz the memory budget assumes.
set(CMAKE_BUILD_TYPE "" CACHE STRING "Empty on purpose: see toolchain-llvm-arm.cmake" FORCE)

foreach(_config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_C_FLAGS_${_config}_INIT "")
    set(CMAKE_EXE_LINKER_FLAGS_${_config}_INIT "")
endforeach()
unset(_config)

# Programs on the host; headers and libraries only in the target sysroot.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
