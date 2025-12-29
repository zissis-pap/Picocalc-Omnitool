#include "ui_screens.h"
#include "version.h"
#include "news_api.h"
#include "telegram_api.h"
#include "weather_api.h"
#include "ntp_client.h"
#include "api_tokens.h"
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
static void ble_device_selected_event(lv_event_t *e);
static void ble_rescan_btn_event(lv_event_t *e);
static void ble_connect_btn_event(lv_event_t *e);
static void ble_disconnect_btn_event(lv_event_t *e);
static void ble_send_btn_event(lv_event_t *e);
static void ble_menu_btn_event(lv_event_t *e);
static void ble_back_btn_event(lv_event_t *e);
static void news_feed_btn_event(lv_event_t *e);
static void news_back_btn_event(lv_event_t *e);
static void news_article_clicked_event(lv_event_t *e);
static void telegram_btn_event(lv_event_t *e);
static void telegram_back_btn_event(lv_event_t *e);
static void telegram_send_btn_event(lv_event_t *e);
static void telegram_input_key_event(lv_event_t *e);
static void weather_btn_event(lv_event_t *e);
static void weather_back_btn_event(lv_event_t *e);
static void weather_city_btn_event(lv_event_t *e);
static void weather_custom_btn_event(lv_event_t *e);
static void weather_submit_city_event(lv_event_t *e);
static void weather_input_key_event(lv_event_t *e);
static void weather_refresh_btn_event(lv_event_t *e);
static void weather_view_map_btn_event(lv_event_t *e);
static void weather_update_timer_cb(lv_timer_t *timer);

// Global UI context pointer for event handlers
static ui_context_t *g_ui_ctx = NULL;

// Global widgets that need to be accessed across functions
static lv_obj_t *password_ta = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *sps_rx_textarea = NULL;  // For displaying received SPS data
static lv_obj_t *sps_tx_textarea = NULL;  // For entering data to send via SPS
static lv_obj_t *news_list = NULL;        // For displaying news articles
static lv_obj_t *news_status_label = NULL; // For news loading status
static lv_timer_t *news_update_timer = NULL; // Timer for updating news display
static lv_obj_t *time_label = NULL;       // For displaying current time
static lv_timer_t *time_update_timer = NULL; // Timer for updating time display
static uint8_t news_article_indices[MAX_NEWS_ARTICLES]; // Store article indices for click handlers
static lv_obj_t *news_article_buttons[MAX_NEWS_ARTICLES]; // Store button references for focus restoration
static lv_obj_t *news_ticker_label = NULL;  // For displaying scrolling news title on main screen
static lv_obj_t *telegram_list = NULL;     // For displaying telegram messages
static lv_obj_t *telegram_input_ta = NULL; // For message input
static lv_obj_t *telegram_status_label = NULL; // For telegram status messages
static lv_timer_t *telegram_update_timer = NULL; // Timer for polling updates
static lv_obj_t *weather_city_input_ta = NULL; // For city name input
static lv_obj_t *weather_loading_label = NULL; // For loading status
static lv_obj_t *weather_detail_label = NULL;  // For loading details
static lv_timer_t *weather_update_timer = NULL; // Timer for checking weather state

// ============================================================================
// MODERN THEME STYLING SYSTEM
// ============================================================================

// Color palette - Modern dark theme with orange text
#define THEME_BG_PRIMARY     0x1a1a1a  // Screen background (darkest)
#define THEME_BG_SECONDARY   0x242424  // Container/list background
#define THEME_BG_TERTIARY    0x2d2d2d  // Input/card background
#define THEME_BG_BUTTON      0x3d3d3d  // Button background
#define THEME_BORDER_LIGHT   0x4a4a4a  // Subtle borders
#define THEME_BORDER_NORMAL  0x3d3d3d  // Standard borders
#define THEME_TEXT_PRIMARY   0xffa726  // Titles/headers (bright orange)
#define THEME_TEXT_SECONDARY 0xff9800  // Body text (orange)
#define THEME_TEXT_TERTIARY  0xff8a65  // Status/muted text (soft orange)
#define THEME_TEXT_DISABLED  0xcc7a52  // Placeholder text (muted orange)
#define THEME_ACCENT_SUCCESS 0x4caf50  // Success states (green)
#define THEME_ACCENT_ERROR   0xf44336  // Error states (red)

// Font hierarchy (Montserrat, minimum 12px for body text)
#define FONT_TITLE &lv_font_montserrat_14  // Screen titles
#define FONT_BODY  &lv_font_montserrat_12  // Body text, labels, buttons
#define FONT_SMALL &lv_font_montserrat_10  // Small details (use sparingly)

// Spacing constants
#define PADDING_SMALL  5
#define PADDING_NORMAL 10
#define PADDING_LARGE  20
#define BORDER_THIN    1

// Styling helper functions for consistent appearance across screens
static void apply_screen_style(lv_obj_t *screen) {
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_BG_PRIMARY), 0);
}

static void apply_title_style(lv_obj_t *label) {
    lv_obj_set_style_text_font(label, FONT_TITLE, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_TEXT_PRIMARY), 0);
}

static void apply_body_style(lv_obj_t *label) {
    lv_obj_set_style_text_font(label, FONT_BODY, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_TEXT_SECONDARY), 0);
}

static void apply_status_style(lv_obj_t *label) {
    lv_obj_set_style_text_font(label, FONT_BODY, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_TEXT_TERTIARY), 0);
}

static void apply_button_style(lv_obj_t *btn) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_BG_BUTTON), 0);
    lv_obj_set_style_radius(btn, 5, 0);
}

static void apply_button_label_style(lv_obj_t *label) {
    lv_obj_set_style_text_font(label, FONT_BODY, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_TEXT_SECONDARY), 0);
}

static void apply_textarea_style(lv_obj_t *ta) {
    lv_obj_set_style_bg_color(ta, lv_color_hex(THEME_BG_TERTIARY), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(THEME_BORDER_LIGHT), 0);
    lv_obj_set_style_border_width(ta, BORDER_THIN, 0);
    lv_obj_set_style_radius(ta, 4, 0);
    lv_obj_set_style_text_font(ta, FONT_BODY, 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(THEME_TEXT_SECONDARY), 0);
    lv_obj_set_style_pad_all(ta, PADDING_SMALL, 0);
}

static void apply_dropdown_style(lv_obj_t *dd) {
    // Style the dropdown button (closed state)
    lv_obj_set_style_bg_color(dd, lv_color_hex(THEME_BG_TERTIARY), 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(THEME_BORDER_LIGHT), 0);
    lv_obj_set_style_border_width(dd, BORDER_THIN, 0);
    lv_obj_set_style_radius(dd, 4, 0);
    lv_obj_set_style_text_font(dd, FONT_BODY, 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(THEME_TEXT_SECONDARY), 0);
    lv_obj_set_style_pad_left(dd, PADDING_SMALL, 0);
    lv_obj_set_style_pad_right(dd, PADDING_SMALL, 0);

    // Style the dropdown list (opened state)
    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list != NULL) {
        lv_obj_set_style_bg_color(list, lv_color_hex(THEME_BG_SECONDARY), 0);
        lv_obj_set_style_border_color(list, lv_color_hex(THEME_BORDER_LIGHT), 0);
        lv_obj_set_style_border_width(list, 2, 0);
        lv_obj_set_style_radius(list, 4, 0);
        lv_obj_set_style_text_font(list, FONT_BODY, 0);
        lv_obj_set_style_text_color(list, lv_color_hex(THEME_TEXT_SECONDARY), 0);
        lv_obj_set_style_pad_all(list, PADDING_SMALL, 0);

        // Style selected/highlighted items - orange background with black text
        lv_obj_set_style_bg_color(list, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_SELECTED);
        lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SELECTED);
        lv_obj_set_style_text_color(list, lv_color_hex(0x000000), LV_PART_SELECTED);
    }
}

// Initialize UI system
void ui_init(ui_context_t *ctx) {
    memset(ctx, 0, sizeof(ui_context_t));
    ctx->current_state = APP_STATE_INIT;
    g_ui_ctx = ctx;
}

// Transition to a new state
void transition_to_state(ui_context_t *ctx, app_state_t new_state)
{
    // Clean up timers and global widget references from previous screen
    if (time_update_timer != NULL) {
        lv_timer_del(time_update_timer);
        time_update_timer = NULL;
    }
    if (news_update_timer != NULL) {
        lv_timer_del(news_update_timer);
        news_update_timer = NULL;
    }
    if (telegram_update_timer != NULL) {
        lv_timer_del(telegram_update_timer);
        telegram_update_timer = NULL;
    }

    // Clear global widget references
    password_ta = NULL;
    status_label = NULL;
    sps_rx_textarea = NULL;
    sps_tx_textarea = NULL;
    news_list = NULL;
    news_status_label = NULL;
    time_label = NULL;
    news_ticker_label = NULL;
    telegram_list = NULL;
    telegram_input_ta = NULL;
    telegram_status_label = NULL;
    memset(news_article_buttons, 0, sizeof(news_article_buttons));

    // Delete old screen
    if (ctx->current_screen != NULL)
    {
        lv_obj_del(ctx->current_screen);
        ctx->current_screen = NULL;
    }

    // Create new screen based on state
    switch (new_state)
    {
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
        case APP_STATE_BLE_ERROR:
            ctx->current_screen = create_error_screen(ctx);
            break;
        case APP_STATE_MAIN_APP:
            ctx->current_screen = create_main_app_screen(ctx);
            break;
        case APP_STATE_BLE_SCAN:
            ctx->current_screen = create_ble_scan_screen(ctx);
            break;
        case APP_STATE_BLE_CONNECTING:
            ctx->current_screen = create_ble_connecting_screen(ctx);
            break;
        case APP_STATE_SPS_DATA:
            ctx->current_screen = create_sps_data_screen(ctx);
            break;
        case APP_STATE_NEWS_FEED:
            ctx->current_screen = create_news_feed_screen(ctx);
            break;
        case APP_STATE_TELEGRAM:
            ctx->current_screen = create_telegram_screen(ctx);
            break;
        case APP_STATE_WEATHER_CITY_SELECT:
            ctx->current_screen = create_weather_city_select_screen(ctx);
            break;
        case APP_STATE_WEATHER_CUSTOM_INPUT:
            ctx->current_screen = create_weather_custom_input_screen(ctx);
            break;
        case APP_STATE_WEATHER_LOADING:
            // Clean up previous timer if exists
            if (weather_update_timer != NULL) {
                lv_timer_del(weather_update_timer);
                weather_update_timer = NULL;
            }
            ctx->current_screen = create_weather_loading_screen(ctx);
            break;
        case APP_STATE_WEATHER_DISPLAY:
            ctx->current_screen = create_weather_display_screen(ctx);
            break;
        case APP_STATE_WEATHER_MAP:
            ctx->current_screen = create_weather_map_screen(ctx);
            break;
        default:
            return;
    }

    if (ctx->current_screen != NULL) 
    {
        lv_scr_load(ctx->current_screen);
    }

    ctx->current_state = new_state;
}

