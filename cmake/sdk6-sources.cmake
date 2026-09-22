# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Which Renesas SDK6 sources this image is built from, and with what.
#
# Recovered from a known-good e2 studio build of the SDK's prox_reporter
# example, because which of the SDK's ~4000 files a BLE image needs exists
# nowhere but e2 studio project XML. Everything that example needed and this
# one does not has since been removed.
#
# Both lists are at a fixed point: every remaining source, removed on its own,
# either fails to link or changes the image; every remaining directory is the
# home of a header the build actually opens (`ninja -t deps`). Re-run both
# checks after adding anything.
#
# After changing anything here, check that the generated ldscript_<part>.lds is
# unchanged and that the map still places heap_env_area, heap_msg_area,
# heap_db_area and chacha20_state in the retention regions.
#
# DA145XX_PART and DA145XX_PART_SHORT are set by CMakeLists.txt.

set(DA145XX_SDK6_VERSION "6.0.24.1464")

# DA14535 is depends on DA14531 files. Include
if(DA145XX_PART STREQUAL "DA14535")
    set(DA145XX_SDK6_DEFINES
        __DA14531__
        __DA14535__
    )
elseif(DA145XX_PART STREQUAL "DA14531")
    set(DA145XX_SDK6_DEFINES
        __DA14531__
    )
endif()

# The profile almost every file is built with. CMAKE_BUILD_TYPE must be left
# empty by the toolchain file, or CMake appends its own -O/-g/-DNDEBUG on top of
# these and the image stops matching the reference build.
#
# The architecture flags the reference build also passed here
# (--target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m) are not repeated:
# CMake needs them before any target exists, so the toolchain file puts them in
# CMAKE_C_FLAGS and every command line already carries them.
set(DA145XX_SDK6_C_FLAGS
    -Oz
    -g
    -fomit-frame-pointer
    -flto
    -mthumb
    -fno-exceptions
    -fno-rtti
    -fshort-enums
    -fshort-wchar
)

# The reduced profile, used only by the two sources below that opt out of LTO.
# Relative to DA145XX_SDK6_C_FLAGS it drops:
#     -g -fomit-frame-pointer -flto -mthumb -fshort-enums -fshort-wchar
# -flto is the load-bearing one: jump_table.c holds the ROM jump table, which has
# to survive as a real object at a fixed address.
set(DA145XX_SDK6_C_FLAGS_NOLTO
    -Oz
    -fno-exceptions
    -fno-rtti
)

# SDK include directories
set(DA145XX_SDK6_INCLUDE_DIRS
    sdk/app_modules/api
    sdk/platform/include/CMSIS/5.9.0/CMSIS/Core/Include
    sdk/ble_stack/controller/em
    sdk/ble_stack/controller/llc
    sdk/ble_stack/controller/lld
    sdk/ble_stack/controller/llm
    sdk/ble_stack/ea/api
    sdk/ble_stack/em/api
    sdk/ble_stack/hci/api
    sdk/ble_stack/host/att
    sdk/ble_stack/host/att/attc
    sdk/ble_stack/host/att/attm
    sdk/ble_stack/host/att/atts
    sdk/ble_stack/host/gap
    sdk/ble_stack/host/gap/gapc
    sdk/ble_stack/host/gap/gapm
    sdk/ble_stack/host/gatt
    sdk/ble_stack/host/gatt/gattc
    sdk/ble_stack/host/gatt/gattm
    sdk/ble_stack/host/l2c/l2cc
    sdk/ble_stack/host/smp
    sdk/ble_stack/host/smp/smpc
    sdk/ble_stack/host/smp/smpm
    sdk/ble_stack/profiles
    sdk/ble_stack/rwble
    sdk/ble_stack/rwble_hl
    sdk/common_project_files
    sdk/platform/arch
    sdk/platform/arch/boot
    sdk/platform/arch/compiler
    sdk/platform/arch/ll
    sdk/platform/arch/main
    sdk/platform/core_modules/arch_console
    sdk/platform/core_modules/common/api
    sdk/platform/core_modules/crypto
    sdk/platform/core_modules/dbg/api
    sdk/platform/core_modules/gtl/api
    sdk/platform/core_modules/gtl/src
    sdk/platform/core_modules/h4tl/api
    sdk/platform/core_modules/ke/api
    sdk/platform/core_modules/ke/src
    sdk/platform/core_modules/nvds/api
    sdk/platform/core_modules/rf/api
    sdk/platform/core_modules/rwip/api
    sdk/platform/driver/adc
    sdk/platform/driver/ble
    sdk/platform/driver/dma
    sdk/platform/driver/gpio
    sdk/platform/driver/hw_otpc
    sdk/platform/driver/reg
    sdk/platform/driver/rtc
    sdk/platform/driver/syscntl
    sdk/platform/driver/trng
    sdk/platform/driver/uart
    sdk/platform/driver/wkupct_quadec
    sdk/platform/include
    sdk/platform/system_library/include
    third_party/irng
    third_party/rand
    sdk/platform/utilities/otp_cs
    sdk/platform/utilities/otp_hdr
)

