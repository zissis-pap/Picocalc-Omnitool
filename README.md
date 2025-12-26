# Picocalc-Omnitool

A comprehensive application for the PicoCalc that showcases LVGL graphics engine integration with WiFi configuration, Bluetooth Low Energy (BLE) connectivity, keyboard input, and display management. Optimized for RP2350 (Pico 2W) with DMA-accelerated graphics and dual-core processing.

## Features

### Core Functionality
- **LVGL Integration**: Full integration of LVGL v9 graphics library
- **Display Support**: SPI display driver for ILI9488 with optimized rendering
- **Keyboard Input**: I2C keyboard support with LVGL input device driver
- **WiFi Management**: Complete WiFi configuration and connection system
- **BLE Connectivity**: Bluetooth Low Energy scanning and Serial Port Service support
- **Dual-Core Architecture**: Optimized multicore processing (Core0: UI/WiFi, Core1: BLE)
- **Multi-Platform**: Support for both Raspberry Pi Pico (RP2040) and Pico 2 (RP2350)

### WiFi Features
- **Network Scanning**: Automatic WiFi network discovery and listing with rescan support
- **Smart Configuration**: Persistent WiFi credentials stored in flash memory with CRC32 validation
- **Auto-Connect**: Automatic reconnection to saved networks on boot
- **Security Support**: WPA/WPA2/WPA2-Mixed authentication modes
- **Network Management**: Easy reconfiguration through settings UI
- **Signal Strength**: Networks sorted by RSSI for optimal connection
- **Skip Option**: Ability to skip WiFi setup and proceed to main app without connection

### BLE Features
- **Real-Time Device Discovery**: BLE devices appear dynamically as they're found during scanning
- **SPS Service Support**: Compatible with Nordic UART Service (NUS) and u-blox Serial Port Service
- **Service Auto-Detection**: Automatically identifies SPS-compatible devices with [SPS] indicator
- **Bi-Directional Communication**: Send and receive data via Serial Port Service
- **30-Second Scan Window**: Automatic timeout with continuous device updates
- **User Selection**: Interactive device list with signal strength sorting
- **Connection Management**: Status monitoring, disconnect handling, and error recovery
- **Core1 Execution**: BLE operations run on dedicated core for non-blocking performance

### User Interface
- **WiFi Setup Flow**: Guided network selection and password entry
- **BLE Scan Screen**: Dynamic device discovery with real-time updates
- **SPS Data Screen**: Send/receive interface for Serial Port Service communication
- **On-Screen Keyboard**: LVGL keyboard for password input
- **Status Indicators**: Real-time WiFi and BLE connection status display
- **Error Handling**: User-friendly error messages and retry options
- **Dual Access**: Quick access buttons for both WiFi and BLE from main menu

### Technical Features
- **DMA-Accelerated Graphics**: Hardware DMA for SPI transfers (10-15x faster screen updates)
- **Optimized Rendering**: 160-row buffer reduces screen updates from 32 to 2 chunks
- **High-Speed SPI**: 50 MHz SPI interface for maximum throughput
- **Flash Persistence**: CRC32-validated configuration storage in flash
- **State Machine**: Robust application state management
- **Duplicate Removal**: Intelligent WiFi scan result deduplication
- **Smart Color Conversion**: Batch RGB565→RGB888 conversion for ILI9488 displays
- **Build Tracking**: Automatic build number increment with date/time stamps
- **Performance**: Screen transitions in ~20-40ms, enabling smooth 30+ FPS animations

## Project Structure

```
├── src/                             # Source files
│   ├── main.c                       # Main application entry point
│   ├── ui_screens.c                 # UI state machine and screen definitions
│   ├── wifi_config.c                # WiFi management and flash persistence
│   ├── lv_port_disp_picocalc_ILI9488.c  # DMA-accelerated display driver for ILI9488
│   └── lv_port_indev_picocalc_kb.c  # I2C keyboard input driver
├── include/                         # Header files
│   ├── ui_screens.h
│   ├── wifi_config.h
│   ├── lv_port_disp_picocalc_ILI9488.h
│   └── lv_port_indev_picocalc_kb.h
├── lcdspi/                          # LCD SPI communication library (DMA support)
├── i2ckbd/                          # I2C keyboard library
├── lib/lvgl/                        # LVGL graphics library v9.3 (git submodule)
├── version.h.in                     # Version template (auto-generates version.h)
├── lv_conf.h                        # LVGL v9.3 configuration
└── CMakeLists.txt                   # Build configuration
```

## Build Instructions
```bash
# Clone the LVGL submodule (one-time setup)
git submodule update --init --recursive

# ------------------------------
# Navigate to the demo project directory
cd {path_to_this_demo}

# Create and enter the build directory
mkdir build
cd build

# Set the PICO SDK path
export PICO_SDK_PATH=/path/to/pico-sdk

# Configure and build the project
cmake ..
make
```


## Build for Pico 1(RP2040) or Pico 2(RP2350)
if you are using other pico board, you could select a board type from the `CMakeLists.txt`

```cmake
# Setup board for Pico 1
set(PICO_BOARD pico)

# Setup board for Pico 2W
# set(PICO_BOARD pico2)
```

## How to Upload UF2 