// Get current state
app_state_t get_current_state(ui_context_t *ctx) 
{
    return ctx->current_state;
}

// Create splash screen
lv_obj_t* create_splash_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Initializing...");
    apply_title_style(label);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

    // Add spinner for visual feedback
    lv_obj_t *spinner = lv_spinner_create(screen);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_INDICATOR);

    return screen;
}

// Create WiFi scan screen
lv_obj_t* create_wifi_scan_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Select WiFi Network");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // Scanning status
    lv_obj_t *scan_label = lv_label_create(screen);

    if (!ctx->scan_state.scan_complete)
    {
        lv_label_set_text(scan_label, "Scanning...");
        apply_body_style(scan_label);
        lv_obj_align(scan_label, LV_ALIGN_CENTER, 0, -30);

        // Spinner
        lv_obj_t *spinner = lv_spinner_create(screen);
        lv_obj_set_size(spinner, 60, 60);
        lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 20);
        lv_obj_set_style_arc_color(spinner, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_INDICATOR);

        // Skip button
        lv_obj_t *skip_btn = lv_btn_create(screen);
        lv_obj_set_size(skip_btn, 120, 35);
        apply_button_style(skip_btn);
        lv_obj_align(skip_btn, LV_ALIGN_BOTTOM_MID, 0, -PADDING_NORMAL);
        lv_obj_add_event_cb(skip_btn, skip_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *skip_label = lv_label_create(skip_btn);
        lv_label_set_text(skip_label, "Skip");
        apply_button_label_style(skip_label);
        lv_obj_center(skip_label);
    } 
    else if (ctx->scan_state.count == 0)
    {
        lv_label_set_text(scan_label, "No networks found");
        apply_body_style(scan_label);
        lv_obj_align(scan_label, LV_ALIGN_CENTER, 0, -40);

        // Rescan button
        lv_obj_t *rescan_btn = lv_btn_create(screen);
        lv_obj_set_size(rescan_btn, 150, 40);
        apply_button_style(rescan_btn);
        lv_obj_align(rescan_btn, LV_ALIGN_CENTER, 0, 10);
        lv_obj_add_event_cb(rescan_btn, rescan_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *btn_label = lv_label_create(rescan_btn);
        lv_label_set_text(btn_label, "Scan Again");
        apply_button_label_style(btn_label);
        lv_obj_center(btn_label);

        // Skip button
        lv_obj_t *skip_btn = lv_btn_create(screen);
        lv_obj_set_size(skip_btn, 120, 35);
        apply_button_style(skip_btn);
        lv_obj_align(skip_btn, LV_ALIGN_BOTTOM_MID, 0, -PADDING_NORMAL);
        lv_obj_add_event_cb(skip_btn, skip_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *skip_label = lv_label_create(skip_btn);
        lv_label_set_text(skip_label, "Skip");
        apply_button_label_style(skip_label);
        lv_obj_center(skip_label);
    } 
    else 
    {
        // Create dropdown with network list
        lv_obj_t *dropdown = lv_dropdown_create(screen);
        lv_obj_set_width(dropdown, 280);
        apply_dropdown_style(dropdown);
        lv_obj_align(dropdown, LV_ALIGN_CENTER, 0, -40);

        // Build network list string
        char network_list[MAX_SCAN_RESULTS * 40] = {0};
        for (int i = 0; i < ctx->scan_state.count; i++)
        {
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
        apply_status_style(info);
        lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 40);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(screen);
        lv_obj_set_size(connect_btn, 120, 40);
        apply_button_style(connect_btn);
        lv_obj_align(connect_btn, LV_ALIGN_CENTER, 0, 20);
        lv_obj_set_user_data(connect_btn, dropdown);
        lv_obj_add_event_cb(connect_btn, network_selected_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "Connect");
        apply_button_label_style(connect_label);
        lv_obj_center(connect_label);

        // Rescan button (left side at bottom)
        lv_obj_t *rescan_btn = lv_btn_create(screen);
        lv_obj_set_size(rescan_btn, 100, 35);
        apply_button_style(rescan_btn);
        lv_obj_align(rescan_btn, LV_ALIGN_BOTTOM_LEFT, PADDING_NORMAL, -PADDING_NORMAL);
        lv_obj_add_event_cb(rescan_btn, rescan_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *btn_label = lv_label_create(rescan_btn);
        lv_label_set_text(btn_label, "Rescan");
        apply_button_label_style(btn_label);
        lv_obj_center(btn_label);

        // Skip button (right side at bottom)
        lv_obj_t *skip_btn = lv_btn_create(screen);
        lv_obj_set_size(skip_btn, 100, 35);
        apply_button_style(skip_btn);
        lv_obj_align(skip_btn, LV_ALIGN_BOTTOM_RIGHT, -PADDING_NORMAL, -PADDING_NORMAL);
        lv_obj_add_event_cb(skip_btn, skip_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *skip_label = lv_label_create(skip_btn);
        lv_label_set_text(skip_label, "Skip");
        apply_button_label_style(skip_label);
        lv_obj_center(skip_label);
    }

    return screen;
}

// Create password entry screen
lv_obj_t* create_password_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title with SSID
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text_fmt(title, "Connect to:\n%s", ctx->selected_ssid);
    apply_title_style(title);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_NORMAL);

    // Check if network is open (no password required)
    if (ctx->selected_auth == CYW43_AUTH_OPEN)
    {
        lv_obj_t *open_label = lv_label_create(screen);
        lv_label_set_text(open_label, "Open network\n(no password)");
        apply_body_style(open_label);
        lv_obj_set_style_text_align(open_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(open_label, LV_ALIGN_CENTER, 0, -30);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(screen);
        lv_obj_set_size(connect_btn, 120, 40);
        apply_button_style(connect_btn);
        lv_obj_align(connect_btn, LV_ALIGN_CENTER, 0, 30);
        lv_obj_add_event_cb(connect_btn, connect_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *btn_label = lv_label_create(connect_btn);
        lv_label_set_text(btn_label, "Connect");
        apply_button_label_style(btn_label);
        lv_obj_center(btn_label);
    } 
    else 
    {
        // Password textarea
        password_ta = lv_textarea_create(screen);
        lv_obj_set_size(password_ta, 280, 40);
        apply_textarea_style(password_ta);
        lv_obj_align(password_ta, LV_ALIGN_TOP_MID, 0, 60);
        lv_textarea_set_password_mode(password_ta, true);
        lv_textarea_set_one_line(password_ta, true);
        lv_textarea_set_max_length(password_ta, WIFI_PASS_MAX_LEN);
        lv_textarea_set_placeholder_text(password_ta, "Password");

        // Show password checkbox
        lv_obj_t *show_pwd_cb = lv_checkbox_create(screen);
        lv_checkbox_set_text(show_pwd_cb, "Show password");
        apply_body_style(show_pwd_cb);
        lv_obj_align(show_pwd_cb, LV_ALIGN_TOP_MID, 0, 110);
        lv_obj_add_event_cb(show_pwd_cb, show_password_checkbox_event, LV_EVENT_VALUE_CHANGED, NULL);

        // Keyboard
        lv_obj_t *kb = lv_keyboard_create(screen);
        lv_obj_set_size(kb, 320, 130);
        lv_obj_set_style_bg_color(kb, lv_color_hex(THEME_BG_SECONDARY), 0);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_textarea(kb, password_ta);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(screen);
        lv_obj_set_size(connect_btn, 100, 35);
        apply_button_style(connect_btn);
        lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_LEFT, PADDING_NORMAL, -140);
        lv_obj_add_event_cb(connect_btn, connect_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "Connect");
        apply_button_label_style(connect_label);
        lv_obj_center(connect_label);

        // Cancel button
        lv_obj_t *cancel_btn = lv_btn_create(screen);
        lv_obj_set_size(cancel_btn, 100, 35);
        apply_button_style(cancel_btn);
        lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -PADDING_NORMAL, -140);
        lv_obj_add_event_cb(cancel_btn, cancel_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *cancel_label = lv_label_create(cancel_btn);
        lv_label_set_text(cancel_label, "Cancel");
        apply_button_label_style(cancel_label);
        lv_obj_center(cancel_label);
    }

    // Back button for open networks
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 100, 35);
    apply_button_style(back_btn);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -PADDING_NORMAL);
    lv_obj_add_event_cb(back_btn, cancel_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    apply_button_label_style(back_label);
    lv_obj_center(back_label);

    return screen;
}

// Create connecting screen
lv_obj_t* create_connecting_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    const char *ssid = (ctx->current_state == APP_STATE_AUTO_CONNECT)
                       ? ctx->config.ssid
                       : ctx->selected_ssid;
    lv_label_set_text_fmt(title, "Connecting to:\n%s", ssid);
    apply_title_style(title);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_LARGE);

    // Spinner
    lv_obj_t *spinner = lv_spinner_create(screen);
    lv_obj_set_size(spinner, 80, 80);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_INDICATOR);

    // Status label
    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Please wait...");
    apply_status_style(status_label);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -50);

    return screen;
}

