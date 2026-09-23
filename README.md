# Hubble Reference Application — Renesas DA145XX

A Hubble Terrestrial Network beacon for the Renesas DA145XX, built with CMake
and Ninja and flashed with SEGGER J-Link. No e² studio required.


## Requirements

1. Renesas DA14535 or DA14531 board
2. Renesas SDK - DA145xx SDK6 6.0.24.1464
3. Toolchain - LLVM Embedded Toolchain for Arm 17.0.1. [Standalone download](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases) or from e² studio download. Unpack it under `external/`, or point at it with `-DLLVM_ARM_TOOLCHAIN_PATH=<dir>`.
4. Segger J-Link software
5. [SmartSnippets Toolbox](https://www.renesas.com/en/software-tool/smartbond-development-tools?srsltid=AU7gw4XWXOrhoe4fSAgtBqb_ic3mYhUN0GosSdnRDH0oiT0UQQkhjjwd#overview), if loading firmware to SPI NOR flash.
6. CMake (version 3.21+), Ninja
7. Python3
8. A 128-bit Hubble key. Default usage: Unix time key.

## Quick start

```sh
git clone https://github.com/HubbleNetwork/hubble-reference-renesas-da

cd hubble-reference-renesas-da

git submodule update --init --recursive

# Place the Renesas SDK -- see below. Nothing else to configure.
#   external/DA145xx_SDK/6.0.24.1464/
# or, append '-DDA145XX_SDK_PATH=<path/to/sdk>' to cmake calls below

# Provision your 16-byte Hubble device key
python external/hubble-device-sdk/tools/embed_key_time.py -b master.key -o src/hubble

# Build / flash da14535
cmake --preset da14535
cmake --build --preset da14535
# This 'flash_sysram' target pushes firmware into SysRAM only.
# If using the Pro Dev Kit, you can use 'flash' to update the motherboard's SPI NOR flash.
cmake --build --preset da14535 --target flash_sysram
```

## Limitations

1. The hardware only supports 128-bit encryption keys.
2. You must manually add the Renesas SDK to comply with licensing.
3. There is no SmartSnippets Toolbox download for macOS.


## The Renesas SDK


**You must supply this yourself.** The DA145xx SDK6 is proprietary and licensed to
you rather than sub-licensable by us, so it is neither vendored here nor a
submodule.

Download **DA145xx SDK6 6.0.24.1464** from Renesas and unpack it into
`external/` such that this path exists:
```
external/DA145xx_SDK/6.0.24.1464/sdk/platform/arch/main/arch_main.c
```

Or, to use an SDK installed elsewhere:

```sh
cmake --preset da14535 -DDA145XX_SDK_PATH=<path/to/DA145xx_SDK/6.0.24.1464>
```

Other SDK versions are not guaranteed to be supported at this time.


## Provision the key and the time

```sh
# raw 16-byte key
python external/hubble-device-sdk/tools/embed_key_time.py master.key -o src/hubble

# base64-encoded key
python external/hubble-device-sdk/tools/embed_key_time.py -b master.key -o src/hubble
```

This writes `src/hubble/key.c` and `src/hubble/time.c`, both gitignored and
untracked so a provisioned key cannot reach a commit. Without this step the
build stops at `#error "No device key or provisioned time."`

**Re-run this before every flash.** `time.c` records the moment you ran the
script, and the beacon has no other way to learn the updated time.

> For a product, provision each unit's key into NVDS or OTP and set the time
> from a gateway or host with `hubble_time_set()`. Every board flashed with
> this image is the same Hubble device, stuck at the same starting time.

## Build and flash

This pushes the firmware into SysRAM:

```sh
cmake --preset da14535
cmake --build --preset da14535
cmake --build --preset da14535 --target flash_sysram
```

There are other available flash / erase targets:

| Target | What it does | Requirements |
|---|---|---|
| `erase`, `erase_spi_nor` | Erases the SPI flash | Pro Dev Kit |
| `flash`, `flash_spi_nor` | Erases the SPI flash, writes the image to SPI flash, restarts the board so it boots from flash | Pro Dev Kit |
| `flash_sysram` | Loads the image straight into SysRAM over SWD and starts it |  |

Use `flash_sysram` on a board with no SPI flash, or with its flash on pins
other than the family default below. Keep in mind that if the SPI flash still
holds an image, that image is what the board boots at its next power cycle.

> [!WARNING]
> If using the Pro Dev Kit motherboard, you may need to `erase` before switching
> to a daughterboard of a different chip type. i.e. if you `flash` a DA14531,
> then switch to a DA14535, run `erase` with the DA14531 still connected before
> changing the daughterboard. Otherwise, the daughterboard may not attach
> correctly because it is trying to run code for a different board.


To pin machine-specific paths without touching the repository, write a
`CMakeUserPresets.json` beside `CMakePresets.json` — it is gitignored:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "local",
      "inherits": "da14535",
      "cacheVariables": {
        "DA145XX_SDK_PATH": "path/to/DA145xx_SDK/6.0.24.1464",
        "LLVM_ARM_TOOLCHAIN_PATH": "path/to/e2_studio/toolchains/llvm_arm/LLVMEmbeddedToolchainForArm-17.0.1-Windows-x86_64"
      }
    }
  ],
  "buildPresets": [{ "name": "local", "configurePreset": "local" }]
}
```

In VS Code: open the repository root, install the **CMake Tools** and **C/C++**
extensions, pick the `da14535` preset, build with
<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>B</kbd>.


## Validation

You can validate with a Python tool in the Hubble SDK, or with the Hubble Connect app.

### Python tool

Use the SDK's `scan.py` tool:
1. Install requirements `pip install -r external/hubble-device-sdk/tools/requirements-scan.txt`.
2. Run `python external/hubble-device-sdk/tools/scan.py -k "<Hubble Key>"`.
3. You should see packets appear in your terminal.

### Hubble Connect app

Use the Hubble Connect app while signed in to your account. Your device should appear in "My Devices"
if functioning properly.


## License

This application is Apache-2.0; see [LICENSE](LICENSE).
