# PicoCalc LVGL Graphics Demo

A comprehensive demonstration application for the PicoCalc that showcases LVGL graphics engine integration with WiFi configuration, keyboard input, and display management.

## Features

### Core Functionality
- **LVGL Integration**: Full integration of LVGL v9 graphics library
- **Display Support**: SPI display driver for ILI9488 with optimized rendering
- **Keyboard Input**: I2C keyboard support with LVGL input device driver
- **WiFi Management**: Complete WiFi configuration and connection system
- **Multi-Platform**: Support for both Raspberry Pi Pico (RP2040) and Pico 2 (RP2350)

### WiFi Features
- **Network Scanning**: Automatic WiFi network discovery and listing
- **Smart Configuration**: Persistent WiFi credentials stored in flash memory
- **Auto-Connect**: Automatic reconnection to saved networks on boot
- **Security Support**: WPA/WPA2/WPA2-Mixed authentication modes
- **Network Management**: Easy reconfiguration through settings UI
- **Signal Strength**: Networks sorted by RSSI for optimal connection

### User Interface
- **WiFi Setup Flow**: Guided network selection and password entry
- **On-Screen Keyboard**: LVGL keyboard for password input
- **Status Indicators**: Real-time WiFi connection status display
- **Error Handling**: User-friendly error messages and retry options
- **Settings Menu**: Quick access to WiFi reconfiguration

### Technical Features
- **Flash Persistence**: CRC32-validated configuration storage in flash
- **State Machine**: Robust application state management
- **Duplicate Removal**: Intelligent WiFi scan result deduplication
- **Bitmap Flushing**: Optimized display driver with efficient rendering
- **Build Tracking**: Automatic build number increment with date/time stamps

## Project Structure

```
├── main.c                           # Main application entry point
├── ui_screens.c/h                   # UI state machine and screen definitions
├── wifi_config.c/h                  # WiFi management and flash persistence
├── lv_port_disp_picocalc_ILI9488.c/h   # Display driver for ILI9488
├── lv_port_indev_picocalc_kb.c/h    # I2C keyboard input driver
├── lcdspi/                          # LCD SPI communication library
├── i2ckbd/                          # I2C keyboard library
└── lib/lvgl/                        # LVGL graphics library (submodule)
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


#### Special thanks
[Hsuan Han Lai](https://github.com/adwuard) for initial porting and demo project for the PicoCalc 