// Create error screen
lv_obj_t* create_error_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Connection Failed");
    apply_title_style(title);
    lv_obj_set_style_text_color(title, lv_color_hex(THEME_ACCENT_ERROR), 0); // Red for error
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_LARGE);

    // Error message
    lv_obj_t *error_msg = lv_label_create(screen);
    lv_label_set_text(error_msg, get_error_message(ctx->last_error));
    apply_body_style(error_msg);
    lv_obj_set_style_text_align(error_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(error_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(error_msg, 280);
    lv_obj_align(error_msg, LV_ALIGN_CENTER, 0, -30);

    // Retry button
    lv_obj_t *retry_btn = lv_btn_create(screen);
    lv_obj_set_size(retry_btn, 130, 40);
    apply_button_style(retry_btn);
    lv_obj_align(retry_btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(retry_btn, retry_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *retry_label = lv_label_create(retry_btn);
    lv_label_set_text(retry_label, "Try Again");
    apply_button_label_style(retry_label);
    lv_obj_center(retry_label);

    // Choose different network button
    lv_obj_t *different_btn = lv_btn_create(screen);
    lv_obj_set_size(different_btn, 200, 40);
    apply_button_style(different_btn);
    lv_obj_align(different_btn, LV_ALIGN_CENTER, 0, 90);
    lv_obj_add_event_cb(different_btn, choose_different_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *different_label = lv_label_create(different_btn);
    lv_label_set_text(different_label, "Choose Different Network");
    apply_button_label_style(different_label);
    lv_obj_center(different_label);

    return screen;
}

// Timer callback to update time display
static void time_update_timer_cb(lv_timer_t *timer)
{
    if (time_label == NULL) {
        return;
    }

    struct tm timeinfo;
    if (ntp_client_get_time(&timeinfo)) {
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        lv_label_set_text(time_label, time_str);
    } else {
        lv_label_set_text(time_label, "--:--:--");
    }
}

// Create main app screen (original UI with settings button added)
lv_obj_t* create_main_app_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title with version info
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text_fmt(title, "%s", VERSION_STRING);
    apply_title_style(title);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // WiFi status indicator
    lv_obj_t *wifi_status = lv_label_create(screen);
    bool is_connected = wifi_is_connected();

    if (is_connected)
    {
        lv_label_set_text_fmt(wifi_status, "WiFi: %s", ctx->config.ssid);
        apply_body_style(wifi_status);
        lv_obj_set_style_text_color(wifi_status, lv_color_hex(THEME_ACCENT_SUCCESS), 0); // Green when connected
    }
    else
    {
        lv_label_set_text(wifi_status, "WiFi: Disconnected");
        apply_status_style(wifi_status); // Gray when disconnected
    }
    lv_obj_align(wifi_status, LV_ALIGN_TOP_LEFT, PADDING_SMALL, 25);

    // WiFi Settings button
    lv_obj_t *settings_btn = lv_btn_create(screen);
    lv_obj_set_size(settings_btn, 50, 30);
    apply_button_style(settings_btn);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -60, 20);
    lv_obj_add_event_cb(settings_btn, settings_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, "WiFi");
    apply_button_label_style(settings_label);
    lv_obj_center(settings_label);

    // BLE button
    lv_obj_t *ble_btn = lv_btn_create(screen);
    lv_obj_set_size(ble_btn, 50, 30);
    apply_button_style(ble_btn);
    lv_obj_align(ble_btn, LV_ALIGN_TOP_RIGHT, -PADDING_SMALL, 20);
    lv_obj_add_event_cb(ble_btn, ble_menu_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *ble_label = lv_label_create(ble_btn);
    lv_label_set_text(ble_label, "BLE");
    apply_button_label_style(ble_label);
    lv_obj_center(ble_label);

    // Applications list label
    lv_obj_t *apps_label = lv_label_create(screen);
    lv_label_set_text(apps_label, "Applications:");
    apply_body_style(apps_label);
    lv_obj_align(apps_label, LV_ALIGN_TOP_LEFT, PADDING_NORMAL, 60);

    // News Feed button
    lv_obj_t *news_btn = lv_btn_create(screen);
    lv_obj_set_size(news_btn, 300, 50);
    apply_button_style(news_btn);
    lv_obj_align(news_btn, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_add_event_cb(news_btn, news_feed_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *news_label = lv_label_create(news_btn);
    lv_label_set_text(news_label, "News Feed");
    apply_button_label_style(news_label);
    lv_obj_center(news_label);

    // Telegram button
    lv_obj_t *telegram_btn = lv_btn_create(screen);
    lv_obj_set_size(telegram_btn, 300, 50);
    apply_button_style(telegram_btn);
    lv_obj_align(telegram_btn, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_add_event_cb(telegram_btn, telegram_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *telegram_label = lv_label_create(telegram_btn);
    lv_label_set_text(telegram_label, "Telegram");
    apply_button_label_style(telegram_label);
    lv_obj_center(telegram_label);

    // Weather button
    lv_obj_t *weather_btn = lv_btn_create(screen);
    lv_obj_set_size(weather_btn, 300, 50);
    apply_button_style(weather_btn);
    lv_obj_align(weather_btn, LV_ALIGN_TOP_MID, 0, 210);
    lv_obj_add_event_cb(weather_btn, weather_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *weather_label = lv_label_create(weather_btn);
    lv_label_set_text(weather_label, "Weather Forecast");
    apply_button_label_style(weather_label);
    lv_obj_center(weather_label);

    // Time display in bottom-right corner
    time_label = lv_label_create(screen);
    lv_label_set_text(time_label, "--:--:--");
    apply_status_style(time_label);
    lv_obj_align(time_label, LV_ALIGN_BOTTOM_RIGHT, -PADDING_SMALL, -PADDING_SMALL);

    // News ticker label (scrolling news title next to clock)
    news_ticker_label = lv_label_create(screen);
    apply_status_style(news_ticker_label);
    lv_label_set_long_mode(news_ticker_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(news_ticker_label, 190);  // Leave space for time on the right
    lv_obj_align(news_ticker_label, LV_ALIGN_BOTTOM_LEFT, PADDING_SMALL, -PADDING_SMALL);

    // Check if there's news data available
    news_data_t *news_data = news_api_get_data();
    if (news_data != NULL && news_data->state == NEWS_STATE_SUCCESS && news_data->count > 0) {
        // Display the first article's title
        lv_label_set_text(news_ticker_label, news_data->articles[0].title);
    } else {
        // No news available
        lv_label_set_text(news_ticker_label, "");
    }

    // Start time update timer (update every 1 second)
    if (time_update_timer != NULL) {
        lv_timer_del(time_update_timer);
    }
    time_update_timer = lv_timer_create(time_update_timer_cb, 1000, NULL);

    return screen;
}

// Update connection status
void update_connection_status(ui_context_t *ctx, const char *status) 
{
    if (status_label != NULL) 
    {
        lv_label_set_text(status_label, status);
    }
}

// Show error message
void show_error_message(ui_context_t *ctx, error_type_t error) 
{
    ctx->last_error = error;
    transition_to_state(ctx, APP_STATE_WIFI_ERROR);
}

// Get error message string
const char* get_error_message(error_type_t error)
{
    switch (error)
    {
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
        case ERROR_BLE_SCAN_FAILED:
            return "BLE scan failed.\nPlease try again.";
        case ERROR_BLE_NO_DEVICES:
            return "No BLE devices found.\nCheck if Bluetooth is enabled.";
        case ERROR_BLE_CONNECTION_FAILED:
            return "Failed to connect to device.\nPlease try again.";
        case ERROR_BLE_NO_SPS_SERVICE:
            return "Device does not support SPS.\nPlease select a different device.";
        case ERROR_BLE_DATA_TRANSFER_FAILED:
            return "Data transfer failed.\nCheck connection.";
        default:
            return "Unknown error occurred.";
    }
}

// Event handler: Network selected from dropdown
static void network_selected_event(lv_event_t *e) 
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *dropdown;

    // Check if event came from button (Connect button has dropdown in user_data)
    // or directly from dropdown
    if (lv_obj_check_type(target, &lv_button_class)) 
    {
        // Event from Connect button - get dropdown from button's user_data
        dropdown = (lv_obj_t *)lv_obj_get_user_data(target);
    } 
    else 
    {
        // Event directly from dropdown
        dropdown = target;
    }

    if (dropdown == NULL) 
    {
        return;
    }

    uint16_t selected = lv_dropdown_get_selected(dropdown);

    if (selected < ctx->scan_state.count) 
    {
        // Store selected network info
        strncpy(ctx->selected_ssid, ctx->scan_state.results[selected].ssid, WIFI_SSID_MAX_LEN);
        ctx->selected_ssid[WIFI_SSID_MAX_LEN] = '\0';
        ctx->selected_auth = ctx->scan_state.results[selected].auth_mode;

        // Go to password screen
        transition_to_state(ctx, APP_STATE_WIFI_PASSWORD);
    }
}

// Event handler: Rescan button
static void rescan_btn_event(lv_event_t *e) 
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Rescan button clicked\n");

    // Reset scan state and request new scan
    memset(&ctx->scan_state, 0, sizeof(wifi_scan_state_t));
    ctx->scan_requested = true;

    printf("Scan state reset, requesting new scan\n");
    transition_to_state(ctx, APP_STATE_WIFI_SCAN);
}

// Event handler: Connect button
static void connect_btn_event(lv_event_t *e) 
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Get password from textarea (if not open network)
    if (ctx->selected_auth != CYW43_AUTH_OPEN && password_ta != NULL) 
    {
        const char *password = lv_textarea_get_text(password_ta);
        strncpy(ctx->config.password, password, WIFI_PASS_MAX_LEN);
        ctx->config.password[WIFI_PASS_MAX_LEN] = '\0';
    } 
    else 
    {
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
static void cancel_btn_event(lv_event_t *e) 
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Clear password for security
    if (password_ta != NULL) 
    {
        lv_textarea_set_text(password_ta, "");
    }

    // Return to scan screen
    transition_to_state(ctx, APP_STATE_WIFI_SCAN);
}

// Event handler: Show password checkbox
static void show_password_checkbox_event(lv_event_t *e) 
{
    lv_obj_t *checkbox = lv_event_get_target(e);
    bool checked = lv_obj_get_state(checkbox) & LV_STATE_CHECKED;

    if (password_ta != NULL) 
    {
        lv_textarea_set_password_mode(password_ta, !checked);
    }
}

// Event handler: Retry button on error screen
static void retry_btn_event(lv_event_t *e) 
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Return to password screen to retry
    transition_to_state(ctx, APP_STATE_WIFI_PASSWORD);
}

// Event handler: Choose different network button
static void choose_different_btn_event(lv_event_t *e) 
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Return to scan screen
    transition_to_state(ctx, APP_STATE_WIFI_SCAN);
}

// Event handler: Settings button
static void settings_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    // Create settings modal using basic message box
    lv_obj_t *mbox = lv_msgbox_create(NULL);

    // Style the message box with modern dark theme
    lv_obj_set_style_bg_color(mbox, lv_color_hex(THEME_BG_TERTIARY), 0);
    lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(mbox, lv_color_hex(THEME_BORDER_LIGHT), 0);
    lv_obj_set_style_border_width(mbox, 2, 0);
    lv_obj_set_style_radius(mbox, 6, 0);

    lv_obj_t *title = lv_msgbox_add_title(mbox, "WiFi Settings");
    if (title != NULL) {
        lv_obj_set_style_text_font(title, FONT_TITLE, 0);
        lv_obj_set_style_text_color(title, lv_color_hex(THEME_TEXT_PRIMARY), 0);
    }

    // Style the header (title container) AFTER adding title
    lv_obj_t *header = lv_msgbox_get_header(mbox);
    if (header != NULL) {
        lv_obj_set_style_bg_color(header, lv_color_hex(THEME_BG_TERTIARY), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_pad_all(header, PADDING_SMALL, 0);
        lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
        lv_obj_set_style_border_color(header, lv_color_hex(THEME_BG_TERTIARY), LV_PART_MAIN);
        lv_obj_set_style_outline_width(header, 0, 0);
    }

    // Add and style close button
    lv_obj_t *close_btn = lv_msgbox_add_close_button(mbox);
    if (close_btn != NULL) {
        apply_button_style(close_btn);
        // Style the X label inside the close button
        lv_obj_t *close_label = lv_obj_get_child(close_btn, 0);
        if (close_label != NULL) {
            lv_obj_set_style_text_color(close_label, lv_color_hex(THEME_TEXT_SECONDARY), 0);
        }
    }

    // Style the content area
    lv_obj_t *content = lv_msgbox_get_content(mbox);
    if (content != NULL) {
        lv_obj_set_style_bg_color(content, lv_color_hex(THEME_BG_TERTIARY), 0);
    }

    lv_obj_t *text = lv_msgbox_add_text(mbox, "Reconfigure WiFi network?");
    if (text != NULL) {
        lv_obj_set_style_text_font(text, FONT_BODY, 0);
        lv_obj_set_style_text_color(text, lv_color_hex(THEME_TEXT_SECONDARY), 0);
    }

    // Add footer buttons
    lv_obj_t *btn_recfg = lv_msgbox_add_footer_button(mbox, "Reconfigure");
    lv_obj_t *btn_cancel = lv_msgbox_add_footer_button(mbox, "Cancel");

    // Style the footer area
    lv_obj_t *footer = lv_msgbox_get_footer(mbox);
    if (footer != NULL) {
        lv_obj_set_style_bg_color(footer, lv_color_hex(THEME_BG_TERTIARY), 0);
    }

    // Style the footer buttons
    if (btn_recfg != NULL) {
        apply_button_style(btn_recfg);
        lv_obj_t *label = lv_obj_get_child(btn_recfg, 0);
        if (label != NULL) {
            apply_button_label_style(label);
        }
    }
    if (btn_cancel != NULL) {
        apply_button_style(btn_cancel);
        lv_obj_t *label = lv_obj_get_child(btn_cancel, 0);
        if (label != NULL) {
            apply_button_label_style(label);
        }
    }

    lv_obj_add_event_cb(btn_recfg, forget_network_event, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(btn_cancel, forget_network_event, LV_EVENT_CLICKED, NULL);

    lv_obj_center(mbox);
}

// Event handler: Forget network (reconfigure)
static void forget_network_event(lv_event_t *e) 
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (ctx != NULL) 
    {  // Reconfigure button
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
        while (mbox && !lv_obj_check_type(mbox, &lv_msgbox_class)) 
        {
            mbox = lv_obj_get_parent(mbox);
        }
        if (mbox) 
        {
            lv_msgbox_close(mbox);
        }

        transition_to_state(ctx, APP_STATE_WIFI_SCAN);
    } 
    else 
    {  // Cancel button
        // Close message box
        lv_obj_t *mbox = btn;
        while (mbox && !lv_obj_check_type(mbox, &lv_msgbox_class)) 
        {
            mbox = lv_obj_get_parent(mbox);
        }
        if (mbox) 
        {
            lv_msgbox_close(mbox);
        }
    }
}

// Event handler: Skip WiFi connection
static void skip_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Skipping WiFi connection\n");
    // Skip WiFi setup and go directly to main app
    transition_to_state(ctx, APP_STATE_MAIN_APP);
}

// =============================================================================
// BLE Screen Implementations
// =============================================================================

// Create BLE scan screen
lv_obj_t* create_ble_scan_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Select BLE Device");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_NORMAL);

    // Scanning status
    lv_obj_t *scan_label = lv_label_create(screen);

    if (!ctx->ble_scan_state.scan_complete && ctx->ble_scan_state.count == 0)
    {
        // Scanning with no devices found yet
        lv_label_set_text(scan_label, "Scanning for BLE devices...");
        apply_body_style(scan_label);
        lv_obj_align(scan_label, LV_ALIGN_CENTER, 0, -30);

        // Spinner
        lv_obj_t *spinner = lv_spinner_create(screen);
        lv_obj_set_size(spinner, 60, 60);
        lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 20);
        lv_obj_set_style_arc_color(spinner, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_INDICATOR);

        // Back button
        lv_obj_t *back_btn = lv_btn_create(screen);
        lv_obj_set_size(back_btn, 120, 35);
        apply_button_style(back_btn);
        lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -PADDING_NORMAL);
        lv_obj_add_event_cb(back_btn, ble_back_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "Back");
        apply_button_label_style(back_label);
        lv_obj_center(back_label);
    }
    else if (ctx->ble_scan_state.count == 0)
    {
        lv_label_set_text(scan_label, "No BLE devices found");
        apply_body_style(scan_label);
        lv_obj_align(scan_label, LV_ALIGN_CENTER, 0, -40);

        // Rescan button
        lv_obj_t *rescan_btn = lv_btn_create(screen);
        lv_obj_set_size(rescan_btn, 150, 40);
        apply_button_style(rescan_btn);
        lv_obj_align(rescan_btn, LV_ALIGN_CENTER, 0, 10);
        lv_obj_add_event_cb(rescan_btn, ble_rescan_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *btn_label = lv_label_create(rescan_btn);
        lv_label_set_text(btn_label, "Scan Again");
        apply_button_label_style(btn_label);
        lv_obj_center(btn_label);

        // Back button
        lv_obj_t *back_btn = lv_btn_create(screen);
        lv_obj_set_size(back_btn, 120, 35);
        apply_button_style(back_btn);
        lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -PADDING_NORMAL);
        lv_obj_add_event_cb(back_btn, ble_back_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "Back");
        apply_button_label_style(back_label);
        lv_obj_center(back_label);
    }
    else
    {
        // Devices found - show them (whether still scanning or complete)
        if (!ctx->ble_scan_state.scan_complete)
        {
            char scan_text[50];
            snprintf(scan_text, sizeof(scan_text), "Scanning... (%d found)", ctx->ble_scan_state.count);
            lv_label_set_text(scan_label, scan_text);
        }
        else
        {
            lv_label_set_text(scan_label, "Select a device:");
        }
        apply_body_style(scan_label);
        lv_obj_align(scan_label, LV_ALIGN_TOP_MID, 0, 40);

        // Create dropdown with device list
        lv_obj_t *dropdown = lv_dropdown_create(screen);
        lv_obj_set_width(dropdown, 280);
        apply_dropdown_style(dropdown);
        lv_obj_align(dropdown, LV_ALIGN_TOP_MID, 0, 70);

        // Build device list string
        char device_list[MAX_BLE_SCAN_RESULTS * 60] = {0};
        for (int i = 0; i < ctx->ble_scan_state.count; i++)
        {
            if (i > 0) strcat(device_list, "\n");

            char device_entry[60];
            char addr_str[18];
            ble_address_to_string(ctx->ble_scan_state.results[i].address, addr_str, sizeof(addr_str));

            if (ctx->ble_scan_state.results[i].has_sps_service)
            {
                snprintf(device_entry, sizeof(device_entry), "%s [SPS]",
                        ctx->ble_scan_state.results[i].name);
            }
            else
            {
                snprintf(device_entry, sizeof(device_entry), "%s",
                        ctx->ble_scan_state.results[i].name);
            }
            strcat(device_list, device_entry);
        }

        lv_dropdown_set_options(dropdown, device_list);
        lv_obj_add_event_cb(dropdown, ble_device_selected_event, LV_EVENT_VALUE_CHANGED, ctx);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(screen);
        lv_obj_set_size(connect_btn, 120, 40);
        apply_button_style(connect_btn);
        lv_obj_align(connect_btn, LV_ALIGN_CENTER, -65, 50);
        lv_obj_set_user_data(connect_btn, dropdown);
        lv_obj_add_event_cb(connect_btn, ble_connect_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "Connect");
        apply_button_label_style(connect_label);
        lv_obj_center(connect_label);

        // Rescan button
        lv_obj_t *rescan_btn = lv_btn_create(screen);
        lv_obj_set_size(rescan_btn, 120, 40);
        apply_button_style(rescan_btn);
        lv_obj_align(rescan_btn, LV_ALIGN_CENTER, 65, 50);
        lv_obj_add_event_cb(rescan_btn, ble_rescan_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *rescan_label = lv_label_create(rescan_btn);
        lv_label_set_text(rescan_label, "Rescan");
        apply_button_label_style(rescan_label);
        lv_obj_center(rescan_label);

        // Back button
        lv_obj_t *back_btn = lv_btn_create(screen);
        lv_obj_set_size(back_btn, 120, 35);
        apply_button_style(back_btn);
        lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -PADDING_NORMAL);
        lv_obj_add_event_cb(back_btn, ble_back_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "Back");
        apply_button_label_style(back_label);
        lv_obj_center(back_label);
    }

    return screen;
}

// Create BLE connecting screen
lv_obj_t* create_ble_connecting_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text_fmt(label, "Connecting to\n%s", ctx->selected_ble_name);
    apply_title_style(label);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *spinner = lv_spinner_create(screen);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_INDICATOR);

    return screen;
}