# Everything built with the standard profile, in one list. Order is irrelevant:
# these all compile to a single LTO unit, so the linker sees one input.
set(DA145XX_SDK6_SRCS
    sdk/app_modules/src/app_common/app.c
    sdk/app_modules/src/app_common/app_msg_utils.c
    sdk/app_modules/src/app_common/app_task.c
    sdk/app_modules/src/app_default_hnd/app_default_handlers.c
    sdk/app_modules/src/app_easy/app_easy_msg_utils.c
    sdk/app_modules/src/app_easy/app_easy_timer.c
    sdk/app_modules/src/app_entry/app_entry_point.c
    sdk/ble_stack/profiles/prf.c
    sdk/ble_stack/rwble/rwble.c
    sdk/platform/arch/boot/startup_${DA145XX_PART}.c
    sdk/platform/arch/boot/system_${DA145XX_PART}.c
    sdk/platform/arch/main/arch_main.c
    sdk/platform/arch/main/arch_rom.c
    sdk/platform/arch/main/arch_sleep.c
    sdk/platform/arch/main/arch_system.c
    sdk/platform/arch/main/hardfault_handler.c
    sdk/platform/arch/main/nmi_handler.c
    sdk/platform/core_modules/crypto/aes_api.c
    sdk/platform/core_modules/crypto/aes_cbc.c
    sdk/platform/core_modules/crypto/aes_cmac.c
    sdk/platform/core_modules/nvds/src/nvds.c
    sdk/platform/core_modules/rf/src/ble_arp.c
    sdk/platform/core_modules/rf/src/rf_531.c
    sdk/platform/core_modules/rwip/src/rwip.c
    sdk/platform/driver/adc/adc_531.c
    sdk/platform/driver/gpio/gpio.c
    sdk/platform/driver/hw_otpc/hw_otpc_531.c
    sdk/platform/driver/rtc/rtc.c
    sdk/platform/driver/syscntl/syscntl.c
    sdk/platform/driver/trng/trng.c
    sdk/platform/driver/wkupct_quadec/wkupct_quadec.c
    sdk/platform/system_library/src/${DA145XX_PART}/system_library_${DA145XX_PART_SHORT}.c
    sdk/platform/utilities/otp_cs/otp_cs.c
    sdk/platform/utilities/otp_hdr/otp_hdr.c
)

# The two sources that opt out of LTO, and so reach the linker as real objects.
# They are listed separately because CMake cannot remove a compile option from
# one file, and they link after everything else because the generated linker
# script places their sections by object file name.
set(DA145XX_SDK6_SRCS_NOLTO
    third_party/rand/chacha20.c
    sdk/platform/arch/main/jump_table.c
)
