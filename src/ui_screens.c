#include "ui_screens.h"
#include "version.h"
#include <stdio.h>
#include <string.h>

// Forward declarations for event handlers
static void network_selected_event(lv_event_t *e);
static void rescan_btn_event(lv_event_t *e);
static void connect_btn_event(lv_event_t *e);
static void cancel_btn_event(lv_event_t *e);
static void show_password_checkbox_event(lv_event_t *e);
static void retry_btn_event(lv_event_t *e);
static void choose_different_btn_event(lv_event_t *e);
static void settings_btn_event(lv_event_t *e);
static void forget_network_event(lv_event_t *e);
static void skip_btn_event(lv_event_t *e);

// Global UI context pointer for event handlers
static ui_context_t *g_ui_ctx = NULL;

// Global widgets that need to be accessed across functions
static lv_obj_t *password_ta = NULL;
static lv_obj_t *status_label = NULL;

// Initialize UI system
void ui_init(ui_context_t *ctx) {
    memset(ctx, 0, sizeof(ui_context_t));
    ctx->current_state = APP_STATE_INIT;
    g_ui_ctx = ctx;
}

// Transition to a new state
void transition_to_state(ui_context_t *ctx, app_state_t new_state) {
    // Delete old screen
    if (ctx->current_screen != NULL) {
        lv_obj_del(ctx->current_screen);
        ctx->current_screen = NULL;
    }

    // Create new screen based on state
    switch (new_state) {
        case APP_STATE_INIT:
            ctx->current_screen = create_splash_screen(ctx);
            break;
        case APP_STATE_WIFI_SCAN:
            ctx->current_screen = create_wifi_scan_screen(ctx);
            break;
        case APP_STATE_WIFI_PASSWORD:
            ctx->current_screen = create_password_screen(ctx);
            break;
        case APP_STATE_WIFI_CONNECTING:
        case APP_STATE_AUTO_CONNECT:
            ctx->current_screen = create_connecting_screen(ctx);
            break;
        case APP_STATE_WIFI_ERROR:
            ctx->current_screen = create_error_screen(ctx);
            break;
        case APP_STATE_MAIN_APP:
            ctx->current_screen = create_main_app_screen(ctx);
            break;
        default:
            return;
    }

    if (ctx->current_screen != NULL) {
        lv_scr_load(ctx->current_screen);
    }

    ctx->current_state = new_state;
}

// Get current state
app_state_t get_current_state(ui_context_t *ctx) {
    return ctx->current_state;
}

// Create splash screen
lv_obj_t* create_splash_screen(ui_context_t *ctx) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Initializing...");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    return screen;
}

