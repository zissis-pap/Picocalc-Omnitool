#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include "lvgl/lvgl.h"
#include "wifi_config.h"

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
    APP_STATE_SETTINGS
} app_state_t;

// Error types
typedef enum {
    ERROR_NONE,
    ERROR_SCAN_FAILED,
    ERROR_NO_NETWORKS,
    ERROR_CONNECTION_TIMEOUT,
    ERROR_CONNECTION_FAILED,
    ERROR_WRONG_PASSWORD,
    ERROR_AUTO_CONNECT_FAILED
} error_type_t;

// Global UI context
typedef struct {
    app_state_t current_state;
    lv_obj_t *current_screen;
    wifi_scan_state_t scan_state;
    wifi_config_t config;
    char selected_ssid[WIFI_SSID_MAX_LEN + 1];
    uint32_t selected_auth;
    error_type_t last_error;
    int auto_connect_retry_count;
    bool scan_requested;  // Flag to trigger new scan
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

// UI update functions
void update_connection_status(ui_context_t *ctx, const char *status);
void show_error_message(ui_context_t *ctx, error_type_t error);

// Get error message string
const char* get_error_message(error_type_t error);

#endif // UI_SCREENS_H
