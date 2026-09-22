# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Tool to flash and erase DA145XX firmware. The tool can 'flash_sysram' directly
# into RAM, or 'flash_spi_nor' on Pro Dev Kits that have SPI NOR flash.

# ---------------------------------------------------------------------------
# Locating J-Link
# ---------------------------------------------------------------------------
# If not found automatically, use -DJLINK_PATH=<path>.
file(GLOB _jlink_dirs
     "C:/Program Files/SEGGER/JLink*"
     "C:/Program Files (x86)/SEGGER/JLink*"
     "/Applications/SEGGER/JLink*"
     "/opt/SEGGER/JLink*"
     "/usr/local/SEGGER/JLink*")
list(SORT _jlink_dirs)
list(REVERSE _jlink_dirs)

# J-Link Commander is JLink.exe on Windows and JLinkExe everywhere else. Only
# the platform's own name is searched: a macOS install also has something
# called plain `JLink` that is not Commander, and finding it first fails every
# J-Link step.
if(WIN32)
    set(_jlink_names JLink)
else()
    set(_jlink_names JLinkExe)
endif()

find_program(JLINK_EXECUTABLE NAMES ${_jlink_names}
             HINTS "${JLINK_PATH}" "$ENV{JLINK_PATH}" ${_jlink_dirs})

# ---------------------------------------------------------------------------
# Locating the Toolbox
# ---------------------------------------------------------------------------
# If not found, automatically, use -DSMARTSNIPPETS_PATH=<path>.
file(GLOB _ss_dirs
     "C:/Program Files/SmartSnippetsToolbox/SmartSnippetsToolbox*"
     "C:/Program Files (x86)/SmartSnippetsToolbox/SmartSnippetsToolbox*"
     "/opt/SmartSnippetsToolbox/SmartSnippetsToolbox*"
     "/opt/SmartSnippetsToolbox*"
     "/usr/local/SmartSnippetsToolbox/SmartSnippetsToolbox*"
     "/usr/local/SmartSnippetsToolbox*"
     "$ENV{HOME}/SmartSnippetsToolbox/SmartSnippetsToolbox*"
     "$ENV{HOME}/SmartSnippetsToolbox*")
list(SORT _ss_dirs)
list(REVERSE _ss_dirs)

set(_ss_hints "${SMARTSNIPPETS_PATH}" "$ENV{SMARTSNIPPETS_PATH}" ${_ss_dirs})

find_program(SMARTSNIPPETS_EXECUTABLE
             NAMES SmartSnippetsToolbox
             HINTS ${_ss_hints})

# On Linux the Toolbox is a jar, run with the Java runtime bundled beside it.
if(NOT SMARTSNIPPETS_EXECUTABLE)
    find_file(SMARTSNIPPETS_JAR
              NAMES SmartSnippetsToolbox.jar
              HINTS ${_ss_hints})
    if(SMARTSNIPPETS_JAR)
        get_filename_component(_ss_jar_dir "${SMARTSNIPPETS_JAR}" DIRECTORY)
        find_program(SMARTSNIPPETS_JAVA
                     NAMES java
                     HINTS "${_ss_jar_dir}/jre/bin")
    endif()
endif()

if(SMARTSNIPPETS_EXECUTABLE)
    set(_ss_command "${SMARTSNIPPETS_EXECUTABLE}")
elseif(SMARTSNIPPETS_JAR AND SMARTSNIPPETS_JAVA)
    set(_ss_command "${SMARTSNIPPETS_JAVA}" -jar "${SMARTSNIPPETS_JAR}")
else()
    set(_ss_command "")
endif()