// Create WiFi scan screen
lv_obj_t* create_wifi_scan_screen(ui_context_t *ctx) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Select WiFi Network");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Scanning status
    lv_obj_t *scan_label = lv_label_create(screen);

    if (!ctx->scan_state.scan_complete) {
        lv_label_set_text(scan_label, "Scanning...");
        lv_obj_align(scan_label, LV_ALIGN_CENTER, 0, -30);

        // Spinner
        lv_obj_t *spinner = lv_spinner_create(screen);
        lv_obj_set_size(spinner, 60, 60);
        lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 20);

        // Skip button
        lv_obj_t *skip_btn = lv_btn_create(screen);
        lv_obj_set_size(skip_btn, 120, 35);
        lv_obj_align(skip_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_add_event_cb(skip_btn, skip_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *skip_label = lv_label_create(skip_btn);
        lv_label_set_text(skip_label, "Skip");
        lv_obj_center(skip_label);
    } else if (ctx->scan_state.count == 0) {
        lv_label_set_text(scan_label, "No networks found");
        lv_obj_align(scan_label, LV_ALIGN_CENTER, 0, -40);

        // Rescan button
        lv_obj_t *rescan_btn = lv_btn_create(screen);
        lv_obj_set_size(rescan_btn, 150, 40);
        lv_obj_align(rescan_btn, LV_ALIGN_CENTER, 0, 10);
        lv_obj_add_event_cb(rescan_btn, rescan_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *btn_label = lv_label_create(rescan_btn);
        lv_label_set_text(btn_label, "Scan Again");
        lv_obj_center(btn_label);

        // Skip button
        lv_obj_t *skip_btn = lv_btn_create(screen);
        lv_obj_set_size(skip_btn, 120, 35);
        lv_obj_align(skip_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_add_event_cb(skip_btn, skip_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *skip_label = lv_label_create(skip_btn);
        lv_label_set_text(skip_label, "Skip");
        lv_obj_center(skip_label);
    } else {
        // Create dropdown with network list
        lv_obj_t *dropdown = lv_dropdown_create(screen);
        lv_obj_set_width(dropdown, 280);
        lv_obj_align(dropdown, LV_ALIGN_CENTER, 0, -40);

        // Build network list string
        char network_list[MAX_SCAN_RESULTS * 40] = {0};
        for (int i = 0; i < ctx->scan_state.count; i++) {
            if (i > 0) strcat(network_list, "\n");

            char network_entry[40];
            snprintf(network_entry, sizeof(network_entry), "%s (%s)",
                    ctx->scan_state.results[i].ssid,
                    wifi_auth_mode_to_string(ctx->scan_state.results[i].auth_mode));
            strcat(network_list, network_entry);
        }

        lv_dropdown_set_options(dropdown, network_list);
        lv_obj_add_event_cb(dropdown, network_selected_event, LV_EVENT_VALUE_CHANGED, ctx);

        // Store dropdown reference in user data for connect button
        lv_obj_set_user_data(dropdown, ctx);

        // Info label
        lv_obj_t *info = lv_label_create(screen);
        lv_label_set_text_fmt(info, "Found %d networks", ctx->scan_state.count);
        lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 40);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(screen);
        lv_obj_set_size(connect_btn, 120, 40);
        lv_obj_align(connect_btn, LV_ALIGN_CENTER, 0, 20);
        lv_obj_set_user_data(connect_btn, dropdown);
        lv_obj_add_event_cb(connect_btn, network_selected_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "Connect");
        lv_obj_center(connect_label);

        // Rescan button (left side at bottom)
        lv_obj_t *rescan_btn = lv_btn_create(screen);
        lv_obj_set_size(rescan_btn, 100, 35);
        lv_obj_align(rescan_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
        lv_obj_add_event_cb(rescan_btn, rescan_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *btn_label = lv_label_create(rescan_btn);
        lv_label_set_text(btn_label, "Rescan");
        lv_obj_center(btn_label);

        // Skip button (right side at bottom)
        lv_obj_t *skip_btn = lv_btn_create(screen);
        lv_obj_set_size(skip_btn, 100, 35);
        lv_obj_align(skip_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
        lv_obj_add_event_cb(skip_btn, skip_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *skip_label = lv_label_create(skip_btn);
        lv_label_set_text(skip_label, "Skip");
        lv_obj_center(skip_label);
    }

    return screen;
}

// Create password entry screen
lv_obj_t* create_password_screen(ui_context_t *ctx) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Title with SSID
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text_fmt(title, "Connect to:\n%s", ctx->selected_ssid);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Check if network is open (no password required)
    if (ctx->selected_auth == CYW43_AUTH_OPEN) {
        lv_obj_t *open_label = lv_label_create(screen);
        lv_label_set_text(open_label, "Open network\n(no password)");
        lv_obj_align(open_label, LV_ALIGN_CENTER, 0, -30);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(screen);
        lv_obj_set_size(connect_btn, 120, 40);
        lv_obj_align(connect_btn, LV_ALIGN_CENTER, 0, 30);
        lv_obj_add_event_cb(connect_btn, connect_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *btn_label = lv_label_create(connect_btn);
        lv_label_set_text(btn_label, "Connect");
        lv_obj_center(btn_label);
    } else {
        // Password textarea
        password_ta = lv_textarea_create(screen);
        lv_obj_set_size(password_ta, 280, 40);
        lv_obj_align(password_ta, LV_ALIGN_TOP_MID, 0, 60);
        lv_textarea_set_password_mode(password_ta, true);
        lv_textarea_set_one_line(password_ta, true);
        lv_textarea_set_max_length(password_ta, WIFI_PASS_MAX_LEN);
        lv_textarea_set_placeholder_text(password_ta, "Password");

        // Show password checkbox
        lv_obj_t *show_pwd_cb = lv_checkbox_create(screen);
        lv_checkbox_set_text(show_pwd_cb, "Show password");
        lv_obj_align(show_pwd_cb, LV_ALIGN_TOP_MID, 0, 110);
        lv_obj_add_event_cb(show_pwd_cb, show_password_checkbox_event, LV_EVENT_VALUE_CHANGED, NULL);

        // Keyboard
        lv_obj_t *kb = lv_keyboard_create(screen);
        lv_obj_set_size(kb, 320, 130);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_textarea(kb, password_ta);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(screen);
        lv_obj_set_size(connect_btn, 100, 35);
        lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_LEFT, 10, -140);
        lv_obj_add_event_cb(connect_btn, connect_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "Connect");
        lv_obj_center(connect_label);

        // Cancel button
        lv_obj_t *cancel_btn = lv_btn_create(screen);
        lv_obj_set_size(cancel_btn, 100, 35);
        lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -140);
        lv_obj_add_event_cb(cancel_btn, cancel_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *cancel_label = lv_label_create(cancel_btn);
        lv_label_set_text(cancel_label, "Cancel");
        lv_obj_center(cancel_label);
    }

    // Back button for open networks
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 100, 35);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(back_btn, cancel_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);

    return screen;
}

// Create connecting screen
lv_obj_t* create_connecting_screen(ui_context_t *ctx) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    const char *ssid = (ctx->current_state == APP_STATE_AUTO_CONNECT)
                       ? ctx->config.ssid
                       : ctx->selected_ssid;
    lv_label_set_text_fmt(title, "Connecting to:\n%s", ssid);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Spinner
    lv_obj_t *spinner = lv_spinner_create(screen);
    lv_obj_set_size(spinner, 80, 80);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);

    // Status label
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Please wait...");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -50);

    return screen;
}

// Create error screen
lv_obj_t* create_error_screen(ui_context_t *ctx) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Connection Failed");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Error message
    lv_obj_t *error_msg = lv_label_create(screen);
    lv_label_set_text(error_msg, get_error_message(ctx->last_error));
    lv_label_set_long_mode(error_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(error_msg, 280);
    lv_obj_align(error_msg, LV_ALIGN_CENTER, 0, -30);

    // Retry button
    lv_obj_t *retry_btn = lv_btn_create(screen);
    lv_obj_set_size(retry_btn, 130, 40);
    lv_obj_align(retry_btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(retry_btn, retry_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *retry_label = lv_label_create(retry_btn);
    lv_label_set_text(retry_label, "Try Again");
    lv_obj_center(retry_label);

    // Choose different network button
    lv_obj_t *different_btn = lv_btn_create(screen);
    lv_obj_set_size(different_btn, 200, 40);
    lv_obj_align(different_btn, LV_ALIGN_CENTER, 0, 90);
    lv_obj_add_event_cb(different_btn, choose_different_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *different_label = lv_label_create(different_btn);
    lv_label_set_text(different_label, "Choose Different Network");
    lv_obj_center(different_label);

    return screen;
}

// Create main app screen (original UI with settings button added)
lv_obj_t* create_main_app_screen(ui_context_t *ctx) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // Title with version info
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text_fmt(title, "%s", VERSION_STRING);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    // WiFi status indicator
    lv_obj_t *wifi_status = lv_label_create(screen);
    bool is_connected = wifi_is_connected();

    if (is_connected) {
        lv_label_set_text_fmt(wifi_status, "WiFi: %s", ctx->config.ssid);
    } else {
        lv_label_set_text(wifi_status, "WiFi: Disconnected");
    }
    lv_obj_align(wifi_status, LV_ALIGN_TOP_LEFT, 5, 25);

    // Settings button
    lv_obj_t *settings_btn = lv_btn_create(screen);
    lv_obj_set_size(settings_btn, 50, 30);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -5, 20);
    lv_obj_add_event_cb(settings_btn, settings_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, "WiFi");
    lv_obj_center(settings_label);

    // Original output textarea
    lv_obj_t *output_ta = lv_textarea_create(screen);
    lv_obj_set_size(output_ta, 300, 200);
    lv_obj_align(output_ta, LV_ALIGN_TOP_MID, 0, 55);
    lv_textarea_set_text(output_ta, "System ready.\nWiFi connected successfully.\n");

    // Original input textarea
    lv_obj_t *input_ta = lv_textarea_create(screen);
    lv_obj_set_size(input_ta, 300, 40);
    lv_obj_align(input_ta, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_textarea_set_one_line(input_ta, true);
    lv_textarea_set_placeholder_text(input_ta, "Command:");

    return screen;
}

// Update connection status
void update_connection_status(ui_context_t *ctx, const char *status) {
    if (status_label != NULL) {
        lv_label_set_text(status_label, status);
    }
}

// Show error message
void show_error_message(ui_context_t *ctx, error_type_t error) {
    ctx->last_error = error;
    transition_to_state(ctx, APP_STATE_WIFI_ERROR);
}

// Get error message string
const char* get_error_message(error_type_t error) {
    switch (error) {
        case ERROR_SCAN_FAILED:
            return "Failed to scan for networks.\nPlease try again.";
        case ERROR_NO_NETWORKS:
            return "No WiFi networks found.\nCheck if WiFi is enabled.";
        case ERROR_CONNECTION_TIMEOUT:
            return "Connection timed out.\nCheck signal strength.";
        case ERROR_CONNECTION_FAILED:
            return "Failed to connect.\nPlease try again.";
        case ERROR_WRONG_PASSWORD:
            return "Incorrect password.\nPlease check and retry.";
        case ERROR_AUTO_CONNECT_FAILED:
            return "Auto-connect failed.\nPlease reconfigure WiFi.";
        default:
            return "Unknown error occurred.";
    }
}

// Event handler: Network selected from dropdown
static void network_selected_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *dropdown;

    // Check if event came from button (Connect button has dropdown in user_data)
    // or directly from dropdown
    if (lv_obj_check_type(target, &lv_button_class)) {
        // Event from Connect button - get dropdown from button's user_data
        dropdown = (lv_obj_t *)lv_obj_get_user_data(target);
    } else {
        // Event directly from dropdown
        dropdown = target;
    }

    if (dropdown == NULL) {
        return;
    }

    uint16_t selected = lv_dropdown_get_selected(dropdown);

    if (selected < ctx->scan_state.count) {
        // Store selected network info
        strncpy(ctx->selected_ssid, ctx->scan_state.results[selected].ssid, WIFI_SSID_MAX_LEN);
        ctx->selected_ssid[WIFI_SSID_MAX_LEN] = '\0';
        ctx->selected_auth = ctx->scan_state.results[selected].auth_mode;

        // Go to password screen
        transition_to_state(ctx, APP_STATE_WIFI_PASSWORD);
    }
}

// Event handler: Rescan button
static void rescan_btn_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Rescan button clicked\n");

    // Reset scan state and request new scan
    memset(&ctx->scan_state, 0, sizeof(wifi_scan_state_t));
    ctx->scan_requested = true;

    printf("Scan state reset, requesting new scan\n");
    transition_to_state(ctx, APP_STATE_WIFI_SCAN);
}

// Event handler: Connect button
static void connect_btn_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Get password from textarea (if not open network)
    if (ctx->selected_auth != CYW43_AUTH_OPEN && password_ta != NULL) {
        const char *password = lv_textarea_get_text(password_ta);
        strncpy(ctx->config.password, password, WIFI_PASS_MAX_LEN);
        ctx->config.password[WIFI_PASS_MAX_LEN] = '\0';
    } else {
        ctx->config.password[0] = '\0';  // Empty password for open networks
    }

    // Store SSID and convert auth mode from scan format to connection format
    strncpy(ctx->config.ssid, ctx->selected_ssid, WIFI_SSID_MAX_LEN);
    ctx->config.ssid[WIFI_SSID_MAX_LEN] = '\0';

    // Convert scan auth mode (0-8) to proper CYW43_AUTH_* constant
    ctx->config.auth_mode = convert_scan_auth_to_connect_auth(ctx->selected_auth);

    // Go to connecting screen
    transition_to_state(ctx, APP_STATE_WIFI_CONNECTING);

    // Note: Actual connection will be handled in main.c state handler
}

// Event handler: Cancel button
static void cancel_btn_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Clear password for security
    if (password_ta != NULL) {
        lv_textarea_set_text(password_ta, "");
    }

    // Return to scan screen
    transition_to_state(ctx, APP_STATE_WIFI_SCAN);
}