// Create SPS data screen
lv_obj_t* create_sps_data_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title with device name
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text_fmt(title, "SPS: %s", ctx->selected_ble_name);
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // Connection info
    lv_obj_t *info_label = lv_label_create(screen);
    char addr_str[18];
    ble_address_to_string(ctx->selected_ble_address, addr_str, sizeof(addr_str));
    lv_label_set_text_fmt(info_label, "%s (%s)",
                          ble_sps_type_to_string(ctx->selected_sps_type),
                          addr_str);
    lv_obj_set_style_text_font(info_label, FONT_SMALL, 0); // Use small font for details
    lv_obj_set_style_text_color(info_label, lv_color_hex(THEME_TEXT_TERTIARY), 0);
    lv_obj_align(info_label, LV_ALIGN_TOP_MID, 0, 25);

    // RX data (received from device) - read-only text area
    lv_obj_t *rx_label = lv_label_create(screen);
    lv_label_set_text(rx_label, "Received:");
    apply_body_style(rx_label);
    lv_obj_align(rx_label, LV_ALIGN_TOP_LEFT, PADDING_NORMAL, 50);

    sps_rx_textarea = lv_textarea_create(screen);
    lv_obj_set_size(sps_rx_textarea, 300, 80);
    apply_textarea_style(sps_rx_textarea);
    lv_obj_align(sps_rx_textarea, LV_ALIGN_TOP_MID, 0, 70);
    lv_textarea_set_text(sps_rx_textarea, "");
    lv_textarea_set_placeholder_text(sps_rx_textarea, "Waiting for data...");
    lv_obj_add_flag(sps_rx_textarea, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    // TX data (send to device) - editable text area
    lv_obj_t *tx_label = lv_label_create(screen);
    lv_label_set_text(tx_label, "Send:");
    apply_body_style(tx_label);
    lv_obj_align(tx_label, LV_ALIGN_TOP_LEFT, PADDING_NORMAL, 160);

    sps_tx_textarea = lv_textarea_create(screen);
    lv_obj_set_size(sps_tx_textarea, 220, 60);
    apply_textarea_style(sps_tx_textarea);
    lv_obj_align(sps_tx_textarea, LV_ALIGN_TOP_LEFT, PADDING_NORMAL, 180);
    lv_textarea_set_text(sps_tx_textarea, "");
    lv_textarea_set_placeholder_text(sps_tx_textarea, "Enter data...");
    lv_textarea_set_one_line(sps_tx_textarea, false);
    lv_textarea_set_max_length(sps_tx_textarea, 128);
    lv_obj_add_flag(sps_tx_textarea, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    // Send button
    lv_obj_t *send_btn = lv_btn_create(screen);
    lv_obj_set_size(send_btn, 70, 60);
    apply_button_style(send_btn);
    lv_obj_align(send_btn, LV_ALIGN_TOP_RIGHT, -PADDING_NORMAL, 180);
    lv_obj_add_event_cb(send_btn, ble_send_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *send_label = lv_label_create(send_btn);
    lv_label_set_text(send_label, "Send");
    apply_button_label_style(send_label);
    lv_obj_center(send_label);

    // Disconnect button
    lv_obj_t *disconnect_btn = lv_btn_create(screen);
    lv_obj_set_size(disconnect_btn, 130, 35);
    apply_button_style(disconnect_btn);
    lv_obj_align(disconnect_btn, LV_ALIGN_BOTTOM_MID, -65, -PADDING_NORMAL);
    lv_obj_add_event_cb(disconnect_btn, ble_disconnect_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *disc_label = lv_label_create(disconnect_btn);
    lv_label_set_text(disc_label, "Disconnect");
    apply_button_label_style(disc_label);
    lv_obj_center(disc_label);

    // Back to Main button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 130, 35);
    apply_button_style(back_btn);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 65, -PADDING_NORMAL);
    lv_obj_add_event_cb(back_btn, ble_back_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Main Menu");
    apply_button_label_style(back_label);
    lv_obj_center(back_label);

    return screen;
}

// =============================================================================
// BLE Event Handlers
// =============================================================================

// Event handler: BLE device selected from dropdown
static void ble_device_selected_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);
    lv_obj_t *dropdown = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dropdown);

    if (selected < ctx->ble_scan_state.count)
    {
        // Store selected device info
        memcpy(ctx->selected_ble_address,
               ctx->ble_scan_state.results[selected].address, 6);
        ctx->selected_ble_address_type = ctx->ble_scan_state.results[selected].address_type;
        strncpy(ctx->selected_ble_name,
                ctx->ble_scan_state.results[selected].name,
                BLE_DEVICE_NAME_MAX_LEN);
        ctx->selected_ble_name[BLE_DEVICE_NAME_MAX_LEN] = '\0';
        ctx->selected_sps_type = ctx->ble_scan_state.results[selected].sps_type;

        printf("Selected BLE device: %s\n", ctx->selected_ble_name);
    }
}

