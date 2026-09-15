# Hubble Reference Application — Renesas DA14535

A Hubble Terrestrial Network beacon for the Renesas DA14535, built with CMake
and Ninja and flashed with SEGGER J-Link. No e² studio required.


## Quick start

```sh
git clone https://github.com/HubbleNetwork/hubble-reference-renesas-da

cd hubble-reference-renesas-da

`git submodule update --init --recursive`

# Place the Renesas SDK -- see below. Nothing else to configure.
#   external/DA145xx_SDK/6.0.24.1464/
# or, append '-DDA145XX_SDK_PATH=<path/to/sdk>' to cmake calls below

# Provision your 16-byte Hubble device key
python external/hubble-device-sdk/tools/embed_key_time.py -b master.key -o src/hubble

cmake --preset da14535
cmake --build --preset da14535
cmake --build --preset da14535 --target flash
```

## Limitations

1. This is a RAM image - the flash tool pushes the code directly into RAM. It does not survive a power cycle.
2. The hardware only supports 128-bit encryption keys.
3. You must manually add the Renesas SDK to comply with licensing.


## Requirements

1. Renesas DA14535 board
2. Renesas SDK - DA145xx SDK6 6.0.24.1464
3. Toolchain - LLVM Embedded Toolchain for Arm 17.0.1. [Standalone download](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases) or from e² studio download.
4. Segger J-Link software
5. CMake (version 3.21+), Ninja
6. Python3
7. A 128-bit Hubble key. Default usage: Unix time key.


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

```sh
cmake --preset da14535
cmake --build --preset da14535
cmake --build --preset da14535 --target flash
```

The build prints its section sizes when it finishes.

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

