# Picocalc-Omnitool

A comprehensive application for the PicoCalc that showcases LVGL graphics engine integration with WiFi configuration, Bluetooth Low Energy (BLE) connectivity, keyboard input, and display management. Optimized for RP2350 (Pico 2W) with DMA-accelerated graphics and dual-core processing.

## Features

### Core Functionality
- **LVGL Integration**: Full integration of LVGL v9 graphics library with input group navigation
- **Display Support**: SPI display driver for ILI9488 with optimized rendering
- **Keyboard Input**: I2C keyboard support with full LVGL navigation (arrow keys, ENTER, TAB, ESC)
- **WiFi Management**: Complete WiFi configuration and connection system
- **BLE Connectivity**: Bluetooth Low Energy scanning and Serial Port Service support
- **Dual-Core Architecture**: Optimized multicore processing (Core0: UI/WiFi, Core1: BLE)
- **Multi-Platform**: Support for both Raspberry Pi Pico (RP2040) and Pico 2 (RP2350)

### WiFi Features
- **Network Scanning**: Automatic WiFi network discovery and listing with rescan support
- **Smart Configuration**: Persistent WiFi credentials stored in flash memory with CRC32 validation
- **Auto-Connect**: Automatic reconnection to saved networks on boot
- **NTP Time Sync**: Automatic network time synchronization on successful WiFi connection
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
- **Application Menu**: Main screen with list of available applications
- **News Feed**: Real-time news headlines via NewsAPI integration
  - Full keyboard navigation with arrow keys (UP/DOWN to scroll, ENTER to open)
  - Complete article descriptions with proper JSON parsing
  - Maintains focus position when closing article popups
  - **News Ticker**: Scrolling news headlines displayed next to clock on main screen
- **Telegram Messaging**: Real-time messaging via Telegram Bot API
  - Send and receive messages through Telegram bot
  - Physical keyboard input with Enter key to send
  - Automatic message polling every 5 seconds
  - Compact UI design with message list and text input
  - Full keyboard navigation support
- **Weather Forecast**: Real-time weather forecasts via OpenWeather API
  - 48-hour weather forecast (today and tomorrow)
  - 10 predefined major cities + custom city input
  - 3-hour interval forecasts with temperature and conditions
  - Weather icons for different conditions (sun, clouds, rain, etc.)
  - HTTPS connection with mbedTLS encryption
  - Multi-screen flow: city selection → loading → forecast display
  - Refresh button for updated forecasts
- **Real-Time Clock**: UTC time display in bottom-right corner (auto-synced via NTP)
- **WiFi Setup Flow**: Guided network selection and password entry
- **BLE Scan Screen**: Dynamic device discovery with real-time updates
- **SPS Data Screen**: Send/receive interface for Serial Port Service communication
- **Keyboard Navigation**: Full keyboard support with arrow keys, ENTER, TAB, and ESC
- **On-Screen Keyboard**: LVGL keyboard for password input (WiFi only)
- **Status Indicators**: Real-time WiFi and BLE connection status display
- **Error Handling**: User-friendly error messages and retry options
- **Dual Access**: Quick access buttons for both WiFi and BLE from main menu

