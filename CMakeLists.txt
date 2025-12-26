# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.16)

# Setup board for Pico 1
#set(PICO_BOARD pico)

# Setup board for Pico 2W
set(PICO_BOARD pico2_w)

include(pico_sdk_import.cmake)

project(Picocalc-Omnitool)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Generate build version information
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")

# Read or initialize build number
set(BUILD_NUMBER_FILE "${CMAKE_CURRENT_BINARY_DIR}/build_number.txt")
if(EXISTS ${BUILD_NUMBER_FILE})
    file(READ ${BUILD_NUMBER_FILE} BUILD_NUMBER)
    math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")
else()
    set(BUILD_NUMBER 1)
endif()
file(WRITE ${BUILD_NUMBER_FILE} ${BUILD_NUMBER})

# Configure version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}
)



add_subdirectory(i2ckbd)
add_subdirectory(lcdspi)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/lvgl lvgl)


add_executable(picocalc_omnitool
    src/main.c
    src/wifi_config.c
    src/ui_screens.c
    src/lv_port_indev_picocalc_kb.c
    src/lv_port_disp_picocalc_ILI9488.c
)

target_compile_options(picocalc_omnitool PRIVATE -DPICOMITE
										-DPICO_HEAP_SIZE=0x1000
										-DGUICONTROLS
										-DPICO_CORE0_STACK_SIZE=0x1000
										)

target_compile_definitions(picocalc_omnitool PRIVATE LV_USE_DEMO_WIDGETS=1)

target_include_directories(picocalc_omnitool PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/lib/lvgl
    ${CMAKE_CURRENT_LIST_DIR}/lib/lvgl/src
    ${CMAKE_CURRENT_LIST_DIR}/lib/lvgl/demos
)


target_link_libraries(picocalc_omnitool
  pico_stdlib
  hardware_flash
  hardware_irq
  hardware_adc
  hardware_pwm
  hardware_i2c
  hardware_spi
  hardware_dma
  hardware_exception
  hardware_pio
  pico_multicore
  i2ckbd
  lcdspi
  lvgl
  lvgl_demos
  pico_cyw43_arch_lwip_threadsafe_background
  pico_btstack_ble
)

pico_enable_stdio_usb(picocalc_omnitool 0)
pico_enable_stdio_uart(picocalc_omnitool 1)
pico_add_extra_outputs(picocalc_omnitool)
