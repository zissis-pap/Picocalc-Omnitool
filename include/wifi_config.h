#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/cyw43_arch.h"

// Flash storage configuration
#define WIFI_CONFIG_MAGIC 0x57494649  // "WIFI" in hex
#define WIFI_CONFIG_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)  // Last 4KB sector
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

// WiFi scanning configuration
#define MAX_SCAN_RESULTS 20
#define WIFI_SCAN_TIMEOUT_MS 30000
#define WIFI_CONNECT_TIMEOUT_MS 15000

// WiFi configuration structure stored in flash
typedef struct {
    uint32_t magic;                          // Validity marker
    char ssid[WIFI_SSID_MAX_LEN + 1];       // Network SSID
    char password[WIFI_PASS_MAX_LEN + 1];   // Network password
    uint32_t auth_mode;                      // CYW43_AUTH_* constant
    uint32_t crc32;                          // Data integrity check
} wifi_config_t;

// Single scan result
typedef struct {
    char ssid[33];
    uint8_t auth_mode;
    int8_t rssi;
} scan_result_t;

// WiFi scan state
typedef struct {
    scan_result_t results[MAX_SCAN_RESULTS];
    int count;
    bool scan_complete;
    bool scan_error;
} wifi_scan_state_t;

// Flash operations
bool wifi_config_load(wifi_config_t *config);
bool wifi_config_save(const wifi_config_t *config);
void wifi_config_erase(void);

// WiFi scanning
bool wifi_start_scan(wifi_scan_state_t *state);
bool wifi_wait_for_scan(wifi_scan_state_t *state, uint32_t timeout_ms);
void wifi_sort_scan_results(wifi_scan_state_t *state);

// WiFi connection
bool wifi_connect_blocking(const char *ssid, const char *pass, uint32_t auth, uint32_t timeout_ms);
bool wifi_is_connected(void);
void wifi_disconnect(void);

// Utility functions
uint32_t calculate_crc32(const uint8_t *data, size_t length);
uint32_t convert_scan_auth_to_connect_auth(uint8_t scan_auth);
const char* wifi_auth_mode_to_string(uint32_t auth_mode);

#endif // WIFI_CONFIG_H