### Technical Features
- **DMA-Accelerated Graphics**: Hardware DMA for SPI transfers (10-15x faster screen updates)
- **Optimized Rendering**: 160-row buffer reduces screen updates from 32 to 2 chunks
- **High-Speed SPI**: 50 MHz SPI interface for maximum throughput
- **NTP Time Sync**: Software-based time tracking using Pico's microsecond timer
- **HTTP/HTTPS Networking**: NewsAPI client (TCP/HTTP), Telegram Bot API (HTTPS), Weather API (HTTPS), and NTP client (UDP) via lwIP
- **TLS/SSL Support**: Secure HTTPS connections for Telegram Bot API and Weather API using mbedTLS
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
│   ├── ble_config.c                 # BLE connectivity and SPS support
│   ├── news_api.c                   # NewsAPI HTTP client for fetching headlines
│   ├── telegram_api.c               # Telegram Bot API HTTPS client for messaging
│   ├── weather_api.c                # OpenWeather API HTTPS client for weather forecasts
│   ├── ntp_client.c                 # NTP client for time synchronization
│   ├── lv_port_disp_picocalc_ILI9488.c  # DMA-accelerated display driver for ILI9488
│   └── lv_port_indev_picocalc_kb.c  # I2C keyboard input driver
├── include/                         # Header files
│   ├── ui_screens.h
│   ├── wifi_config.h
│   ├── ble_config.h
│   ├── news_api.h
│   ├── telegram_api.h
│   ├── weather_api.h
│   ├── ntp_client.h
│   ├── api_tokens.h.template        # Template for API keys and bot tokens
│   ├── lv_port_disp_picocalc_ILI9488.h
│   └── lv_port_indev_picocalc_kb.h
├── lcdspi/                          # LCD SPI communication library (DMA support)
├── i2ckbd/                          # I2C keyboard library
├── lib/lvgl/                        # LVGL graphics library v9.3 (git submodule)
├── version.h.in                     # Version template (auto-generates version.h)
├── lv_conf.h                        # LVGL v9.3 configuration
└── CMakeLists.txt                   # Build configuration
```

## Configuration

### API Keys Setup

The application uses `include/api_tokens.h` for storing API keys and bot tokens. This file is git-ignored to prevent accidentally committing your credentials.

1. If `include/api_tokens.h` doesn't exist, copy it from the template:
   ```bash
   cp include/api_tokens.h.template include/api_tokens.h
   ```

2. Edit `include/api_tokens.h` and configure the services you want to use:

#### NewsAPI Setup (Required for News Feed)

1. Visit [newsapi.org](https://newsapi.org) and sign up for a free account
2. Copy your API key from the dashboard
3. Edit `include/api_tokens.h` and replace `YOUR_API_KEY_HERE`:
   ```c
   #define NEWSAPI_KEY "your_actual_api_key_here"
   ```

**Note**: The free tier allows 100 requests per day, which is sufficient for testing and personal use.

#### Telegram Bot Setup (Required for Telegram Messaging)

1. Open Telegram and search for [@BotFather](https://t.me/botfather)
2. Send `/newbot` and follow the instructions to create a new bot
3. Copy the bot token provided by BotFather
4. Get your Chat ID:
   - Send a message to your bot
   - Visit `https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates`
   - Find the `"chat":{"id":` value in the JSON response
5. Edit `include/api_tokens.h` and add your credentials:
   ```c
   #define TELEGRAM_BOT_TOKEN "your_bot_token_here"
   #define TELEGRAM_CHAT_ID "your_chat_id_here"
   ```

**Note**: The bot token allows your device to send/receive messages. Keep it secure.

#### OpenWeather API Setup (Required for Weather Forecast)

1. Visit [openweathermap.org](https://openweathermap.org/api) and sign up for a free account
2. Navigate to the "API keys" section in your account dashboard
3. Copy your API key (it may take a few hours to activate)
4. Edit `include/api_tokens.h` and replace `YOUR_API_KEY_HERE`:
   ```c
   #define OPENWEATHER_API_KEY "your_actual_api_key_here"
   ```

**Note**: The free tier allows:
- 60 calls per minute
- 1,000,000 calls per month
- Access to current weather and 5-day forecast data
- Sufficient for personal use and testing

5. Rebuild the project after making changes

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

### Version 0.03.0 - Weather Forecast Integration (2025-12-29)

#### New Features
- **Weather Forecast Application**: Real-time weather forecasts via OpenWeather API
  - 48-hour weather forecast with 3-hour intervals
  - Support for today and tomorrow forecasts
  - Current temperature, feels-like temperature, and humidity
  - Weather condition descriptions and icons
  - Multi-screen user flow for optimal UX

#### UI Additions
- **Weather City Selection Screen** (`ui_screens.c`):
  - 10 predefined major cities (New York, London, Paris, Tokyo, Sydney, etc.)
  - "Custom City..." option for manual city entry
  - Scrollable city list with consistent dark theme styling
  - Back button to return to main menu

- **Weather Custom Input Screen**:
  - Text input field with I2C keyboard support
  - Placeholder text: "e.g., Berlin"
  - One-line textarea with max length validation
  - Submit button and Enter key support

- **Weather Loading Screen**:
  - Animated spinner with orange theme
  - Dynamic status messages:
    - "Fetching forecast data..."
    - "Downloading map image..."
  - Real-time state monitoring with 500ms updates
  - Error display with descriptive messages

- **Weather Display Screen**:
  - Current weather summary at top
  - Scrollable forecast list grouped by day ("Today:", "Tomorrow:")
  - Weather icons: `*` (sun), `o` (clouds), `~` (rain), `!` (thunderstorm), `S` (snow), `F` (fog)
  - Temperature in Celsius with degree symbol
  - Refresh button for updated forecasts
  - Back button to return to city selection

- **Main Menu Addition**: "Weather Forecast" button added to application launcher

#### Technical Implementation
- **Weather API Client** (`weather_api.c/h`):
  - HTTPS client using lwIP + mbedTLS for secure communication
  - Non-blocking async architecture with state machine
  - DNS resolution for api.openweathermap.org
  - TLS/SSL connection with proper error handling
  - OpenWeather 5 Day/3 Hour Forecast API integration
  - JSON response parsing for forecast data
  - Support for up to 16 forecast entries (48 hours)
  - Temperature, humidity, weather description extraction
  - Coordinate parsing for future map integration
  - Custom lightweight JSON parser (no external dependencies)
  - Memory management with on-demand allocation
  - Weather icon mapping to simple text symbols

- **State Machine Integration**:
  - Added 4 new app states:
    - `APP_STATE_WEATHER_CITY_SELECT` - City selection
    - `APP_STATE_WEATHER_CUSTOM_INPUT` - Custom city input
    - `APP_STATE_WEATHER_LOADING` - Loading/fetching data
    - `APP_STATE_WEATHER_DISPLAY` - Forecast display
  - Weather update timer for loading screen state monitoring
  - Global widget references for UI updates
  - Proper cleanup on state transitions

- **Configuration**:
  - API key configuration via `api_tokens.h`
  - Template file updated with OpenWeather setup instructions
  - Support for 10 predefined cities with coordinates

- **Data Structures**:
  - `weather_forecast_t`: Individual forecast entry (timestamp, temp, humidity, description, icon)
  - `weather_data_t`: Complete weather data (city, coordinates, forecast array, state, errors)
  - `weather_city_t`: Predefined city with name and coordinates

#### Dependencies
- **lwIP**: TCP/IP networking stack
- **mbedTLS**: TLS/SSL encryption for HTTPS
- **OpenWeather API**: 5 Day/3 Hour Forecast endpoint
- **JSON Parser**: Custom lightweight parser for API responses

#### Build System
- Added `src/weather_api.c` to CMakeLists.txt
- Added `include/weather_api.h` header
- Integrated with existing mbedTLS and lwIP libraries

#### API Compatibility Fixes
- Updated for LVGL v9.x API:
  - `lv_spinner_create()` simplified signature
  - `lv_timer_get_user_data()` for timer user data access
  - Manual list styling (replaced unavailable helper functions)
- Added `<math.h>` for coordinate calculations
- Defined `M_PI` constant for portability

---

### Version 0.02.1 - Telegram UI Refinements (2025-12-28)

#### UI Improvements
- **Optimized Telegram Screen Layout**: Refined message display and input areas
  - Reduced message list height from 220px to 180px for better screen fit
  - Reduced input textarea height from 60px to 50px
  - Repositioned widgets to prevent overflow on 480px displays
  - All widgets now fit properly without overlapping

- **Improved Input Experience**: Enhanced physical keyboard integration
  - Removed on-screen keyboard for cleaner interface
  - Removed send button in favor of Enter key functionality
  - Added `LV_EVENT_KEY` handler with Enter key detection
  - Updated placeholder text: "Type message and press Enter..."
  - Messages send immediately when pressing Enter on physical keyboard

#### Technical Improvements
- **Event Handler Enhancement** (`ui_screens.c`):
  - Added `telegram_input_key_event()` function for proper key press detection
  - Intercepts `LV_KEY_ENTER` and triggers message send
  - Maintains multi-line support while enabling Enter-to-send
  - Removed keyboard navigation reference to non-existent send button

---

### Version 0.02.0 - Telegram Messaging Integration (2025-12-28)

#### New Features
- **Telegram Bot Integration**: Real-time messaging through Telegram Bot API
  - Send and receive messages via HTTPS to Telegram servers
  - Message polling system with 5-second update intervals
  - Displays up to 10 most recent messages with username attribution
  - Message format: "@username: message text" or plain text if no username
  - Connection status indicators (Connecting, Sending, Receiving, Connected, Error)
  - Automatic bot token validation with user-friendly error messages

#### UI Additions
- **Telegram Screen** (`ui_screens.c`):
  - Message list (300x220px) with scrollbar for viewing conversation history
  - Multi-line text input area (300x60px) for composing messages
  - On-screen keyboard for text input
  - Back button to return to main app
  - Send button to transmit messages
  - Status label showing connection state
  - Full keyboard navigation support (TAB, arrow keys)
  - Modern dark theme with orange accents matching app design

- **Main Menu Addition**: Telegram button added to application launcher

#### Technical Implementation
- **Telegram API Client** (`telegram_api.c/h`):
  - HTTPS client using lwIP altcp_tls for secure communication
  - Non-blocking async architecture with state machine
  - DNS resolution for api.telegram.org
  - TLS/SSL connection establishment with certificate validation
  - JSON response parsing for getUpdates and sendMessage endpoints
  - Message history management (stores last 10 messages)
  - Polling system with update_id tracking to prevent duplicates
  - Error handling with descriptive error messages
  - Support for messages up to 512 characters
  - Username extraction (up to 32 characters)

- **State Machine Integration**:
  - Added `APP_STATE_TELEGRAM` to application states
  - Telegram update timer for automatic message polling
  - Global widget references for UI updates
  - Proper cleanup on state transitions

- **Configuration**:
  - Bot token and chat ID configuration via `api_tokens.h`
  - Template file includes placeholder values with instructions

#### Dependencies
- **lwIP altcp_tls**: Secure HTTPS connections
- **mbedTLS**: TLS/SSL encryption (via Pico SDK)
- **JSON Parser**: Custom lightweight parser for Telegram API responses

---

### Version 0.01.1 - News Ticker Enhancement (2025-12-28)

#### New Features
- **Scrolling News Ticker on Main Screen**: Latest news headline scrolls next to the clock
  - Displays first article title from news feed
  - Circular scrolling mode for seamless looping
  - Positioned in bottom-left corner next to time display
  - 190px width to leave space for clock
  - Automatically updates when returning from News Feed
  - Uses same orange theme styling as other UI elements

---

### Version 0.01.0 - UI Rework (2025-12-27)

#### UI Changes
- Complete UI rework and modernization
- Improved screen layouts and styling consistency
- Enhanced user experience across all screens

---

### Version 0.00.10 - Major Bug Fixes and Improvements (2025-12-26)

#### New Features
- **NTP Time Synchronization**: Automatic time sync on WiFi connection
  - NTP client fetches time from pool.ntp.org on successful WiFi connection
  - Software-based time tracking using Pico's microsecond timer
  - Real-time clock display in bottom-right corner of main screen (HH:MM:SS format)
  - Updates every second with UTC time
  - Persists across screen changes (resyncs on WiFi reconnect)

- **News Feed Keyboard Navigation**: Full keyboard control for news browsing
  - **Arrow Keys (UP/DOWN)**: Scroll through news articles in the feed
  - **ENTER**: Open selected article to read full details
  - **TAB**: Switch focus between news list and back button
  - **ESC**: Close article detail popups
  - Automatic focus management - returns to selected article after closing popup
  - News list auto-focused when entering News Feed screen

#### UI Modernization
- **Modern Dark Theme with Orange Accents**: Complete visual redesign across all screens
  - **Color Palette**: Dark backgrounds (0x1a1a1a primary) with orange text hierarchy
    - Bright orange (0xffa726) for titles and emphasis
    - Orange (0xff9800) for body text
    - Soft orange (0xff8a65) for secondary/status text
    - Muted orange (0xcc7a52) for disabled/placeholder text
  - **Typography**: Montserrat font family with proper size hierarchy
    - 14px for titles and headers
    - 12px for body text and buttons (minimum readable size)
    - 10px for small details only
  - **Visual Refinements**:
    - Consistent 5px border radius on all buttons
    - Consistent 4px border radius on inputs, textareas, and dropdowns
    - Improved spacing with 5px/10px/20px padding constants
    - Subtle borders (0x4a4a4a) for better element separation
    - Dark themed dropdowns with styled opened lists
    - Fully themed message boxes and dialogs
  - **Applied to all 10 screens**: Splash, WiFi Scan, Password Entry, Connecting, Error, Main App, BLE Scan, BLE Connecting, SPS Data, and News Feed

- **Enhanced WiFi Settings Dialog**: Reconfiguration dialog matches modern theme
  - Dark background with orange text
  - Consistent button styling and spacing
  - Orange close button (X) instead of default blue

#### UI Changes
- **Real-time clock display** in bottom-right corner of main screen (orange text)
- **Keyboard-navigable News Feed**: Arrow key scrolling with orange focus indicators
- **Improved News Article Display**: Full descriptions shown with orange text on dark background
- **Success/Error States**: Green (0x4caf50) for success, Red (0xf44336) for errors (semantic colors preserved)

#### Technical Additions
- **UI Styling System** (`ui_screens.c`):
  - Centralized theme constants for colors, fonts, and spacing
  - 8 helper functions for consistent component styling:
    - `apply_screen_style()` - Screen backgrounds
    - `apply_title_style()` - Title labels
    - `apply_body_style()` - Body text labels
    - `apply_status_style()` - Status/info labels
    - `apply_button_style()` - Button backgrounds
    - `apply_button_label_style()` - Button text
    - `apply_textarea_style()` - Text input areas
    - `apply_dropdown_style()` - Dropdown menus (both closed and opened states)
  - Font hierarchy using Montserrat 10/12/14px
  - Orange color palette with 3-tier text hierarchy
  - Consistent border radius and spacing across all UI elements

- **NTP Client** (`ntp_client.c/h`):
  - NTP protocol implementation using UDP
  - Software time tracking with `time_us_64()` microsecond timer
  - Automatic sync on WiFi connection (both auto-connect and manual)
  - State machine for sync status (IDLE, REQUESTING, SYNCED, ERROR)
  - Time API with `struct tm` and Unix timestamp support

- **Enhanced JSON Parser** (`news_api.c`):
  - Proper handling of escaped characters in JSON strings (`\"`, `\\`, `\/`)
  - Character-by-character parsing to correctly identify string boundaries
  - Prevents premature truncation when descriptions contain quotes
  - Efficient read/write pointer algorithm for escape sequence cleanup
  - Applied to titles, sources, and descriptions for complete data extraction

- **LVGL Input Group Integration** (`ui_screens.c`):
  - News list added to default keyboard navigation group
  - Automatic focus restoration after popup close
  - Custom event handlers for focus management
  - Prevents unwanted focus jumps to last item

- **LVGL Timer Integration**: Automatic time display updates every second
- **Improved Resource Management**: Centralized cleanup of timers and widget references in `transition_to_state()`

#### Bug Fixes
- **Fixed News Feed unresponsiveness**: Resolved issue where app would hang when pressing Back button
  - Added proper timer cleanup during state transitions
  - Cleared global widget references to prevent dangling pointers
  - Applied fix to all screen transitions for improved stability

- **Fixed incomplete news descriptions**: Articles with quotes or special characters now display completely
  - Previous parser used `strchr()` which stopped at first quote, truncating content
  - Example: `"CEO says \"we're excited\""` would show as `"CEO says \"` (incomplete)
  - Now correctly handles all JSON escape sequences and shows full text

- **Fixed focus restoration in News Feed**: Closing article popup now returns to the same position
  - Previously jumped to last article in list when closing popup
  - Custom close handler stores and restores originally focused article button
  - Maintains scroll position for seamless browsing experience

---

### Version 0.00.9 - 2025-12-26

#### New Features
- **NTP Time Synchronization**: Added automatic time sync on WiFi connection
  - NTP client implementation (see Version 0.00.10 for full details)
  - Real-time clock display on main screen
  - Software-based time tracking

#### Bug Fixes
- **Fixed News Feed unresponsiveness**: App no longer hangs when pressing Back button
  - Proper timer cleanup during state transitions
  - Cleared global widget references

---

### Version 0.00.8 - 2025-12-26

#### New Features
- **News Feed Application**: Added NewsAPI integration to fetch and display news headlines
  - Main screen redesigned with application list interface
  - Removed text input/output fields in favor of modular app structure
  - News Feed displays top headlines with source attribution
  - HTTP client implementation using lwIP TCP/IP stack
  - Simple JSON parser for NewsAPI responses
  - Automatic refresh with status indicators
  - Configurable country and API key support
  - Error handling with user-friendly messages

#### UI Changes
- **Main Screen Redesign**: Replaced text input/output with application launcher
  - "Applications:" section with list of available apps
  - News Feed button as first application
  - Cleaner, more organized interface
  - WiFi and BLE buttons remain in top-right corner

#### Technical Additions
- **NewsAPI Client** (`news_api.c/h`):
  - Non-blocking HTTP requests via lwIP TCP/IP stack
  - DNS resolution with fallback
  - TCP client implementation
  - Simple JSON parser for NewsAPI responses
  - State machine for fetch status
  - Support for up to 10 news articles
- **Enhanced State Machine**: Added `APP_STATE_NEWS_FEED` state
- **LVGL Timer Integration**: Automatic UI updates when news data arrives

---

### Version 0.00.7 - 2025-12-26

#### BLE Features
- **Implemented BLE connectivity with SPS support**
  - Bluetooth Low Energy scanning and device discovery
  - Serial Port Service (SPS) implementation
  - Real-time device discovery during scanning
  - SPS service auto-detection with [SPS] indicator
  - Bi-directional data communication
  - Connection management and error handling
  - Core1 execution for non-blocking BLE operations

#### Technical Implementation
- Added `ble_config.c/h` for BLE functionality
- BLE scan screen with dynamic device updates
- SPS data screen for send/receive operations
- 30-second scan window with automatic timeout
- Support for Nordic UART Service (NUS) and u-blox SPS

---

### Version 0.00.6 - 2025-12-26

#### Documentation
- **Improved README.md**
  - Enhanced feature descriptions
  - Better documentation structure
  - Updated build instructions
  - Added technical details

---

### Version 0.00.5 - 2025-12-26

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

### Version 0.00.4 - 2025-12-26

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

### Version 0.00.3 - 2025-12-26

#### Project Organization
- **Renamed project** from "pico_lvgl_display_demo" to "Picocalc-Omnitool"
- **Reorganized file structure**: Moved source files to `src/` and headers to `include/`
- **Simplified version display**: Main screen now shows only version number (e.g., "v0.00.3") instead of full build info
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

### Version 0.00.2 and earlier
Initial development versions with basic WiFi and LVGL functionality.

