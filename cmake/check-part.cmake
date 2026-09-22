# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Verifies CHIP_ID_REG register matches the connected target for the build type.
# Used to refuse to flash the incorrect board.
#
# CHIP_ID_REG register for DA145XX: 0x50003200
# DA14531 value - "2632"
# DA14535 value - "3081"

set(_ids_DA14531 "2632")
set(_ids_DA14535 "3081")

set(_commands "${WORK_DIR}/check-part.jlink")
file(WRITE "${_commands}" "si SWD
speed 2000
device ${PART}
connect
mem32 0x50003200 4
q
")

execute_process(
    COMMAND "${JLINK}" -NoGui 1 -USB ${PROBE} -CommandFile "${_commands}"
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE  _out
    TIMEOUT 60)

if(NOT _out MATCHES "50003200 = ([0-9A-F]+) ([0-9A-F]+) ([0-9A-F]+) ([0-9A-F]+)")
    message(FATAL_ERROR
        "Could not attach to the target over SWD (probe ${PROBE}). Check the "
        "board is powered and the daughterboard is seated. If the SPI flash may "
        "hold an image for the other part, put that part back in and run "
        "erase_spi_nor from its preset.")
endif()

set(_id "")
foreach(_word ${CMAKE_MATCH_1} ${CMAKE_MATCH_2} ${CMAKE_MATCH_3} ${CMAKE_MATCH_4})
    math(EXPR _code "0x${_word} & 0xFF")
    string(ASCII ${_code} _char)
    string(APPEND _id "${_char}")
endforeach()

if(NOT _id STREQUAL "${_ids_${PART}}")
    set(_found "an unrecognised part")
    foreach(_known DA14531 DA14535)
        if(_id STREQUAL "${_ids_${_known}}")
            set(_found "a ${_known}")
        endif()
    endforeach()

    message(FATAL_ERROR
        "This build is for the ${PART}, but the probe is attached to ${_found} "
        "(CHIP_ID \"${_id}\"). Nothing was written. Build and flash with the "
        "matching preset instead.")
endif()

message(STATUS "Attached part: ${PART} (CHIP_ID \"${_id}\")")
