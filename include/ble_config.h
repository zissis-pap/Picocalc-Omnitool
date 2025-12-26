#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "btstack.h"

// BLE scanning configuration
#define MAX_BLE_SCAN_RESULTS 20
#define BLE_SCAN_TIMEOUT_MS 30000
#define BLE_CONNECT_TIMEOUT_MS 15000
#define BLE_DEVICE_NAME_MAX_LEN 32

// Nordic UART Service (NUS) UUIDs
#define NORDIC_NUS_SERVICE_UUID         "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NORDIC_NUS_RX_CHAR_UUID         "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Write to device
#define NORDIC_NUS_TX_CHAR_UUID         "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Read from device

// u-blox Serial Port Service UUIDs
#define UBLOX_SPS_SERVICE_UUID          "2456E1B9-26E2-8F83-E744-F34F01E9D701"
#define UBLOX_SPS_FIFO_CHAR_UUID        "2456E1B9-26E2-8F83-E744-F34F01E9D703"  // TX (read from device)
#define UBLOX_SPS_CREDITS_CHAR_UUID     "2456E1B9-26E2-8F83-E744-F34F01E9D704"  // RX (write to device)

// SPS device type
typedef enum {
    SPS_TYPE_UNKNOWN = 0,
    SPS_TYPE_NORDIC_NUS,
    SPS_TYPE_UBLOX_SPS
} sps_device_type_t;

// Single BLE scan result
typedef struct {
    bd_addr_t address;                          // 6-byte MAC address
    bd_addr_type_t address_type;                // Public or random address
    char name[BLE_DEVICE_NAME_MAX_LEN + 1];     // Device name from advertising
    int8_t rssi;                                // Signal strength
    sps_device_type_t sps_type;                 // Detected SPS service type
    bool has_sps_service;                        // True if Nordic NUS or u-blox SPS detected
} ble_device_result_t;

// BLE scan state
typedef struct {
    ble_device_result_t results[MAX_BLE_SCAN_RESULTS];
    int count;
    bool scan_complete;
    bool scan_error;
    bool scan_active;
} ble_scan_state_t;

// BLE connection state
typedef struct {
    hci_con_handle_t connection_handle;
    bd_addr_t device_address;
    bd_addr_type_t address_type;
    char device_name[BLE_DEVICE_NAME_MAX_LEN + 1];
    sps_device_type_t sps_type;
    bool connected;

    // GATT client state
    gatt_client_service_t sps_service;
    gatt_client_characteristic_t tx_characteristic;  // Read from device
    gatt_client_characteristic_t rx_characteristic;  // Write to device
    bool service_discovered;
    bool characteristics_discovered;

    // Data transfer
    uint16_t tx_value_handle;
    uint16_t rx_value_handle;
    bool notifications_enabled;
} ble_connection_state_t;

// Data callback type for received data
typedef void (*ble_data_received_callback_t)(const uint8_t *data, uint16_t length);

// BLE initialization and control
void ble_init(void);
void ble_core1_entry(void);
bool ble_is_initialized(void);

// BLE scanning
bool ble_start_scan(void);
void ble_stop_scan(void);
bool ble_is_scanning(void);
void ble_get_scan_results(ble_scan_state_t *state);
void ble_sort_scan_results(ble_scan_state_t *state);

// BLE connection
bool ble_connect(const bd_addr_t address, bd_addr_type_t address_type);
void ble_disconnect(void);
bool ble_is_connected(void);
ble_connection_state_t* ble_get_connection_state(void);

// SPS service discovery
bool ble_discover_sps_service(void);
bool ble_is_sps_ready(void);

// SPS data transfer
bool ble_sps_send_data(const uint8_t *data, uint16_t length);
void ble_sps_set_data_callback(ble_data_received_callback_t callback);

// Utility functions
const char* ble_sps_type_to_string(sps_device_type_t type);
void ble_address_to_string(const bd_addr_t address, char *str, size_t len);

#endif // BLE_CONFIG_H
