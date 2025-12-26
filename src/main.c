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
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lv_conf.h"
#include "lvgl.h"
#include "lv_port_indev_picocalc_kb.h"
#include "lv_port_disp_picocalc_ILI9488.h"
#include "wifi_config.h"
#include "ui_screens.h"

const unsigned int LEDPIN = 25;

int main(void)
{
    // Initialize standard I/O
    stdio_init_all();

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
                uint32_t now = to_ms_since_boot(get_absolute_time());
                if (now - last_check > 10000) 
                {  // Check every 10 seconds
                    if (!wifi_is_connected()) 
                    {
                        printf("WiFi connection lost\n");
                        // Could implement auto-reconnect here
                    }
                    last_check = now;
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
