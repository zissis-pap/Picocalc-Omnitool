#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include "lvgl.h"
#include "wifi_config.h"
#include "ble_config.h"

// Application states
typedef enum {
    APP_STATE_INIT,
    APP_STATE_CHECK_FLASH,
    APP_STATE_AUTO_CONNECT,
    APP_STATE_WIFI_SCAN,
    APP_STATE_WIFI_PASSWORD,
    APP_STATE_WIFI_CONNECTING,
    APP_STATE_WIFI_ERROR,
    APP_STATE_MAIN_APP,
    APP_STATE_SETTINGS,
    APP_STATE_BLE_SCAN,
    APP_STATE_BLE_CONNECTING,
    APP_STATE_BLE_CONNECTED,
    APP_STATE_BLE_ERROR,
    APP_STATE_SPS_DATA,
    APP_STATE_NEWS_FEED,
    APP_STATE_TELEGRAM,
    APP_STATE_WEATHER_CITY_SELECT,
    APP_STATE_WEATHER_CUSTOM_INPUT,
    APP_STATE_WEATHER_LOADING,
    APP_STATE_WEATHER_DISPLAY,
    APP_STATE_WEATHER_MAP
} app_state_t;

// Error types
typedef enum {
    ERROR_NONE,
    ERROR_SCAN_FAILED,
    ERROR_NO_NETWORKS,
    ERROR_CONNECTION_TIMEOUT,
    ERROR_CONNECTION_FAILED,
    ERROR_WRONG_PASSWORD,
    ERROR_AUTO_CONNECT_FAILED,
    ERROR_BLE_SCAN_FAILED,
    ERROR_BLE_NO_DEVICES,
    ERROR_BLE_CONNECTION_FAILED,
    ERROR_BLE_NO_SPS_SERVICE,
    ERROR_BLE_DATA_TRANSFER_FAILED
} error_type_t;

// Global UI context
typedef struct {
    app_state_t current_state;
    lv_obj_t *current_screen;

    // WiFi state
    wifi_scan_state_t scan_state;
    wifi_config_t config;
    char selected_ssid[WIFI_SSID_MAX_LEN + 1];
    uint32_t selected_auth;
    int auto_connect_retry_count;
    bool scan_requested;  // Flag to trigger new scan

    // BLE state
    ble_scan_state_t ble_scan_state;
    bd_addr_t selected_ble_address;
    bd_addr_type_t selected_ble_address_type;
    char selected_ble_name[BLE_DEVICE_NAME_MAX_LEN + 1];
    sps_device_type_t selected_sps_type;
    bool ble_scan_requested;

    // Common
    error_type_t last_error;

    // Weather state
    char weather_custom_city[65];
    int selected_city_index;  // -1 = custom, 0-9 = predefined
} ui_context_t;

// Initialize UI system
void ui_init(ui_context_t *ctx);

// State transitions
void transition_to_state(ui_context_t *ctx, app_state_t new_state);
app_state_t get_current_state(ui_context_t *ctx);

// Screen creation functions
lv_obj_t* create_splash_screen(ui_context_t *ctx);
lv_obj_t* create_wifi_scan_screen(ui_context_t *ctx);
lv_obj_t* create_password_screen(ui_context_t *ctx);
lv_obj_t* create_connecting_screen(ui_context_t *ctx);
lv_obj_t* create_error_screen(ui_context_t *ctx);
lv_obj_t* create_main_app_screen(ui_context_t *ctx);
lv_obj_t* create_ble_scan_screen(ui_context_t *ctx);
lv_obj_t* create_ble_connecting_screen(ui_context_t *ctx);
lv_obj_t* create_sps_data_screen(ui_context_t *ctx);
lv_obj_t* create_news_feed_screen(ui_context_t *ctx);
lv_obj_t* create_telegram_screen(ui_context_t *ctx);
lv_obj_t* create_weather_city_select_screen(ui_context_t *ctx);
lv_obj_t* create_weather_custom_input_screen(ui_context_t *ctx);
lv_obj_t* create_weather_loading_screen(ui_context_t *ctx);
lv_obj_t* create_weather_display_screen(ui_context_t *ctx);
lv_obj_t* create_weather_map_screen(ui_context_t *ctx);

// UI update functions
void update_connection_status(ui_context_t *ctx, const char *status);
void show_error_message(ui_context_t *ctx, error_type_t error);

// Get error message string
const char* get_error_message(error_type_t error);

#endif // UI_SCREENS_H