// Event handler: BLE rescan button
static void ble_rescan_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Rescanning for BLE devices\n");

    // Stop any active scan first
    if (ble_is_scanning())
    {
        printf("Stopping previous BLE scan\n");
        ble_stop_scan();
        sleep_ms(100);  // Give Core1 time to stop
    }

    // Clear error state
    ctx->last_error = ERROR_NONE;

    // Reset BLE scan state
    memset(&ctx->ble_scan_state, 0, sizeof(ble_scan_state_t));
    ctx->ble_scan_requested = true;

    transition_to_state(ctx, APP_STATE_BLE_SCAN);
}

// Event handler: BLE connect button
static void ble_connect_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *dropdown = (lv_obj_t *)lv_obj_get_user_data(btn);

    // Get selected device
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    if (selected < ctx->ble_scan_state.count)
    {
        // Store selected device info
        memcpy(ctx->selected_ble_address,
               ctx->ble_scan_state.results[selected].address, 6);
        ctx->selected_ble_address_type = ctx->ble_scan_state.results[selected].address_type;
        strncpy(ctx->selected_ble_name,
                ctx->ble_scan_state.results[selected].name,
                BLE_DEVICE_NAME_MAX_LEN);
        ctx->selected_ble_name[BLE_DEVICE_NAME_MAX_LEN] = '\0';
        ctx->selected_sps_type = ctx->ble_scan_state.results[selected].sps_type;

        printf("Connecting to %s\n", ctx->selected_ble_name);
        transition_to_state(ctx, APP_STATE_BLE_CONNECTING);
    }
}

// Event handler: BLE disconnect button
static void ble_disconnect_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Disconnecting from BLE device\n");
    ble_disconnect();

    transition_to_state(ctx, APP_STATE_MAIN_APP);
}

// Event handler: BLE send data button
static void ble_send_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    const char *text = lv_textarea_get_text(sps_tx_textarea);
    uint16_t len = strlen(text);

    if (len > 0)
    {
        printf("Sending %d bytes via SPS: %s\n", len, text);

        if (ble_sps_send_data((const uint8_t *)text, len))
        {
            // Clear send text area after successful send
            lv_textarea_set_text(sps_tx_textarea, "");
        }
        else
        {
            printf("Failed to send data\n");
            ctx->last_error = ERROR_BLE_DATA_TRANSFER_FAILED;
            transition_to_state(ctx, APP_STATE_BLE_ERROR);
        }
    }
}

// Event handler: BLE menu button (from main app)
static void ble_menu_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Opening BLE scan menu\n");

    // Stop any active scan first
    if (ble_is_scanning())
    {
        printf("Stopping previous BLE scan\n");
        ble_stop_scan();
        sleep_ms(100);  // Give Core1 time to stop
    }

    // Clear error state
    ctx->last_error = ERROR_NONE;

    // Reset BLE scan state
    memset(&ctx->ble_scan_state, 0, sizeof(ble_scan_state_t));
    ctx->ble_scan_requested = true;

    transition_to_state(ctx, APP_STATE_BLE_SCAN);
}

// Event handler: BLE back button
static void ble_back_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Returning to main app from BLE\n");

    // Stop any active scan
    if (ble_is_scanning())
    {
        printf("Stopping BLE scan\n");
        ble_stop_scan();
    }

    // Return to main app
    transition_to_state(ctx, APP_STATE_MAIN_APP);
}

// =============================================================================
// News Feed Screen Event Handlers
// =============================================================================

// Event handler: News feed button
static void news_feed_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Opening News Feed\n");
    transition_to_state(ctx, APP_STATE_NEWS_FEED);
}

// Event handler: News back button
static void news_back_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Returning to main app from News Feed\n");
    transition_to_state(ctx, APP_STATE_MAIN_APP);
}

// Event handler: News popup close button clicked
static void news_popup_close_event(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *clicked_article_btn = (lv_obj_t *)lv_event_get_user_data(e);

    // Find and close the msgbox
    lv_obj_t *mbox = btn;
    while (mbox && !lv_obj_check_type(mbox, &lv_msgbox_class)) {
        mbox = lv_obj_get_parent(mbox);
    }
    if (mbox) {
        lv_msgbox_close(mbox);
    }

    // Restore focus to the article button that was clicked
    if (clicked_article_btn != NULL) {
        lv_group_t *group = lv_group_get_default();
        if (group != NULL) {
            lv_group_focus_obj(clicked_article_btn);
        }
    }
}