// Event handler: Show password checkbox
static void show_password_checkbox_event(lv_event_t *e) {
    lv_obj_t *checkbox = lv_event_get_target(e);
    bool checked = lv_obj_get_state(checkbox) & LV_STATE_CHECKED;

    if (password_ta != NULL) {
        lv_textarea_set_password_mode(password_ta, !checked);
    }
}

// Event handler: Retry button on error screen
static void retry_btn_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Return to password screen to retry
    transition_to_state(ctx, APP_STATE_WIFI_PASSWORD);
}

// Event handler: Choose different network button
static void choose_different_btn_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Return to scan screen
    transition_to_state(ctx, APP_STATE_WIFI_SCAN);
}

// Event handler: Settings button
static void settings_btn_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Create settings modal using basic message box
    lv_obj_t *mbox = lv_msgbox_create(NULL);
    lv_msgbox_add_title(mbox, "WiFi Settings");
    lv_msgbox_add_text(mbox, "Reconfigure WiFi network?");

    // Add buttons
    lv_msgbox_add_close_button(mbox);
    lv_obj_t *btn_recfg = lv_msgbox_add_footer_button(mbox, "Reconfigure");
    lv_obj_t *btn_cancel = lv_msgbox_add_footer_button(mbox, "Cancel");

    lv_obj_add_event_cb(btn_recfg, forget_network_event, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(btn_cancel, forget_network_event, LV_EVENT_CLICKED, NULL);

    lv_obj_center(mbox);
}

