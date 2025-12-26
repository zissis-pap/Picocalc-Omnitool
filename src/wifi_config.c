#include "wifi_config.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// CRC32 lookup table
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

// Calculate CRC32 checksum
uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

// Load WiFi configuration from flash
bool wifi_config_load(wifi_config_t *config) {
    const wifi_config_t *flash_config =
        (const wifi_config_t *)(XIP_BASE + WIFI_CONFIG_FLASH_OFFSET);

    // Check magic number
    if (flash_config->magic != WIFI_CONFIG_MAGIC) {
        printf("No valid WiFi config in flash (magic: 0x%08X)\n", flash_config->magic);
        return false;
    }

    // Validate CRC32
    uint32_t calc_crc = calculate_crc32((const uint8_t*)flash_config,
                                        sizeof(wifi_config_t) - sizeof(uint32_t));
    if (calc_crc != flash_config->crc32) {
        printf("WiFi config CRC mismatch (calculated: 0x%08X, stored: 0x%08X)\n",
               calc_crc, flash_config->crc32);
        return false;
    }

    // Copy valid configuration
    memcpy(config, flash_config, sizeof(wifi_config_t));
    printf("Loaded WiFi config from flash: SSID=%s\n", config->ssid);
    return true;
}

// Flash write callback (called from flash_safe_execute)
static void __no_inline_not_in_flash_func(flash_write_impl)(void *param) {
    wifi_config_t *config = (wifi_config_t *)param;

    // Erase the sector
    flash_range_erase(WIFI_CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);

    // Write the configuration
    flash_range_program(WIFI_CONFIG_FLASH_OFFSET, (const uint8_t*)config, FLASH_PAGE_SIZE);
}

// Save WiFi configuration to flash
bool wifi_config_save(const wifi_config_t *config) {
    // Create a copy with calculated CRC
    wifi_config_t config_copy;
    memcpy(&config_copy, config, sizeof(wifi_config_t));
    config_copy.magic = WIFI_CONFIG_MAGIC;
    config_copy.crc32 = calculate_crc32((const uint8_t*)&config_copy,
                                        sizeof(wifi_config_t) - sizeof(uint32_t));

    printf("Saving WiFi config to flash: SSID=%s, CRC=0x%08X\n",
           config_copy.ssid, config_copy.crc32);

    // Retry up to 3 times
    const int MAX_RETRIES = 3;
    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        // Disable interrupts during flash operation
        uint32_t ints = save_and_disable_interrupts();

        // Perform flash write
        flash_write_impl(&config_copy);

        // Re-enable interrupts
        restore_interrupts(ints);

        // Verify the write
        wifi_config_t verify;
        if (wifi_config_load(&verify) &&
            strcmp(verify.ssid, config_copy.ssid) == 0) {
            printf("WiFi config saved successfully\n");
            return true;
        }

        printf("Flash write verification failed (attempt %d/%d)\n",
               retry + 1, MAX_RETRIES);
        sleep_ms(100);
    }

    printf("Failed to save WiFi config after %d attempts\n", MAX_RETRIES);
    return false;
}

