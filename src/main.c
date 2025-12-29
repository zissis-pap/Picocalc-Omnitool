/**
 * PicoCalc Omnitool
 *
 * Implements WiFi configuration, keyboard input and display driver.
 *
 * Author: Zissis Papadopoulos
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lv_conf.h"
#include "lvgl.h"
#include "lv_port_indev_picocalc_kb.h"
#include "lv_port_disp_picocalc_ILI9488.h"
#include "wifi_config.h"
#include "ble_config.h"
#include "ui_screens.h"
#include "ntp_client.h"
#include "psram_helper.h"

const unsigned int LEDPIN = 25;

// SPS data received callback - updates UI with received data
static void sps_data_received(const uint8_t *data, uint16_t length)
{
    // Note: This runs on Core1, need to be careful with shared data
    printf("Received %d bytes via SPS: %.*s\n", length, length, data);

    // TODO: Add to RX textarea in SPS data screen
    // For now, just print to console
}

int main(void)
{
    // Initialize standard I/O
    stdio_init_all();

    // Initialize PSRAM early (before any large allocations)
    if (!psram_init()) {
        printf("WARNING: PSRAM initialization failed!\n");
    }

    // Initialize LED
    gpio_init(LEDPIN);
    gpio_set_dir(LEDPIN, GPIO_OUT);

    // Initialize WiFi
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_GREECE)) 
    {
        printf("failed to initialise\n");
        return 1;
    }
    printf("wi-fi initialised\n");

    // Initialize LVGL
    lv_init();

    // Initialize the custom display driver
    lv_port_disp_init();

    // Initialize the keyboard input device (implementation in lv_port_indev_kbd.c)
    lv_port_indev_init();

    printf("system boot\n");

    // Initialize BLE on Core1
    printf("Launching BLE on Core1...\n");
    ble_init();
    ble_sps_set_data_callback(sps_data_received);
    multicore_launch_core1(ble_core1_entry);
    sleep_ms(100);  // Give Core1 time to initialize
    printf("Core1 launched\n");

    // Initialize UI context
    static ui_context_t ui_ctx;
    ui_init(&ui_ctx);

    // Enable WiFi station mode
    cyw43_arch_enable_sta_mode();

    // Track scan start time for timeout
    static absolute_time_t scan_start_time;

    // Check flash for saved WiFi configuration
    transition_to_state(&ui_ctx, APP_STATE_CHECK_FLASH);

    if (wifi_config_load(&ui_ctx.config)) 
    {
        // Valid config found, try auto-connect
        printf("Found saved WiFi config, attempting auto-connect\n");
        transition_to_state(&ui_ctx, APP_STATE_AUTO_CONNECT);

        // Attempt connection with saved credentials
        if (wifi_connect_blocking(ui_ctx.config.ssid,
                                 ui_ctx.config.password,
                                 ui_ctx.config.auth_mode,
                                 WIFI_CONNECT_TIMEOUT_MS)) 
        {
            printf("Auto-connect successful\n");

            // Initialize and sync time from NTP server
            ntp_client_init();
            ntp_client_request();
            printf("NTP time sync requested\n");

            transition_to_state(&ui_ctx, APP_STATE_MAIN_APP);
        } 
        else 
        {
            printf("Auto-connect failed\n");
            ui_ctx.auto_connect_retry_count++;
            if (ui_ctx.auto_connect_retry_count >= 3) 
            {
                show_error_message(&ui_ctx, ERROR_AUTO_CONNECT_FAILED);
            } 
            else 
            {
                show_error_message(&ui_ctx, ERROR_CONNECTION_FAILED);
            }
        }
    } 
    else 
    {
        // No valid config, start WiFi setup flow
        printf("No saved WiFi config, starting setup\n");
        ui_ctx.scan_requested = true;
        transition_to_state(&ui_ctx, APP_STATE_WIFI_SCAN);
    }

    while (1)
    {
        // Handle state machine
        switch (ui_ctx.current_state) 
        {
            case APP_STATE_WIFI_SCAN:
                // Check if we need to start a new scan
                if (ui_ctx.scan_requested) 
                {
                    printf("Starting WiFi scan (scan_requested=true)...\n");
                    ui_ctx.scan_requested = false;
                    if (!wifi_start_scan(&ui_ctx.scan_state)) 
                    {
                        printf("ERROR: wifi_start_scan failed\n");
                        show_error_message(&ui_ctx, ERROR_SCAN_FAILED);
                    } 
                    else 
                    {
                        printf("Scan started successfully, waiting for results...\n");
                        scan_start_time = get_absolute_time();
                    }
                }

                // Poll WiFi and check if scan is complete
                if (!ui_ctx.scan_state.scan_complete &&
                    !ui_ctx.scan_state.scan_error) 
                    {
                    // Poll the WiFi stack to process scan results
                    cyw43_arch_poll();

                    // Check for timeout (30 seconds)
                    if (absolute_time_diff_us(scan_start_time, get_absolute_time()) > 30000000) 
                    {
                        printf("WiFi scan timeout\n");
                        ui_ctx.scan_state.scan_error = true;
                        show_error_message(&ui_ctx, ERROR_SCAN_FAILED);
                        break;
                    }

                    // Check if scan is still active
                    if (!cyw43_wifi_scan_active(&cyw43_state)) 
                    {
                        // Scan complete, sort results and update UI
                        ui_ctx.scan_state.scan_complete = true;
                        printf("WiFi scan complete, found %d networks\n", ui_ctx.scan_state.count);
                        wifi_sort_scan_results(&ui_ctx.scan_state);
                        printf("Updating screen with scan results...\n");
                        transition_to_state(&ui_ctx, APP_STATE_WIFI_SCAN);
                    }
                }
                break;

            case APP_STATE_WIFI_CONNECTING:
                // Connection attempt is blocking, so this state should transition quickly
                // The actual connection happens when entering this state
                {
                    bool connected = wifi_connect_blocking(ui_ctx.config.ssid,
                                                          ui_ctx.config.password,
                                                          ui_ctx.config.auth_mode,
                                                          WIFI_CONNECT_TIMEOUT_MS);

                    if (connected) 
                    {
                        printf("WiFi connected successfully\n");

                        // Save configuration to flash
                        if (wifi_config_save(&ui_ctx.config))
                        {
                            printf("WiFi config saved to flash\n");
                        }
                        else
                        {
                            printf("Warning: Could not save WiFi config\n");
                        }

                        // Initialize and sync time from NTP server
                        ntp_client_init();
                        ntp_client_request();
                        printf("NTP time sync requested\n");

                        transition_to_state(&ui_ctx, APP_STATE_MAIN_APP);
                    } 
                    else 
                    {
                        printf("WiFi connection failed\n");
                        show_error_message(&ui_ctx, ERROR_CONNECTION_FAILED);
                    }
                }
                break;

            case APP_STATE_MAIN_APP:
                // Check WiFi connection periodically
                static uint32_t last_check = 0;
                static bool was_connected = false;
                uint32_t now = to_ms_since_boot(get_absolute_time());

                if (now - last_check > 10000)
                {  // Check every 10 seconds
                    bool is_connected = wifi_is_connected();

                    // Only report if connection was lost (not if never connected)
                    if (was_connected && !is_connected)
                    {
                        printf("WiFi connection lost\n");
                        // Could implement auto-reconnect here
                    }

                    was_connected = is_connected;
                    last_check = now;
                }
                break;

            case APP_STATE_BLE_SCAN:
                {
                    static absolute_time_t ble_scan_start_time;
                    static int last_device_count = 0;
                    static bool scan_timeout_shown = false;

                    // Check if we need to start a new BLE scan
                    if (ui_ctx.ble_scan_requested)
                    {
                        printf("Starting BLE scan (ble_scan_requested=true)...\n");
                        ui_ctx.ble_scan_requested = false;
                        last_device_count = 0;
                        scan_timeout_shown = false;

                        if (!ble_start_scan())
                        {
                            printf("ERROR: ble_start_scan failed\n");
                            show_error_message(&ui_ctx, ERROR_BLE_SCAN_FAILED);
                        }
                        else
                        {
                            printf("BLE scan started successfully\n");
                            ble_scan_start_time = get_absolute_time();
                        }
                    }

                    // While scanning, check for new devices and timeout
                    if (ble_is_scanning())
                    {
                        // Get current scan results
                        ble_get_scan_results(&ui_ctx.ble_scan_state);

                        // If new devices found, update UI
                        if (ui_ctx.ble_scan_state.count > last_device_count)
                        {
                            last_device_count = ui_ctx.ble_scan_state.count;
                            ble_sort_scan_results(&ui_ctx.ble_scan_state);
                            printf("Found %d BLE devices so far...\n", last_device_count);

                            // Mark as incomplete so UI shows "Scanning..." with device list
                            ui_ctx.ble_scan_state.scan_complete = false;
                            transition_to_state(&ui_ctx, APP_STATE_BLE_SCAN);
                        }

                        // Check for timeout (30 seconds)
                        if (absolute_time_diff_us(ble_scan_start_time, get_absolute_time()) > 30000000)
                        {
                            printf("BLE scan timeout reached\n");
                            ble_stop_scan();
                            ble_get_scan_results(&ui_ctx.ble_scan_state);
                            ble_sort_scan_results(&ui_ctx.ble_scan_state);
                            ui_ctx.ble_scan_state.scan_complete = true;
                            printf("BLE scan complete, found %d devices total\n", ui_ctx.ble_scan_state.count);
                            transition_to_state(&ui_ctx, APP_STATE_BLE_SCAN);
                            scan_timeout_shown = true;
                        }
                    }
                    else if (!scan_timeout_shown && ui_ctx.ble_scan_state.scan_active)
                    {
                        // Scan stopped (not by timeout), get final results
                        ble_get_scan_results(&ui_ctx.ble_scan_state);
                        ble_sort_scan_results(&ui_ctx.ble_scan_state);
                        ui_ctx.ble_scan_state.scan_complete = true;
                        printf("BLE scan stopped, found %d devices\n", ui_ctx.ble_scan_state.count);
                        transition_to_state(&ui_ctx, APP_STATE_BLE_SCAN);
                    }
                }
                break;

            case APP_STATE_BLE_CONNECTING:
                {
                    // Initiate connection
                    bool connected = ble_connect(ui_ctx.selected_ble_address,
                                                ui_ctx.selected_ble_address_type);

                    if (connected)
                    {
                        printf("BLE connection initiated\n");

                        // Wait for connection to complete (with timeout)
                        int timeout = 100;  // 10 seconds
                        while (timeout-- > 0 && !ble_is_connected())
                        {
                            sleep_ms(100);
                        }

                        if (ble_is_connected())
                        {
                            printf("BLE connected successfully\n");

                            // Wait for SPS service discovery
                            timeout = 100;  // 10 seconds
                            while (timeout-- > 0 && !ble_is_sps_ready())
                            {
                                sleep_ms(100);
                            }

                            if (ble_is_sps_ready())
                            {
                                printf("SPS service ready\n");
                                transition_to_state(&ui_ctx, APP_STATE_SPS_DATA);
                            }
                            else
                            {
                                printf("SPS service not found\n");
                                show_error_message(&ui_ctx, ERROR_BLE_NO_SPS_SERVICE);
                            }
                        }
                        else
                        {
                            printf("BLE connection timeout\n");
                            show_error_message(&ui_ctx, ERROR_BLE_CONNECTION_FAILED);
                        }
                    }
                    else
                    {
                        printf("BLE connection failed\n");
                        show_error_message(&ui_ctx, ERROR_BLE_CONNECTION_FAILED);
                    }
                }
                break;

            case APP_STATE_SPS_DATA:
                // SPS data screen active - data is handled by callback
                // Just monitor connection status
                if (!ble_is_connected())
                {
                    printf("BLE connection lost\n");
                    show_error_message(&ui_ctx, ERROR_BLE_CONNECTION_FAILED);
                }
                break;

            default:
                break;
        }

        // LVGL task handler
        lv_timer_handler();
        lv_tick_inc(5); // Increment LVGL tick by 5 milliseconds
        sleep_ms(5); // Sleep for 5 milliseconds
    }
}