# ---------------------------------------------------------------------------
# The probe
# ---------------------------------------------------------------------------
# The Toolbox needs --jtag <serial> as an input. Find the JTAG serial number.
set(_emus "")
if(JLINK_EXECUTABLE)
    set(_emu_commands "${CMAKE_CURRENT_BINARY_DIR}/emulist.jlink")
    file(WRITE "${_emu_commands}" "ShowEmuList
q
")

    execute_process(
        COMMAND "${JLINK_EXECUTABLE}" -NoGui 1 -CommandFile "${_emu_commands}"
        OUTPUT_VARIABLE _emu_output
        ERROR_VARIABLE  _emu_output
        TIMEOUT 30)

    string(REGEX MATCHALL "Serial number: ([0-9]+)" _emus "${_emu_output}")
    list(TRANSFORM _emus REPLACE "Serial number: " "")
endif()

list(LENGTH _emus _emu_count)
if(_emu_count EQUAL 1)
    message(STATUS "J-Link probe: ${_emus}")
endif()

# ---------------------------------------------------------------------------
# Inputs
# ---------------------------------------------------------------------------
set(_spi_programmer
    "${DA145XX_SDK_PATH}/config/toolbox_resources/DA1453x/common/jtag_programmer_${DA145XX_PART_SHORT}.bin")
set(_sysram_script "${DA145XX_SDK_PATH}/config/DA1453x.JLinkScript")

if(DA145XX_PART STREQUAL "DA14535")
    set(_ss_chip "DA14535-00")
else()
    set(_ss_chip "DA14531")
endif()

set(SPI_FLASH_SIZE "0x40000" CACHE STRING
    "SPI flash size in bytes. The SDK and the Toolbox both assume 256 kB on the DA1453x dev kits.")

# For a board wired away from the default CS P0_1, CLK P0_4, MOSI P0_0,
# MISO P0_3, e.g. -DSPI_FLASH_TOOLBOX_ARGS="-cs;P0_1;-clk;P0_4"
set(SPI_FLASH_TOOLBOX_ARGS "" CACHE STRING
    "Extra arguments appended to the SmartSnippets SPI commands, e.g. pin overrides.")

# ---------------------------------------------------------------------------
# Preconditions
# ---------------------------------------------------------------------------
if(NOT JLINK_EXECUTABLE)
    set(_jlink_blocked
        "J-Link not found. Install SEGGER J-Link, or configure with -DJLINK_PATH=<dir containing the JLink executable>")
elseif(_emu_count EQUAL 0)
    set(_jlink_blocked
        "No J-Link probe found. Attach the board and re-run cmake --preset <part>.")
elseif(NOT _emu_count EQUAL 1)
    string(REPLACE ";" ", " _emu_list "${_emus}")
    set(_jlink_blocked
        "${_emu_count} J-Link probes attached (${_emu_list}); this build flashes one board at a time. Detach the others and re-run cmake --preset <part>.")
else()
    set(_jlink_blocked "")
endif()

if(_jlink_blocked)
    set(_sysram_blocked "${_jlink_blocked}")
elseif(NOT EXISTS "${_sysram_script}")
    set(_sysram_blocked "Missing ${_sysram_script}. It ships with SDK6.")
else()
    set(_sysram_blocked "")
endif()

if(_jlink_blocked)
    set(_spi_blocked "${_jlink_blocked}")
elseif(NOT _ss_command)
    set(_spi_blocked
        "SmartSnippets Toolbox not found. Install it, or configure with -DSMARTSNIPPETS_PATH=<dir containing SmartSnippetsToolbox.exe, or SmartSnippetsToolbox.jar on Linux>. flash_sysram needs only J-Link.")
elseif(NOT EXISTS "${_spi_programmer}")
    set(_spi_blocked "Missing ${_spi_programmer}. It ships with SDK6.")
else()
    set(_spi_blocked "")
endif()

function(_flash_stub name reason)
    add_custom_target(${name}
        COMMAND "${CMAKE_COMMAND}" -E false
        COMMENT "${reason}"
        VERBATIM)
endfunction()

set(_check_part
    "${CMAKE_COMMAND}"
    "-DJLINK=${JLINK_EXECUTABLE}"
    "-DPROBE=${_emus}"
    "-DPART=${DA145XX_PART}"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    -P "${CMAKE_CURRENT_LIST_DIR}/check-part.cmake")

# ---------------------------------------------------------------------------
# flash_sysram
# ---------------------------------------------------------------------------
if(_sysram_blocked)
    _flash_stub(flash_sysram "${_sysram_blocked}")
else()
    set(_sysram_commands "${CMAKE_CURRENT_BINARY_DIR}/load-sysram.jlink")
    file(GENERATE OUTPUT "${_sysram_commands}" CONTENT
"si SWD
speed 2000
device ${DA145XX_PART}
connect
loadfile \"$<TARGET_FILE:${PROJECT_NAME}>\"
r
g
q
")

    add_custom_target(flash_sysram
        COMMAND ${_check_part}
        COMMAND "${JLINK_EXECUTABLE}"
                -NoGui 1
                -ExitOnError 1
                -USB ${_emus}
                -JLinkScriptFile "${_sysram_script}"
                -CommandFile "${_sysram_commands}"
        DEPENDS ${PROJECT_NAME}
        COMMENT "Loading ${PROJECT_NAME}.elf into SysRAM and starting it (lost at power-off)"
        USES_TERMINAL
        VERBATIM)
endif()

if(_spi_blocked)
    _flash_stub(flash_spi_nor "${_spi_blocked}")
    _flash_stub(erase_spi_nor "${_spi_blocked}")
    _flash_stub(flash "${_spi_blocked}")
    _flash_stub(erase "${_spi_blocked}")
    return()
endif()

string(REPLACE ";" " " _ss_command_text "${_ss_command}")
message(STATUS "SmartSnippets Toolbox: ${_ss_command_text}")

# ---------------------------------------------------------------------------
# flash_spi_nor and erase_spi_nor
# ---------------------------------------------------------------------------
set(_ss_common -type spi -chip ${_ss_chip}
               -jtag ${_emus}
               -swclk 2000
               -max ${SPI_FLASH_SIZE}
               -firmware "${_spi_programmer}"
               -y
               ${SPI_FLASH_TOOLBOX_ARGS})

set(_reset_commands "${CMAKE_CURRENT_BINARY_DIR}/boot-from-flash.jlink")
file(GENERATE OUTPUT "${_reset_commands}" CONTENT
"si SWD
speed 2000
device ${DA145XX_PART}
connect
w2 0x50000012, 0x01A0
r
g
q
")

# A whole-chip erase first: -cmd write erases only the sectors it writes.
add_custom_target(flash_spi_nor
    COMMAND ${_check_part}
    COMMAND ${_ss_command} ${_ss_common} -cmd erase
    COMMAND ${_ss_command} ${_ss_common}
            -cmd write -bootable -verify
            -offset 0
            -file "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.bin"
    COMMAND "${JLINK_EXECUTABLE}"
            -NoGui 1
            -ExitOnError 1
            -USB ${_emus}
            -CommandFile "${_reset_commands}"
    DEPENDS ${PROJECT_NAME}
    COMMENT "Writing ${PROJECT_NAME}.bin to SPI flash and booting it"
    USES_TERMINAL
    VERBATIM)

add_custom_target(erase_spi_nor
    COMMAND ${_check_part}
    COMMAND ${_ss_command} ${_ss_common} -cmd erase
    COMMENT "Erasing the SPI flash"
    USES_TERMINAL
    VERBATIM)

add_custom_target(flash)
add_dependencies(flash flash_spi_nor)

add_custom_target(erase)
add_dependencies(erase erase_spi_nor)
