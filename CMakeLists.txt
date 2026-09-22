# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# A Hubble Terrestrial Network beacon for the Renesas DA145XX
#
# No CMAKE_BUILD_TYPE: the compile profile comes from cmake/sdk6-sources.cmake,
# and a build type would make CMake append its own -O and -g on top.

cmake_minimum_required(VERSION 3.21)

project(hubble_beacon C)

set(SRC_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(CONFIG_DIR "${SRC_DIR}/config")

# ---------------------------------------------------------------------------
# Select Chip
# ---------------------------------------------------------------------------
# One preset per part; everything downstream is derived from this and nothing
# else names a part.
set(DA145XX_PART CACHE STRING "Target part: DA14535 or DA14531")
set_property(CACHE DA145XX_PART PROPERTY STRINGS DA14535 DA14531)

if(NOT DA145XX_PART MATCHES "^DA1453[15]$")
    message(FATAL_ERROR
        "DA145XX_PART is '${DA145XX_PART}'; expected DA14535 or DA14531.")
endif()

string(TOLOWER "${DA145XX_PART}" DA145XX_PART_LOWER)
string(SUBSTRING "${DA145XX_PART}" 4 3 DA145XX_PART_SHORT)

set(PROJECT_NAME "hubble_beacon_${DA145XX_PART_LOWER}")

message(STATUS "Target part: ${DA145XX_PART}")

# ---------------------------------------------------------------------------
# The Renesas SDK6
# ---------------------------------------------------------------------------
# Proprietary and not redistributable, so you supply it. Unpack it under
# external/ and it is found automatically; newest wins.
if(NOT DA145XX_SDK_PATH AND DEFINED ENV{DA145XX_SDK_PATH})
    set(DA145XX_SDK_PATH "$ENV{DA145XX_SDK_PATH}")
endif()

if(NOT DA145XX_SDK_PATH)
    file(GLOB _sdk_candidates
         "${CMAKE_CURRENT_SOURCE_DIR}/external/DA145xx_SDK/*"
         "${CMAKE_CURRENT_SOURCE_DIR}/external/*")
    list(SORT _sdk_candidates)
    list(REVERSE _sdk_candidates)
    foreach(_candidate ${_sdk_candidates})
        if(EXISTS "${_candidate}/sdk/platform/arch/main/arch_main.c")
            set(DA145XX_SDK_PATH "${_candidate}")
            break()
        endif()
    endforeach()
endif()

if(NOT DA145XX_SDK_PATH)
    message(FATAL_ERROR
        "No DA145xx SDK6 found in external/.\n"
        "The SDK is proprietary and cannot be redistributed, so it is neither "
        "vendored here nor a submodule. Download it from Renesas and unpack it "
        "so that this path exists:\n"
        "  external/DA145xx_SDK/<version>/sdk/platform/arch/main/arch_main.c\n"
        "Or point at an install elsewhere:\n"
        "  cmake -DDA145XX_SDK_PATH=C:/Renesas/DA145xx_SDK/6.0.24.1464 ...")
endif()

file(TO_CMAKE_PATH "${DA145XX_SDK_PATH}" DA145XX_SDK_PATH)
string(REGEX REPLACE "/+$" "" DA145XX_SDK_PATH "${DA145XX_SDK_PATH}")

# Reject a wrong-but-plausible path here rather than as a hundred missing
# headers later.
if(NOT EXISTS "${DA145XX_SDK_PATH}/sdk/platform/arch/main/arch_main.c")
    message(FATAL_ERROR
        "DA145XX_SDK_PATH is '${DA145XX_SDK_PATH}', which does not look like an "
        "SDK6 install: sdk/platform/arch/main/arch_main.c is missing.\n"
        "Point it at the versioned directory containing sdk/ and projects/.")
endif()

include(cmake/sdk6-sources.cmake)

get_filename_component(_installed_version "${DA145XX_SDK_PATH}" NAME)
if(NOT _installed_version STREQUAL DA145XX_SDK6_VERSION)
    message(WARNING
        "SDK ${_installed_version} is installed, but cmake/sdk6-sources.cmake "
        "lists the sources of ${DA145XX_SDK6_VERSION}. If the build fails with "
        "missing sources or unresolved symbols, that is the file to correct.")
endif()

message(STATUS "DA145xx SDK6: ${DA145XX_SDK_PATH}")

# Holds da145XX.lib, which ships prebuilt with the SDK.
set(SDK_LIB_DIR "${DA145XX_SDK_PATH}/sdk/platform/system_library/output/Keil_5")

# ---------------------------------------------------------------------------
# The Hubble Device SDK
# ---------------------------------------------------------------------------
# Three core sources, compiled from the submodule in place. The port layer is
# implemented under src/hubble and belongs to this application.
set(HUBBLE_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/hubble-device-sdk")

if(NOT EXISTS "${HUBBLE_SDK_ROOT}/src/hubble.c")
    message(FATAL_ERROR
        "The Hubble Device SDK submodule at external/hubble-device-sdk is empty.\n"
        "Run:  git submodule update --init --recursive")
endif()

set(HUBBLE_SOURCES
    "${HUBBLE_SDK_ROOT}/src/hubble.c"
    "${HUBBLE_SDK_ROOT}/src/hubble_ble.c"
    "${HUBBLE_SDK_ROOT}/src/hubble_crypto.c"
)

# src/ too: the core sources include their own private headers by bare name.
set(HUBBLE_INCLUDES "${HUBBLE_SDK_ROOT}/include" "${HUBBLE_SDK_ROOT}/src")

# -imacros, not -include: it keeps only the macro definitions, so the Hubble
# configuration can be pushed into sources that never asked for it.
set(HUBBLE_OPTIONS "SHELL:-imacros ${CONFIG_DIR}/hubble_config.h")

# ---------------------------------------------------------------------------
# Linker script
# ---------------------------------------------------------------------------
# SDK6 ships a *source* for one. Preprocessing it with this project's config
# headers on the include path makes the memory map a function of the same
# CFG_* macros the firmware compiles against; a stale script against fresh
# firmware links cleanly and corrupts memory at runtime.
#
# Done at configure time so the rewrite below can be plain CMake. The
# CMAKE_CONFIGURE_DEPENDS below re-runs configure -- and so regenerates this
# -- whenever an input changes, which Ninja and Make both trigger from an
# ordinary build.
#
# The `_e2s` suffix is required. The plainly-named ldscript_DA14535.lds.S
# beside it is the GNU-toolchain variant and produces a script LLD rejects.
set(_ldscript_template
    "${DA145XX_SDK_PATH}/sdk/common_project_files/ldscripts/ldscript_${DA145XX_PART}_e2s.lds.S")
set(LDSCRIPT "${CMAKE_CURRENT_BINARY_DIR}/ldscript_${DA145XX_PART}.lds")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${_ldscript_template}"
    "${CONFIG_DIR}/${DA145XX_PART_LOWER}_config_basic.h"
    "${CONFIG_DIR}/${DA145XX_PART_LOWER}_config_advanced.h")

# -E -P: preprocess only, no line markers. -c is what e2 studio passes and is
# inert here; kept so the two command lines stay comparable.
execute_process(
    COMMAND "${CMAKE_C_COMPILER}"
            -I "${CONFIG_DIR}"
            -I "${DA145XX_SDK_PATH}/sdk/common_project_files"
            -D__${DA145XX_PART}__
            -E -P -c "${_ldscript_template}"
    OUTPUT_VARIABLE _lds
    ERROR_VARIABLE  _lds_err
    RESULT_VARIABLE _lds_result
)
if(NOT _lds_result EQUAL 0)
    message(FATAL_ERROR
        "Could not preprocess ${_ldscript_template}:
${_lds_err}")
endif()

# The SDK places four heaps and the ChaCha state by *object file name*
# (`*jump_table.o(heap_env_area)`), which would force every object to be
# called <name>.o -- not CMake's default on a Windows host, and silent when it
# does not match: the heaps fall through to the next section and the firmware
# corrupts retention memory on its first sleep cycle. Each of these sections is
# emitted by exactly one source file, so matching on the section name alone is
# equivalent, and was verified to leave the image byte-identical.
set(_placement_by_objname [[\*[A-Za-z0-9_]+\.o ?\(]])

string(REGEX MATCHALL "${_placement_by_objname}" _placements "${_lds}")
list(LENGTH _placements _placement_count)
if(NOT _placement_count EQUAL 5)
    message(FATAL_ERROR
        "Expected 5 placements by object file name in the generated linker "
        "script, found ${_placement_count}. The memory map is not the one this "
        "build was verified against; re-check before trusting it.")
endif()

string(REGEX REPLACE "${_placement_by_objname}" "*(" _lds "${_lds}")
file(WRITE "${LDSCRIPT}" "${_lds}")

# ---------------------------------------------------------------------------
# Compilation
# ---------------------------------------------------------------------------
# SDK directories first, matching e2 studio. Search order decides which header
# wins for the several names that appear more than once across the SDK.
set(_sdk_includes ${DA145XX_SDK6_INCLUDE_DIRS})
list(TRANSFORM _sdk_includes PREPEND "${DA145XX_SDK_PATH}/")
set(ALL_INCLUDES ${_sdk_includes} "${CONFIG_DIR}" "${SRC_DIR}" ${HUBBLE_INCLUDES})

# SDK sources assume these three are already in every translation unit; they
# carry the CFG_* switches that decide which declarations exist at all. By
# absolute path, so the SDK include directories above cannot shadow them.
set(FORCE_INCLUDES "")
foreach(_header ${DA145XX_PART_LOWER}_config_basic.h ${DA145XX_PART_LOWER}_config_advanced.h user_config.h)
    list(APPEND FORCE_INCLUDES "SHELL:-include ${CONFIG_DIR}/${_header}")
endforeach()

set(SDK_SOURCES ${DA145XX_SDK6_SRCS})
list(TRANSFORM SDK_SOURCES PREPEND "${DA145XX_SDK_PATH}/")

set(SDK_SOURCES_NOLTO ${DA145XX_SDK6_SRCS_NOLTO})
list(TRANSFORM SDK_SOURCES_NOLTO PREPEND "${DA145XX_SDK_PATH}/")

# Three object libraries. app and sdk differ only in that app carries the
# Hubble configuration, which has no business in a Renesas translation unit;
# sdk_nolto is the two sources that opt out of LTO. Nearly the whole image is
# a single LTO unit regardless, so the linker sees three real inputs -- that
# unit, chacha20.o and jump_table.o.
add_library(app OBJECT
    "${SRC_DIR}/platform/user_periph_setup.c"
    "${SRC_DIR}/user_hubble_beacon.c"
    "${SRC_DIR}/hubble/hubble_port.c"
    "${SRC_DIR}/hubble/hubble_crypto_renesas.c"
    ${HUBBLE_SOURCES}
)
target_compile_options(app PRIVATE ${HUBBLE_OPTIONS})

add_library(sdk       OBJECT ${SDK_SOURCES})
add_library(sdk_nolto OBJECT ${SDK_SOURCES_NOLTO})

target_compile_options(sdk_nolto PRIVATE ${DA145XX_SDK6_C_FLAGS_NOLTO})

foreach(_lib app sdk)
    target_compile_options(${_lib} PRIVATE ${DA145XX_SDK6_C_FLAGS})
endforeach()

foreach(_lib app sdk sdk_nolto)
    target_include_directories(${_lib} PRIVATE ${ALL_INCLUDES})
    target_compile_definitions(${_lib} PRIVATE ${DA145XX_SDK6_DEFINES})
    target_compile_options(${_lib} PRIVATE ${FORCE_INCLUDES})
endforeach()

# ---------------------------------------------------------------------------
# The image
# ---------------------------------------------------------------------------
# Object libraries as sources, not target_link_libraries(), because that makes
# no ordering promise and the reference build's section addresses assume this
# order.
add_executable(${PROJECT_NAME}
    $<TARGET_OBJECTS:app>
    $<TARGET_OBJECTS:sdk>
    $<TARGET_OBJECTS:sdk_nolto>
)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${LDSCRIPT}")

target_link_options(${PROJECT_NAME} PRIVATE
    # Through the clang driver, not ld.lld: the driver supplies picolibc and
    # its startup object, which arch_main.c needs. -nostdlib here produces
    # undefined references that look like missing SDK sources.
    #
    # Flags are joined tokens (-Tfile, -Ldir) rather than flag + path, because
    # CMake de-duplicates repeated link options and a bare `-L` written twice
    # loses its second occurrence, stranding the path as a stray argument.
    -flto
    -lcrt0
    "-T${LDSCRIPT}"
    # Holds the symbol files the script pulls in with `INCLUDE
    # da145xx_symbols.lds`; a -L despite containing no libraries.
    "-L${DA145XX_SDK_PATH}/sdk/common_project_files/misc"
    "-L${SDK_LIB_DIR}"
    # da145xx.lib is named twice on purpose: it and the SDK objects each
    # resolve symbols in the other. SHELL: on the second stops CMake
    # de-duplicating it away.
    -Wl,--start-group
    "-l:${DA145XX_PART_LOWER}.lib"
    -Wl,--end-group
    -Wl,-eReset_Handler
    "SHELL:-l:${DA145XX_PART_LOWER}.lib"
    -Wl,--gc-sections
    -Wl,--cref
    # ICF would fold identical functions together, breaking the linker script's
    # placement-by-object-name and the ROM patch table.
    -Wl,--icf=none
    "-Wl,-Map=$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.map"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O binary
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.bin"
    COMMAND "${CMAKE_SIZE}" --format=berkeley "$<TARGET_FILE:${PROJECT_NAME}>"
    VERBATIM
)

# ---------------------------------------------------------------------------
# Flash
# ---------------------------------------------------------------------------
# flash_sysram, flash_spi_nor and erase_spi_nor. See cmake/flash.cmake.
include(cmake/flash.cmake)