// Event handler: Forget network (reconfigure)
static void forget_network_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (ctx != NULL) {  // Reconfigure button
        // Erase saved config
        wifi_config_erase();

        // Disconnect current WiFi
        wifi_disconnect();

        // Reset scan state
        memset(&ctx->scan_state, 0, sizeof(wifi_scan_state_t));

        // Re-enable STA mode (needed after disconnect disables it)
        cyw43_arch_enable_sta_mode();

        // Request new scan
        ctx->scan_requested = true;

        // Close message box - navigate up to find the msgbox
        lv_obj_t *mbox = btn;
        while (mbox && !lv_obj_check_type(mbox, &lv_msgbox_class)) {
            mbox = lv_obj_get_parent(mbox);
        }
        if (mbox) {
            lv_msgbox_close(mbox);
        }

        transition_to_state(ctx, APP_STATE_WIFI_SCAN);
    } else {  // Cancel button
        // Close message box
        lv_obj_t *mbox = btn;
        while (mbox && !lv_obj_check_type(mbox, &lv_msgbox_class)) {
            mbox = lv_obj_get_parent(mbox);
        }
        if (mbox) {
            lv_msgbox_close(mbox);
        }
    }
}

// Event handler: Skip WiFi connection
static void skip_btn_event(lv_event_t *e) {
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Skipping WiFi connection\n");
    // Skip WiFi setup and go directly to main app
    transition_to_state(ctx, APP_STATE_MAIN_APP);
}