Uploading a UF2 file to the Raspberry Pi Pico on a Linux system is straightforward. Here’s how you can do it:

### Step 1: Prepare Your Raspberry Pi Pico
Enter Bootloader Mode:

- Hold down the BOOTSEL button on your Pico.
- While holding the button, connect the Pico to your Linux PC via USB.
- Release the BOOTSEL button.
- Check If the Pico Is Recognized:

Your Pico should appear as a mass storage device named RPI-RP2.

Verify using the following command:

```bash
lsblk
```

You should see a new device (e.g., /media/$USER/RPI-RP2 or /run/media/$USER/RPI-RP2).

### Step 2: Copy the UF2 File to the Pico
```
cp your_firmware.uf2 /media/$USER/RPI-RP2/
```

### Step 3: Run it
On PicoCalc, the default serial port of the Pico is the USB Type-C port, not its built-in Micro USB port.  
So here is the standard running procedures: 

- Unplug the pico from Micro-USB cable
- Plug the pico via USB Type-C
- Press Power On on Top of the PicoCalc


If your firmware includes serial output, you can monitor it using **minicom** or **screen**:   
```bash
screen /dev/ttyACM0 115200
```

(Replace /dev/ttyACM0 with the correct serial port for your Pico.)  

The serial monitor of **Arduino IDE** is another great choice for PicoCalc serial output on both Linux and Windows.

## Changelog

### Version 0.0.6 - Current Development
- Work in progress

---

### Version 0.0.5 - 2025-12-26

#### Display Performance Optimizations (RP2350)
- **Increased LVGL buffer**: Expanded from 10 rows (6.4 KB) to 160 rows (~102 KB)
  - Reduces screen updates from 32 chunks to 2 chunks per full redraw
  - **~16x fewer screen updates**
- **Implemented DMA-accelerated SPI transfers**:
  - Added 153 KB DMA buffer for batch RGB565→RGB888 conversion
  - Offloaded SPI transfers to DMA hardware (CPU-free transfers)
  - Batch converts entire buffer before transfer (vs pixel-by-pixel)
  - Automatic fallback to non-DMA for buffers >153 KB
  - **~3x faster transfers with CPU freed for rendering**
- **Increased SPI speed**: Boosted from 25 MHz to 50 MHz
  - **2x faster pixel transmission bandwidth**
- **Combined performance gain**: Screen transitions now **~10-15x faster** (~20-40ms vs 300-500ms)
  - Enables smooth 30+ FPS animations
  - Near-instant screen transitions

#### Build System
- **Added hardware_dma library** to CMakeLists.txt for DMA support

#### Technical Improvements
- Optimized RGB565 to RGB888 color conversion (batch vs per-pixel)

#### Known Issues
- If display shows visual artifacts at 50 MHz SPI, reduce to 40 MHz in `lcdspi/lcdspi.h`
- DMA buffer limited to 153 KB (supports up to 160 rows); larger updates use fallback mode

---

### Version 0.0.4 - 2025-12-26

#### WiFi Features
- **Added Skip button**: Users can now skip WiFi configuration and go directly to main app
  - Skip button appears on all WiFi scan screen states (scanning, no networks, networks found)
  - Positioned at bottom of screen for easy access
- **Fixed WiFi rescan issue**: Resolved bug where rescanning didn't return/display networks
  - Added check to prevent starting new scan while one is already active
  - Improved state management and debug logging
  - Enhanced scan completion detection and screen updates

#### Technical Improvements
- Added extensive debug logging for WiFi scan operations
- Improved error handling for WiFi scanning failures
- Enhanced state machine robustness with better state transitions

---

### Version 0.0.3 - 2025-12-26

#### Project Organization
- **Renamed project** from "pico_lvgl_display_demo" to "Picocalc-Omnitool"
- **Reorganized file structure**: Moved source files to `src/` and headers to `include/`
- **Simplified version display**: Main screen now shows only version number (e.g., "v0.0.3") instead of full build info
- **Automated version strings**: `VERSION_STRING` and `FULL_VERSION_STRING` now auto-generate from `APP_VERSION_MAJOR/MINOR/PATCH` defines

#### LVGL & Dependencies
- **Converted LVGL to git submodule**: Replaced manually copied files with proper submodule management
  - Repository: https://github.com/lvgl/lvgl.git
  - Branch: release/v9.3
  - Commit: c033a98afddd65aaafeebea625382a94020fe4a7
- **Updated lv_conf.h**: Replaced v8.3.0 configuration with v9.3.0 template for compatibility
- **Fixed include paths**: Changed from `#include "lvgl/lvgl.h"` to `#include "lvgl.h"` throughout codebase

#### Build System
- **Configured for Ninja**: Build system properly set up for Ninja generator
- **Updated .gitignore**: Comprehensive exclusions for build artifacts and dependencies

#### Bug Fixes
- Fixed LVGL version mismatch between config (v8.3) and library (v9.3)
- Resolved include path errors after LVGL restructuring
- Fixed build system issues with Ninja generator expectations
- Corrected config file placement (kept in root as required by build system)

---

### Version 0.0.2 and earlier
Initial development versions with basic WiFi and LVGL functionality.

## Special thanks
[Hsuan Han Lai](https://github.com/adwuard) for initial porting and demo project for the PicoCalc 