// Event handler: News article clicked
// Event handler to change label text color when button is focused/unfocused
static void news_item_focus_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    // Find the label child - iterate through all children
    uint32_t child_count = lv_obj_get_child_count(btn);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *child = lv_obj_get_child(btn, i);
        if (child && lv_obj_check_type(child, &lv_label_class)) {
            if (code == LV_EVENT_FOCUSED) {
                // Button is focused - set label text to black
                lv_obj_set_style_text_color(child, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else if (code == LV_EVENT_DEFOCUSED) {
                // Button lost focus - restore orange text
                lv_obj_set_style_text_color(child, lv_color_hex(THEME_TEXT_SECONDARY), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }
}

static void news_article_clicked_event(lv_event_t *e)
{
    // Get the article index from user data
    uint8_t *article_idx = (uint8_t *)lv_event_get_user_data(e);
    if (article_idx == NULL) return;

    news_data_t *news_data = news_api_get_data();
    if (*article_idx >= news_data->count) return;

    news_article_t *article = &news_data->articles[*article_idx];

    // Store which button was clicked for focus restoration
    lv_obj_t *clicked_btn = lv_event_get_target(e);

    // Create message box to show article details
    lv_obj_t *mbox = lv_msgbox_create(NULL);

    // Style the message box with modern dark theme
    lv_obj_set_width(mbox, 300);
    lv_obj_set_height(mbox, 200);
    lv_obj_set_style_bg_color(mbox, lv_color_hex(THEME_BG_TERTIARY), 0);
    lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(mbox, lv_color_hex(THEME_BORDER_LIGHT), 0);
    lv_obj_set_style_border_width(mbox, 2, 0);
    lv_obj_set_style_radius(mbox, 6, 0);

    // Set title (source)
    if (strlen(article->source) > 0) {
        lv_msgbox_add_title(mbox, article->source);
    } else {
        lv_msgbox_add_title(mbox, "News Article");
    }

    // Style the header
    lv_obj_t *header = lv_msgbox_get_header(mbox);
    if (header != NULL) {
        lv_obj_set_style_bg_color(header, lv_color_hex(THEME_BG_TERTIARY), 0);
        lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(header, lv_color_hex(THEME_TEXT_PRIMARY), 0);
        lv_obj_set_style_text_font(header, FONT_TITLE, 0);
        lv_obj_set_style_pad_all(header, PADDING_NORMAL, 0);
        lv_obj_set_style_border_width(header, 0, 0);
    }

    // Build content text with title and description
    char content[NEWS_TITLE_MAX_LEN + NEWS_DESCRIPTION_MAX_LEN + 10];
    if (strlen(article->description) > 0) {
        snprintf(content, sizeof(content), "%s\n\n%s",
                 article->title, article->description);
    } else {
        snprintf(content, sizeof(content), "%s\n\n(No description available)",
                 article->title);
    }

    lv_obj_t *text_obj = lv_msgbox_add_text(mbox, content);

    // Style the text content
    if (text_obj != NULL) {
        lv_obj_set_style_text_font(text_obj, FONT_SMALL, 0);
        lv_obj_set_style_text_color(text_obj, lv_color_hex(THEME_TEXT_SECONDARY), 0);
        lv_label_set_long_mode(text_obj, LV_LABEL_LONG_WRAP);
    }

    // Get content area to enable scrolling
    lv_obj_t *content_area = lv_msgbox_get_content(mbox);
    if (content_area != NULL) {
        lv_obj_set_height(content_area, 140);
        lv_obj_set_style_pad_all(content_area, PADDING_SMALL, 0);
        lv_obj_set_style_bg_color(content_area, lv_color_hex(THEME_BG_TERTIARY), 0);
    }

    // Add close button with custom event handler to restore focus
    lv_obj_t *close_btn = lv_msgbox_add_close_button(mbox);
    if (close_btn != NULL) {
        lv_obj_set_style_text_color(close_btn, lv_color_hex(THEME_TEXT_PRIMARY), 0);
        lv_obj_set_style_text_font(close_btn, FONT_TITLE, 0);
        lv_obj_add_event_cb(close_btn, news_popup_close_event, LV_EVENT_CLICKED, clicked_btn);
    }

    lv_obj_center(mbox);

    // Auto-focus the close button so user can immediately interact
    if (close_btn != NULL) {
        lv_group_t *group = lv_group_get_default();
        if (group != NULL) {
            lv_group_add_obj(group, close_btn);
            lv_group_focus_obj(close_btn);
        }
    }

    printf("Showing article %d: %s\n", *article_idx, article->title);
}

// =============================================================================
// News Feed Screen Implementation
// =============================================================================

// Timer callback to update news display
static void news_update_timer_cb(lv_timer_t *timer)
{
    news_data_t *news_data = news_api_get_data();

    if (news_data->state == NEWS_STATE_SUCCESS && news_list != NULL && news_status_label != NULL) {
        // Hide status label
        lv_obj_add_flag(news_status_label, LV_OBJ_FLAG_HIDDEN);

        // Clear the list
        lv_obj_clean(news_list);

        // Add news articles to the list with improved styling
        for (uint8_t i = 0; i < news_data->count; i++) {
            // Store article index for click handler
            news_article_indices[i] = i;

            // Create a button for each news item
            lv_obj_t *btn = lv_list_add_button(news_list, NULL, "");
            lv_obj_set_style_pad_ver(btn, 4, 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_BG_TERTIARY), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(btn, 4, 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(THEME_BORDER_LIGHT), 0);

            // Style focused state - orange background (text color handled by event)
            lv_obj_set_style_bg_color(btn, lv_color_hex(THEME_TEXT_PRIMARY), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUS_KEY);
            lv_obj_set_style_border_color(btn, lv_color_hex(THEME_TEXT_PRIMARY), LV_STATE_FOCUS_KEY);

            // Add event handlers
            lv_obj_add_event_cb(btn, news_article_clicked_event, LV_EVENT_CLICKED, &news_article_indices[i]);
            lv_obj_add_event_cb(btn, news_item_focus_event, LV_EVENT_FOCUSED, NULL);
            lv_obj_add_event_cb(btn, news_item_focus_event, LV_EVENT_DEFOCUSED, NULL);

            // Create label for the news item
            lv_obj_t *label = lv_label_create(btn);

            // Build the text with source and title
            char item_text[256];
            if (strlen(news_data->articles[i].source) > 0) {
                snprintf(item_text, sizeof(item_text), "[%s]\n%s",
                         news_data->articles[i].source,
                         news_data->articles[i].title);
            } else {
                snprintf(item_text, sizeof(item_text), "%s",
                         news_data->articles[i].title);
            }

            lv_label_set_text(label, item_text);
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label, 270);
            lv_obj_set_style_text_font(label, FONT_BODY, 0);
            lv_obj_set_style_text_color(label, lv_color_hex(THEME_TEXT_SECONDARY), 0);
            lv_obj_set_style_pad_all(label, 2, 0);
        }

        // Stop the timer
        if (news_update_timer != NULL) {
            lv_timer_del(news_update_timer);
            news_update_timer = NULL;
        }

        printf("News display updated with %d articles\n", news_data->count);

    } else if (news_data->state == NEWS_STATE_ERROR && news_status_label != NULL) {
        // Show error message
        lv_label_set_text(news_status_label, news_data->error_message);
        lv_obj_set_style_text_font(news_status_label, FONT_BODY, 0);

        // Stop the timer
        if (news_update_timer != NULL) {
            lv_timer_del(news_update_timer);
            news_update_timer = NULL;
        }

        printf("News fetch error: %s\n", news_data->error_message);
    }
    // If still fetching, timer will continue running
}

lv_obj_t* create_news_feed_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "News Feed");
    apply_title_style(title);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 60, 30);
    apply_button_style(back_btn);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, PADDING_SMALL, PADDING_SMALL);
    lv_obj_add_event_cb(back_btn, news_back_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    apply_button_label_style(back_label);
    lv_obj_center(back_label);

    // Status label for loading/error messages
    news_status_label = lv_label_create(screen);
    lv_obj_set_style_text_align(news_status_label, LV_TEXT_ALIGN_CENTER, 0);
    apply_status_style(news_status_label);
    lv_obj_align(news_status_label, LV_ALIGN_CENTER, 0, -50);

    // List for news articles
    news_list = lv_list_create(screen);
    lv_obj_set_size(news_list, 300, 220);
    lv_obj_align(news_list, LV_ALIGN_BOTTOM_MID, 0, -PADDING_NORMAL);
    lv_obj_set_style_bg_color(news_list, lv_color_hex(THEME_BG_SECONDARY), 0);
    lv_obj_set_style_border_width(news_list, 2, 0);
    lv_obj_set_style_border_color(news_list, lv_color_hex(THEME_BORDER_NORMAL), 0);
    lv_obj_set_style_radius(news_list, 6, 0);

    // Style the scrollbar - orange
    lv_obj_set_style_bg_color(news_list, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(news_list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(news_list, 8, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(news_list, 4, LV_PART_SCROLLBAR);

    lv_obj_set_style_pad_all(news_list, PADDING_SMALL, 0);

    // Add widgets to keyboard navigation group for arrow key scrolling
    // The back button is added first, then the news list is added and focused
    lv_group_t *group = lv_group_get_default();
    if (group != NULL) {
        lv_group_add_obj(group, back_btn);
        lv_group_add_obj(group, news_list);
        lv_group_focus_obj(news_list);  // Focus list by default for immediate scrolling
    }

    // Initialize news API if not already done
    news_api_init();

    // API key is defined in api_tokens.h
    // NOTE: Users need to get a free API key from https://newsapi.org
    const char *api_key = NEWSAPI_KEY;

    // Check if API key is configured
    if (strcmp(api_key, "YOUR_API_KEY_HERE") == 0) {
        lv_label_set_text(news_status_label,
                         "Please configure NewsAPI key\n"
                         "Get free key at:\n"
                         "newsapi.org");
        lv_obj_t *placeholder = lv_label_create(news_list);
        lv_label_set_text(placeholder, "API key not configured");
        lv_obj_set_style_text_font(placeholder, FONT_BODY, 0);
        lv_obj_set_style_text_color(placeholder, lv_color_hex(THEME_TEXT_DISABLED), 0);
    } else {
        lv_label_set_text(news_status_label, "Loading news...");
        lv_obj_t *placeholder = lv_label_create(news_list);
        lv_label_set_text(placeholder, "Fetching latest headlines...");
        lv_obj_set_style_text_font(placeholder, FONT_BODY, 0);
        lv_obj_set_style_text_color(placeholder, lv_color_hex(THEME_TEXT_DISABLED), 0);

        // Start fetching news (country code: us = United States)
        news_api_fetch_headlines(api_key, "us");

        // Start update timer (check every 500ms)
        if (news_update_timer != NULL) {
            lv_timer_del(news_update_timer);
        }
        news_update_timer = lv_timer_create(news_update_timer_cb, 500, NULL);
    }

    return screen;
}

// =============================================================================
// TELEGRAM MESSAGING SCREEN
// =============================================================================

// Event handler: Telegram button (main screen)
static void telegram_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Opening Telegram\n");
    transition_to_state(ctx, APP_STATE_TELEGRAM);
}

// Event handler: Telegram back button
static void telegram_back_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    printf("Returning to main app from Telegram\n");

    // Stop polling
    telegram_data_t *data = telegram_api_get_data();
    if (data != NULL) {
        data->polling_active = false;
    }

    transition_to_state(ctx, APP_STATE_MAIN_APP);
}

// Event handler: Telegram send button
static void telegram_send_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);

    if (telegram_input_ta == NULL) return;

    const char *text = lv_textarea_get_text(telegram_input_ta);
    if (text == NULL || strlen(text) == 0) {
        printf("Empty message, not sending\n");
        return;
    }

    printf("Sending message: %s\n", text);

    // Send message via Telegram API (HTTPS)
    telegram_send_message(TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, text);

    // Clear input
    lv_textarea_set_text(telegram_input_ta, "");

    // Show sending status
    if (telegram_status_label != NULL) {
        lv_label_set_text(telegram_status_label, "Sending...");
    }
}

// Event handler: Telegram input textarea key press
static void telegram_input_key_event(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);

    // Check if Enter key was pressed
    if (key == LV_KEY_ENTER) {
        // Call the send function
        telegram_send_btn_event(e);
    }
}

// Timer callback: Update telegram message list
static void telegram_update_timer_cb(lv_timer_t *timer)
{
    telegram_data_t *data = telegram_api_get_data();

    if (data == NULL || telegram_list == NULL) {
        return;
    }

    // Check if we need to update the UI
    if (data->state == TELEGRAM_STATE_SUCCESS && data->message_count > 0) {
        // Update message list
        lv_obj_clean(telegram_list);

        for (uint8_t i = 0; i < data->message_count; i++) {
            telegram_message_t *msg = &data->messages[i];

            lv_obj_t *btn = lv_list_add_button(telegram_list, NULL, "");
            lv_obj_set_style_pad_ver(btn, 4, 0);

            // Format message: "@username: message text"
            char label_text[TELEGRAM_USERNAME_MAX + TELEGRAM_MESSAGE_TEXT_MAX + 20];
            if (strlen(msg->username) > 0) {
                snprintf(label_text, sizeof(label_text), "@%s: %s",
                        msg->username, msg->text);
            } else {
                snprintf(label_text, sizeof(label_text), "%s", msg->text);
            }

            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, label_text);
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label, 270);
            apply_body_style(label);
        }

        // Update status
        if (telegram_status_label != NULL) {
            lv_label_set_text(telegram_status_label, "Connected");
        }
    } else if (data->state == TELEGRAM_STATE_ERROR) {
        // Show error
        if (telegram_status_label != NULL) {
            lv_label_set_text(telegram_status_label, data->error_message);
        }
    } else if (data->state == TELEGRAM_STATE_SENDING) {
        if (telegram_status_label != NULL) {
            lv_label_set_text(telegram_status_label, "Sending...");
        }
    } else if (data->state == TELEGRAM_STATE_RECEIVING) {
        if (telegram_status_label != NULL) {
            lv_label_set_text(telegram_status_label, "Checking for messages...");
        }
    }

    // Poll for new messages if active
    if (data->polling_active && data->state != TELEGRAM_STATE_SENDING &&
        data->state != TELEGRAM_STATE_RECEIVING) {
        telegram_poll_updates(TELEGRAM_BOT_TOKEN);
    }
}