// Erase WiFi configuration from flash
void wifi_config_erase(void) {
    printf("Erasing WiFi config from flash\n");

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(WIFI_CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

// Global scan state for callback
static wifi_scan_state_t *g_scan_state = NULL;

// WiFi scan result callback
static int scan_result_callback(void *env, const cyw43_ev_scan_result_t *result) {
    wifi_scan_state_t *state = (wifi_scan_state_t *)env;

    if (result && state->count < MAX_SCAN_RESULTS && result->ssid_len > 0) {
        // Copy SSID
        size_t copy_len = result->ssid_len < 32 ? result->ssid_len : 32;
        memcpy(state->results[state->count].ssid, result->ssid, copy_len);
        state->results[state->count].ssid[copy_len] = '\0';

        // Store auth mode and RSSI
        state->results[state->count].auth_mode = result->auth_mode;
        state->results[state->count].rssi = result->rssi;

        state->count++;
    }

    return 0; // Continue scanning
}

// Start WiFi scan
bool wifi_start_scan(wifi_scan_state_t *state) {
    // Check if a scan is already active
    if (cyw43_wifi_scan_active(&cyw43_state)) {
        printf("Scan already in progress, cannot start new scan\n");
        state->scan_error = true;
        return false;
    }

    memset(state, 0, sizeof(wifi_scan_state_t));
    g_scan_state = state;

    printf("Starting WiFi scan...\n");

    cyw43_wifi_scan_options_t scan_options = {0};
    int result = cyw43_wifi_scan(&cyw43_state, &scan_options, state, scan_result_callback);

    if (result != 0) {
        printf("Failed to start WiFi scan: %d\n", result);
        state->scan_error = true;
        return false;
    }

    printf("WiFi scan started successfully\n");
    return true;
}

// Wait for scan to complete
bool wifi_wait_for_scan(wifi_scan_state_t *state, uint32_t timeout_ms) {
    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);

    while (cyw43_wifi_scan_active(&cyw43_state)) {
        cyw43_arch_poll();
        sleep_ms(100);

        if (time_reached(timeout_time)) {
            printf("WiFi scan timeout\n");
            state->scan_error = true;
            return false;
        }
    }

    state->scan_complete = true;
    printf("WiFi scan complete, found %d networks\n", state->count);
    return true;
}

// Comparison function for qsort (sort by RSSI descending)
static int rssi_compare(const void *a, const void *b) {
    const scan_result_t *ra = (const scan_result_t *)a;
    const scan_result_t *rb = (const scan_result_t *)b;
    return rb->rssi - ra->rssi; // Descending order (strongest first)
}

// Sort scan results by signal strength and remove duplicates
void wifi_sort_scan_results(wifi_scan_state_t *state) {
    if (state->count == 0) return;

    // First, sort by RSSI (strongest first)
    qsort(state->results, state->count, sizeof(scan_result_t), rssi_compare);

    // Remove duplicates, keeping the strongest signal for each SSID
    int write_idx = 0;
    for (int read_idx = 0; read_idx < state->count; read_idx++) {
        // Check if this SSID already exists in the deduplicated list
        bool is_duplicate = false;
        for (int check_idx = 0; check_idx < write_idx; check_idx++) {
            if (strcmp(state->results[read_idx].ssid, state->results[check_idx].ssid) == 0) {
                is_duplicate = true;
                break;
            }
        }

        // If not duplicate, keep it
        if (!is_duplicate) {
            if (write_idx != read_idx) {
                state->results[write_idx] = state->results[read_idx];
            }
            write_idx++;
        }
    }

    state->count = write_idx;
}

// Connect to WiFi network
bool wifi_connect_blocking(const char *ssid, const char *pass, uint32_t auth, uint32_t timeout_ms) {
    int result = cyw43_arch_wifi_connect_timeout_ms(ssid, pass, auth, timeout_ms);

    if (result != 0) {
        printf("WiFi connection failed (error %d)\n", result);
        return false;
    }

    printf("WiFi connected to %s\n", ssid);
    return true;
}

// Check if WiFi is connected
// Note: link_status values: 0=DOWN, 1=JOIN, 2=NOIP, 3=UP
// We consider 1+ as "connected" since JOIN means associated with AP
bool wifi_is_connected(void) {
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    return (link_status > 0);  // 1=JOIN, 2=NOIP, 3=UP all mean connected
}

// Disconnect from WiFi
void wifi_disconnect(void) {
    printf("Disconnecting from WiFi\n");
    cyw43_arch_disable_sta_mode();
}

// Convert scan result auth mode (uint8_t) to connection auth mode (uint32_t)
// Scan results use simple integer encoding, but connection needs full CYW43_AUTH_* constants
uint32_t convert_scan_auth_to_connect_auth(uint8_t scan_auth) {
    switch (scan_auth) {
        case 0: return CYW43_AUTH_OPEN;
        case 1: return CYW43_AUTH_OPEN;  // WEP - not supported, treat as open
        case 2: // WPA-PSK
        case 3: // WPA-PSK (AES variant)
            return CYW43_AUTH_WPA_TKIP_PSK;
        case 4: // WPA2-PSK (AES)
        case 5: // WPA2-PSK (Mixed TKIP+AES)
        case 6: // WPA2-PSK variants
            return CYW43_AUTH_WPA2_AES_PSK;
        case 7: // WPA3 or WPA2/WPA3 mixed
        case 8:
            return CYW43_AUTH_WPA2_MIXED_PSK;
        default:
            return CYW43_AUTH_WPA2_MIXED_PSK;
    }
}

// Convert auth mode to string
const char* wifi_auth_mode_to_string(uint32_t auth_mode) {
    switch (auth_mode) {
        case CYW43_AUTH_OPEN: return "Open";
        case CYW43_AUTH_WPA_TKIP_PSK: return "WPA";
        case CYW43_AUTH_WPA2_AES_PSK: return "WPA2";
        case CYW43_AUTH_WPA2_MIXED_PSK: return "WPA/WPA2";
        default: return "Unknown";
    }
}