// Create Telegram messaging screen
lv_obj_t* create_telegram_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Telegram");
    apply_title_style(title);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 60, 30);
    apply_button_style(back_btn);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, PADDING_SMALL, PADDING_SMALL);
    lv_obj_add_event_cb(back_btn, telegram_back_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    apply_button_label_style(back_label);
    lv_obj_center(back_label);

    // Status label
    telegram_status_label = lv_label_create(screen);
    lv_obj_set_style_text_align(telegram_status_label, LV_TEXT_ALIGN_CENTER, 0);
    apply_status_style(telegram_status_label);
    lv_obj_align(telegram_status_label, LV_ALIGN_TOP_MID, 0, 40);
    lv_label_set_text(telegram_status_label, "Connecting...");

    // Message list
    telegram_list = lv_list_create(screen);
    lv_obj_set_size(telegram_list, 300, 180);
    lv_obj_align(telegram_list, LV_ALIGN_TOP_MID, 0, 65);
    lv_obj_set_style_bg_color(telegram_list, lv_color_hex(THEME_BG_SECONDARY), 0);
    lv_obj_set_style_border_width(telegram_list, 2, 0);
    lv_obj_set_style_border_color(telegram_list, lv_color_hex(THEME_BORDER_NORMAL), 0);
    lv_obj_set_style_radius(telegram_list, 6, 0);

    // Style the scrollbar
    lv_obj_set_style_bg_color(telegram_list, lv_color_hex(THEME_TEXT_PRIMARY), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(telegram_list, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(telegram_list, 8, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(telegram_list, 4, LV_PART_SCROLLBAR);

    lv_obj_set_style_pad_all(telegram_list, PADDING_SMALL, 0);

    // Message input textarea
    telegram_input_ta = lv_textarea_create(screen);
    lv_obj_set_size(telegram_input_ta, 300, 50);
    apply_textarea_style(telegram_input_ta);
    lv_obj_align(telegram_input_ta, LV_ALIGN_TOP_MID, 0, 255);
    lv_textarea_set_text(telegram_input_ta, "");
    lv_textarea_set_placeholder_text(telegram_input_ta, "Type message and press Enter...");
    lv_textarea_set_one_line(telegram_input_ta, false);  // Multi-line
    lv_textarea_set_max_length(telegram_input_ta, TELEGRAM_MESSAGE_TEXT_MAX);

    // Add event handler for Enter key to send message
    lv_obj_add_event_cb(telegram_input_ta, telegram_input_key_event, LV_EVENT_KEY, ctx);

    // Add widgets to keyboard navigation group
    lv_group_t *group = lv_group_get_default();
    if (group != NULL) {
        lv_group_add_obj(group, back_btn);
        lv_group_add_obj(group, telegram_list);
        lv_group_add_obj(group, telegram_input_ta);
        lv_group_focus_obj(telegram_input_ta);  // Focus input by default
    }

    // Initialize telegram API if not already done
    telegram_api_init();

    // Check if bot token is configured
    const char *bot_token = TELEGRAM_BOT_TOKEN;
    if (strcmp(bot_token, "YOUR_BOT_TOKEN_HERE") == 0) {
        lv_label_set_text(telegram_status_label,
                         "Please configure bot token\n"
                         "See api_tokens.h");
        lv_obj_t *placeholder = lv_label_create(telegram_list);
        lv_label_set_text(placeholder, "Bot token not configured");
        lv_obj_set_style_text_color(placeholder, lv_color_hex(THEME_TEXT_DISABLED), 0);
    } else {
        // Start polling for messages
        telegram_data_t *data = telegram_api_get_data();
        if (data != NULL) {
            data->polling_active = true;
        }

        // Initial poll
        telegram_poll_updates(TELEGRAM_BOT_TOKEN);

        // Start update timer (check every 5 seconds)
        if (telegram_update_timer != NULL) {
            lv_timer_del(telegram_update_timer);
        }
        telegram_update_timer = lv_timer_create(telegram_update_timer_cb, 5000, NULL);
    }

    return screen;
}

// ============================================================================
// WEATHER APP EVENT HANDLERS AND SCREENS
// ============================================================================

// Weather button event (from main menu)
static void weather_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t*)lv_event_get_user_data(e);
    transition_to_state(ctx, APP_STATE_WEATHER_CITY_SELECT);
}

// Weather back button event
static void weather_back_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t*)lv_event_get_user_data(e);
    transition_to_state(ctx, APP_STATE_MAIN_APP);
}

// Weather city button event (predefined city selected)
static void weather_city_btn_event(lv_event_t *e)
{
    int city_index = (int)(intptr_t)lv_event_get_user_data(e);
    ui_context_t *ctx = g_ui_ctx;

    if (ctx == NULL) return;

    ctx->selected_city_index = city_index;

    // Start fetching weather
    const weather_city_t *cities = weather_get_cities();
    weather_api_fetch_forecast(OPENWEATHER_API_KEY, cities[city_index].name);

    transition_to_state(ctx, APP_STATE_WEATHER_LOADING);
}

// Weather custom city button event
static void weather_custom_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t*)lv_event_get_user_data(e);
    transition_to_state(ctx, APP_STATE_WEATHER_CUSTOM_INPUT);
}

// Weather submit city event (custom city entered)
static void weather_submit_city_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t*)lv_event_get_user_data(e);

    if (weather_city_input_ta == NULL) return;

    const char *city = lv_textarea_get_text(weather_city_input_ta);
    if (strlen(city) == 0) {
        return;
    }

    strncpy(ctx->weather_custom_city, city, sizeof(ctx->weather_custom_city) - 1);
    ctx->weather_custom_city[sizeof(ctx->weather_custom_city) - 1] = '\0';
    ctx->selected_city_index = -1;  // Mark as custom

    // Start fetching
    weather_api_fetch_forecast(OPENWEATHER_API_KEY, city);
    transition_to_state(ctx, APP_STATE_WEATHER_LOADING);
}

// Weather input key event (Enter key in custom city input)
static void weather_input_key_event(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER) {
        weather_submit_city_event(e);
    }
}

// Weather refresh button event
static void weather_refresh_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t*)lv_event_get_user_data(e);

    // Refetch weather for the same city
    weather_data_t *data = weather_api_get_data();
    if (data != NULL && strlen(data->city_name) > 0) {
        weather_api_fetch_forecast(OPENWEATHER_API_KEY, data->city_name);
        transition_to_state(ctx, APP_STATE_WEATHER_LOADING);
    }
}

// Weather update timer callback (for loading screen)
static void weather_update_timer_cb(lv_timer_t *timer)
{
    ui_context_t *ctx = (ui_context_t*)lv_timer_get_user_data(timer);
    weather_data_t *data = weather_api_get_data();

    if (data == NULL) return;

    if (data->state == WEATHER_STATE_FETCHING_FORECAST) {
        if (weather_detail_label != NULL) {
            lv_label_set_text(weather_detail_label, "Fetching forecast data...");
        }
    } else if (data->state == WEATHER_STATE_FETCHING_MAP) {
        if (weather_detail_label != NULL) {
            lv_label_set_text(weather_detail_label, "Downloading map image...");
        }
    } else if (data->state == WEATHER_STATE_SUCCESS) {
        // Both forecast and map loaded (or map skipped)
        lv_timer_del(weather_update_timer);
        weather_update_timer = NULL;
        transition_to_state(ctx, APP_STATE_WEATHER_DISPLAY);
    } else if (data->state == WEATHER_STATE_ERROR) {
        lv_timer_del(weather_update_timer);
        weather_update_timer = NULL;

        // Show error
        if (weather_loading_label != NULL) {
            lv_label_set_text(weather_loading_label, "Error");
        }
        if (weather_detail_label != NULL) {
            lv_label_set_text(weather_detail_label, data->error_message);
        }
    }
}

// Create weather city select screen
lv_obj_t* create_weather_city_select_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Weather App");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 60, 30);
    apply_button_style(back_btn);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, PADDING_SMALL, PADDING_SMALL);
    lv_obj_add_event_cb(back_btn, weather_back_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    apply_button_label_style(back_label);
    lv_obj_center(back_label);

    // Select city label
    lv_obj_t *select_label = lv_label_create(screen);
    lv_label_set_text(select_label, "Select City:");
    apply_body_style(select_label);
    lv_obj_align(select_label, LV_ALIGN_TOP_LEFT, PADDING_NORMAL, 45);

    // City list
    lv_obj_t *city_list = lv_list_create(screen);
    lv_obj_set_size(city_list, 300, 240);
    lv_obj_align(city_list, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(city_list, lv_color_hex(THEME_BG_SECONDARY), 0);
    lv_obj_set_style_border_width(city_list, BORDER_THIN, 0);
    lv_obj_set_style_border_color(city_list, lv_color_hex(THEME_BORDER_NORMAL), 0);

    // Add predefined cities
    const weather_city_t *cities = weather_get_cities();
    for (int i = 0; i < MAX_WEATHER_CITIES; i++) {
        lv_obj_t *btn = lv_list_add_btn(city_list, NULL, cities[i].name);
        lv_obj_set_style_text_color(btn, lv_color_hex(THEME_TEXT_SECONDARY), 0);
        lv_obj_add_event_cb(btn, weather_city_btn_event, LV_EVENT_CLICKED,
                           (void*)(intptr_t)i);  // Pass city index
    }

    // Custom city button
    lv_obj_t *custom_btn = lv_list_add_btn(city_list, NULL, "Custom City...");
    lv_obj_set_style_text_color(custom_btn, lv_color_hex(THEME_TEXT_SECONDARY), 0);
    lv_obj_add_event_cb(custom_btn, weather_custom_btn_event, LV_EVENT_CLICKED, ctx);

    return screen;
}

// Create weather custom input screen
lv_obj_t* create_weather_custom_input_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Enter City Name");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // Cancel button
    lv_obj_t *cancel_btn = lv_btn_create(screen);
    lv_obj_set_size(cancel_btn, 70, 30);
    apply_button_style(cancel_btn);
    lv_obj_align(cancel_btn, LV_ALIGN_TOP_LEFT, PADDING_SMALL, PADDING_SMALL);
    lv_obj_add_event_cb(cancel_btn, weather_back_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    apply_button_label_style(cancel_label);
    lv_obj_center(cancel_label);

    // Input label
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "City Name:");
    apply_body_style(label);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -60);

    // Textarea
    weather_city_input_ta = lv_textarea_create(screen);
    lv_obj_set_size(weather_city_input_ta, 280, 50);
    apply_textarea_style(weather_city_input_ta);
    lv_obj_align(weather_city_input_ta, LV_ALIGN_CENTER, 0, -10);
    lv_textarea_set_placeholder_text(weather_city_input_ta, "e.g., Berlin");
    lv_textarea_set_one_line(weather_city_input_ta, true);
    lv_textarea_set_max_length(weather_city_input_ta, WEATHER_CITY_NAME_MAX);

    // Submit button
    lv_obj_t *submit_btn = lv_btn_create(screen);
    lv_obj_set_size(submit_btn, 150, 40);
    apply_button_style(submit_btn);
    lv_obj_align(submit_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(submit_btn, weather_submit_city_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *submit_label = lv_label_create(submit_btn);
    lv_label_set_text(submit_label, "Get Weather");
    apply_button_label_style(submit_label);
    lv_obj_center(submit_label);

    // Add Enter key handler
    lv_obj_add_event_cb(weather_city_input_ta, weather_input_key_event,
                       LV_EVENT_KEY, ctx);

    return screen;
}

// Create weather loading screen
lv_obj_t* create_weather_loading_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Weather");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, PADDING_SMALL);

    // Status label
    weather_loading_label = lv_label_create(screen);
    lv_label_set_text(weather_loading_label, "Loading weather...");
    apply_status_style(weather_loading_label);
    lv_obj_align(weather_loading_label, LV_ALIGN_CENTER, 0, -40);

    // Spinner
    lv_obj_t *spinner = lv_spinner_create(screen);
    lv_obj_set_size(spinner, 50, 50);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(THEME_TEXT_PRIMARY),
                                LV_PART_INDICATOR);

    // Detail label
    weather_detail_label = lv_label_create(screen);
    lv_label_set_text(weather_detail_label, "Fetching forecast data...");
    apply_body_style(weather_detail_label);
    lv_obj_align(weather_detail_label, LV_ALIGN_CENTER, 0, 60);

    // Start update timer (check API state every 500ms)
    if (weather_update_timer != NULL) {
        lv_timer_del(weather_update_timer);
    }
    weather_update_timer = lv_timer_create(weather_update_timer_cb, 500, ctx);

    return screen;
}

// Create weather display screen
lv_obj_t* create_weather_display_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    weather_data_t *data = weather_api_get_data();
    if (data == NULL) {
        return screen;
    }

    // Title (city name)
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, data->city_name);
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, PADDING_SMALL, PADDING_SMALL);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 60, 30);
    apply_button_style(back_btn);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -PADDING_SMALL, PADDING_SMALL);
    lv_obj_add_event_cb(back_btn, weather_back_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    apply_button_label_style(back_label);
    lv_obj_center(back_label);

    // Current weather (first forecast entry)
    if (data->forecast_count > 0) {
        weather_forecast_t *current = &data->forecasts[0];

        lv_obj_t *current_label = lv_label_create(screen);
        char temp_buf[128];
        snprintf(temp_buf, sizeof(temp_buf), "Current: %.1f%cC %s\nFeels like: %.1f%cC",
                 current->temp, 0xB0, weather_get_emoji(current->icon),
                 current->feels_like, 0xB0);
        lv_label_set_text(current_label, temp_buf);
        apply_body_style(current_label);
        lv_obj_align(current_label, LV_ALIGN_TOP_LEFT, PADDING_NORMAL, 40);
    }

    // Forecast list (scrollable)
    lv_obj_t *forecast_list = lv_list_create(screen);
    lv_obj_set_size(forecast_list, 300, 150);
    lv_obj_align(forecast_list, LV_ALIGN_TOP_MID, 0, 85);
    lv_obj_set_style_bg_color(forecast_list, lv_color_hex(THEME_BG_SECONDARY), 0);
    lv_obj_set_style_border_width(forecast_list, BORDER_THIN, 0);
    lv_obj_set_style_border_color(forecast_list, lv_color_hex(THEME_BORDER_NORMAL), 0);

    // Get current time for day grouping
    time_t now = time(NULL);
    struct tm *now_tm = localtime(&now);
    int today_day = now_tm->tm_mday;

    bool today_header_added = false;
    bool tomorrow_header_added = false;

    for (int i = 0; i < data->forecast_count; i++) {
        weather_forecast_t *fc = &data->forecasts[i];
        struct tm *fc_tm = localtime(&fc->timestamp);

        // Add day headers
        if (fc_tm->tm_mday == today_day && !today_header_added) {
            lv_obj_t *header_btn = lv_list_add_btn(forecast_list, NULL, "Today:");
            lv_obj_set_style_text_color(header_btn, lv_color_hex(THEME_TEXT_PRIMARY), 0);
            today_header_added = true;
        } else if (fc_tm->tm_mday == today_day + 1 && !tomorrow_header_added) {
            lv_obj_t *header_btn = lv_list_add_btn(forecast_list, NULL, "Tomorrow:");
            lv_obj_set_style_text_color(header_btn, lv_color_hex(THEME_TEXT_PRIMARY), 0);
            tomorrow_header_added = true;
        }

        // Forecast entry
        char fc_buf[128];
        snprintf(fc_buf, sizeof(fc_buf), "%02d:00  %.0f%cC %s  %s",
                 fc_tm->tm_hour, fc->temp, 0xB0,
                 weather_get_emoji(fc->icon), fc->description);

        lv_obj_t *fc_btn = lv_list_add_btn(forecast_list, NULL, fc_buf);
        lv_obj_set_style_text_color(fc_btn, lv_color_hex(THEME_TEXT_SECONDARY), 0);
    }

    // Weather map image (if loaded)
    if (data->map_loaded && data->map_image_data != NULL && data->map_image_size > 0) {
        printf("Displaying weather map: %d bytes\n", data->map_image_size);

        lv_obj_t *map_img = lv_img_create(screen);

        // Create image descriptor for binary PNG data
        lv_img_dsc_t img_dsc;
        img_dsc.header.w = 0;  // Auto-detect from PNG
        img_dsc.header.h = 0;
        img_dsc.header.cf = LV_COLOR_FORMAT_RAW;
        img_dsc.header.stride = 0;
        img_dsc.data_size = data->map_image_size;
        img_dsc.data = data->map_image_data;

        lv_img_set_src(map_img, &img_dsc);
        lv_obj_align(map_img, LV_ALIGN_BOTTOM_RIGHT, -PADDING_NORMAL, -45);
        lv_obj_set_size(map_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        // Add border
        lv_obj_set_style_border_width(map_img, BORDER_THIN, 0);
        lv_obj_set_style_border_color(map_img, lv_color_hex(THEME_BORDER_NORMAL), 0);
        lv_obj_set_style_radius(map_img, 3, 0);
    } else {
        printf("Map not loaded: loaded=%d, data=%p, size=%d\n",
               data->map_loaded, data->map_image_data, data->map_image_size);
    }

    // View Map button (left)
    lv_obj_t *map_btn = lv_btn_create(screen);
    lv_obj_set_size(map_btn, 100, 35);
    apply_button_style(map_btn);
    lv_obj_align(map_btn, LV_ALIGN_BOTTOM_LEFT, PADDING_NORMAL, -PADDING_SMALL);
    lv_obj_add_event_cb(map_btn, weather_view_map_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *map_label = lv_label_create(map_btn);
    lv_label_set_text(map_label, "View Map");
    apply_button_label_style(map_label);
    lv_obj_center(map_label);

    // Refresh button (right)
    lv_obj_t *refresh_btn = lv_btn_create(screen);
    lv_obj_set_size(refresh_btn, 100, 35);
    apply_button_style(refresh_btn);
    lv_obj_align(refresh_btn, LV_ALIGN_BOTTOM_RIGHT, -PADDING_NORMAL, -PADDING_SMALL);
    lv_obj_add_event_cb(refresh_btn, weather_refresh_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *refresh_label = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_label, "Refresh");
    apply_button_label_style(refresh_label);
    lv_obj_center(refresh_label);

    return screen;
}

// View Map button event handler
static void weather_view_map_btn_event(lv_event_t *e)
{
    ui_context_t *ctx = (ui_context_t *)lv_event_get_user_data(e);
    
    weather_data_t *data = weather_api_get_data();
    if (data && data->map_loaded) {
        printf("Opening fullscreen map view\n");
        transition_to_state(ctx, APP_STATE_WEATHER_MAP);
    } else {
        printf("Map not yet loaded, fetching...\n");
        // Trigger map fetch
        if (data) {
            weather_api_fetch_map("YOUR_API_KEY", data->latitude, data->longitude);
        }
    }
}

// Create fullscreen weather map screen
lv_obj_t* create_weather_map_screen(ui_context_t *ctx)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    apply_screen_style(screen);

    weather_data_t *data = weather_api_get_data();
    if (data == NULL) {
        lv_obj_t *error_label = lv_label_create(screen);
        lv_label_set_text(error_label, "No weather data");
        apply_body_style(error_label);
        lv_obj_center(error_label);
        return screen;
    }

    // Title
    lv_obj_t *title = lv_label_create(screen);
    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "%s - Weather Map", data->city_name);
    lv_label_set_text(title, title_buf);
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, PADDING_SMALL, PADDING_SMALL);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 60, 30);
    apply_button_style(back_btn);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -PADDING_SMALL, PADDING_SMALL);
    lv_obj_add_event_cb(back_btn, weather_back_btn_event, LV_EVENT_CLICKED, ctx);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    apply_button_label_style(back_label);
    lv_obj_center(back_label);

    // Display fullscreen map
    if (data->map_loaded && data->map_image_data != NULL && data->map_image_size > 0) {
        printf("Displaying fullscreen weather map: %d bytes\n", data->map_image_size);

        lv_obj_t *map_img = lv_img_create(screen);

        // Create LVGL image descriptor for PNG data
        static lv_img_dsc_t img_dsc;  // Static to persist
        img_dsc.header.w = 0;  // Auto-detect from PNG
        img_dsc.header.h = 0;
        img_dsc.header.cf = LV_COLOR_FORMAT_RAW;
        img_dsc.header.stride = 0;
        img_dsc.data_size = data->map_image_size;
        img_dsc.data = data->map_image_data;

        lv_img_set_src(map_img, &img_dsc);
        lv_obj_align(map_img, LV_ALIGN_CENTER, 0, 10);
        
        // Scale to fit screen if needed
        lv_obj_set_size(map_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        // Add border
        lv_obj_set_style_border_width(map_img, BORDER_THIN, 0);
        lv_obj_set_style_border_color(map_img, lv_color_hex(THEME_BORDER_NORMAL), 0);
        lv_obj_set_style_radius(map_img, 5, 0);

        // Map info label
        lv_obj_t *info_label = lv_label_create(screen);
        lv_label_set_text(info_label, "Temperature Map");
        apply_body_style(info_label);
        lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -45);
    } else {
        // Map not loaded - show loading message
        lv_obj_t *loading_label = lv_label_create(screen);
        if (weather_api_get_state() == WEATHER_STATE_FETCHING_MAP) {
            lv_label_set_text(loading_label, "Loading map...");
        } else {
            lv_label_set_text(loading_label, "Map not available\nPress Fetch to download");
        }
        apply_body_style(loading_label);
        lv_obj_center(loading_label);

        // Fetch button
        lv_obj_t *fetch_btn = lv_btn_create(screen);
        lv_obj_set_size(fetch_btn, 120, 40);
        apply_button_style(fetch_btn);
        lv_obj_align(fetch_btn, LV_ALIGN_CENTER, 0, 50);
        lv_obj_add_event_cb(fetch_btn, weather_view_map_btn_event, LV_EVENT_CLICKED, ctx);

        lv_obj_t *fetch_label = lv_label_create(fetch_btn);
        lv_label_set_text(fetch_label, "Fetch Map");
        apply_button_label_style(fetch_label);
        lv_obj_center(fetch_label);
    }

    return screen;
}
