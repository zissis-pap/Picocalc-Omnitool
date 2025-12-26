/**
 * BLE Configuration and SPS Service Support
 *
 * Implements BLE scanning, connection, and Serial Port Service (SPS) support
 * for both Nordic UART Service (NUS) and u-blox SPS.
 *
 * Runs on Core1 for parallel processing with WiFi and UI on Core0.
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "ble_config.h"
#include "btstack.h"

// UUID conversion helpers
static uint8_t nordic_nus_service_uuid[16];
static uint8_t nordic_nus_rx_char_uuid[16];
static uint8_t nordic_nus_tx_char_uuid[16];
static uint8_t ublox_sps_service_uuid[16];
static uint8_t ublox_sps_fifo_char_uuid[16];
static uint8_t ublox_sps_credits_char_uuid[16];

// Global state (protected by mutex for Core0 ↔ Core1 communication)
auto_init_mutex(ble_mutex);
static ble_scan_state_t scan_state = {0};
static ble_connection_state_t connection_state = {0};
static ble_data_received_callback_t data_callback = NULL;
static bool initialized = false;
static bool scanning = false;

// BTStack state
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

// Forward declarations
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static bool parse_advertisement_data(const uint8_t *adv_data, uint8_t adv_len,
                                     char *name, size_t name_len, sps_device_type_t *sps_type);

// =============================================================================
// UUID Conversion Utilities
// =============================================================================

static void parse_uuid128_string(const char *uuid_str, uint8_t *uuid128) {
    // Parse UUID string format: "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
    // Convert to little-endian byte array
    sscanf_bd_addr(uuid_str, uuid128);
    reverse_128(uuid128, uuid128);
}

static void init_service_uuids(void) {
    parse_uuid128_string(NORDIC_NUS_SERVICE_UUID, nordic_nus_service_uuid);
    parse_uuid128_string(NORDIC_NUS_RX_CHAR_UUID, nordic_nus_rx_char_uuid);
    parse_uuid128_string(NORDIC_NUS_TX_CHAR_UUID, nordic_nus_tx_char_uuid);
    parse_uuid128_string(UBLOX_SPS_SERVICE_UUID, ublox_sps_service_uuid);
    parse_uuid128_string(UBLOX_SPS_FIFO_CHAR_UUID, ublox_sps_fifo_char_uuid);
    parse_uuid128_string(UBLOX_SPS_CREDITS_CHAR_UUID, ublox_sps_credits_char_uuid);
}

// =============================================================================
// BLE Initialization
// =============================================================================

void ble_init(void) {
    if (initialized) {
        printf("BLE already initialized\n");
        return;
    }

    printf("Initializing BLE...\n");

    // Initialize service UUIDs
    init_service_uuids();

    // Clear states
    mutex_enter_blocking(&ble_mutex);
    memset(&scan_state, 0, sizeof(scan_state));
    memset(&connection_state, 0, sizeof(connection_state));
    mutex_exit(&ble_mutex);

    initialized = true;
    printf("BLE initialization complete\n");
}

bool ble_is_initialized(void) {
    return initialized;
}

// =============================================================================
// Core1 Entry Point
// =============================================================================

void ble_core1_entry(void) {
    printf("BLE Core1 started\n");

    // Initialize BTStack
    l2cap_init();
    sm_init();
    att_server_init(NULL, NULL, NULL);  // No ATT database, callbacks
    gatt_client_init();

    // Register HCI event handler
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Register SM event handler
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // Turn on Bluetooth
    hci_power_control(HCI_POWER_ON);

    printf("BTStack initialized on Core1\n");

    // Main event loop for Core1
    while (true) {
        // Process BTStack events
        btstack_run_loop_execute();

        // Small delay to prevent busy-waiting
        sleep_ms(1);
    }
}

// =============================================================================
// BLE Scanning
// =============================================================================

bool ble_start_scan(void) {
    if (!initialized) {
        printf("BLE not initialized\n");
        return false;
    }

    if (scanning) {
        printf("Scan already in progress\n");
        return false;
    }

    printf("Starting BLE scan...\n");

    mutex_enter_blocking(&ble_mutex);
    memset(&scan_state, 0, sizeof(scan_state));
    scan_state.scan_active = true;
    mutex_exit(&ble_mutex);

    // Configure scan parameters (active scanning for full advertisement data)
    gap_set_scan_parameters(1, 0x0030, 0x0030);  // Active scan, 30ms interval, 30ms window
    gap_start_scan();

    scanning = true;
    printf("BLE scan started\n");
    return true;
}

void ble_stop_scan(void) {
    if (scanning) {
        gap_stop_scan();
        scanning = false;

        mutex_enter_blocking(&ble_mutex);
        scan_state.scan_complete = true;
        scan_state.scan_active = false;
        mutex_exit(&ble_mutex);

        printf("BLE scan stopped, found %d devices\n", scan_state.count);
    }
}

bool ble_is_scanning(void) {
    return scanning;
}

void ble_get_scan_results(ble_scan_state_t *state) {
    if (state == NULL) return;

    mutex_enter_blocking(&ble_mutex);
    memcpy(state, &scan_state, sizeof(ble_scan_state_t));
    mutex_exit(&ble_mutex);
}

// Comparison function for qsort (sort by RSSI, descending)
static int rssi_compare(const void *a, const void *b) {
    const ble_device_result_t *ra = (const ble_device_result_t *)a;
    const ble_device_result_t *rb = (const ble_device_result_t *)b;
    return rb->rssi - ra->rssi;  // Descending (strongest first)
}

void ble_sort_scan_results(ble_scan_state_t *state) {
    if (state == NULL || state->count == 0) return;

    // Sort by signal strength
    qsort(state->results, state->count, sizeof(ble_device_result_t), rssi_compare);

    // Remove duplicates (keep strongest signal)
    int write_idx = 0;
    for (int read_idx = 0; read_idx < state->count; read_idx++) {
        bool is_duplicate = false;

        // Check if MAC address already exists
        for (int check_idx = 0; check_idx < write_idx; check_idx++) {
            if (memcmp(state->results[read_idx].address,
                      state->results[check_idx].address,
                      6) == 0) {
                is_duplicate = true;
                break;
            }
        }

        // Keep if not duplicate
        if (!is_duplicate) {
            if (write_idx != read_idx) {
                state->results[write_idx] = state->results[read_idx];
            }
            write_idx++;
        }
    }

    state->count = write_idx;
    printf("Deduplicated scan results: %d unique devices\n", state->count);
}

// =============================================================================
// Advertisement Data Parsing
// =============================================================================

static bool parse_advertisement_data(const uint8_t *adv_data, uint8_t adv_len,
                                     char *name, size_t name_len, sps_device_type_t *sps_type) {
    ad_context_t context;
    bool found_name = false;
    *sps_type = SPS_TYPE_UNKNOWN;

    for (ad_iterator_init(&context, adv_len, (uint8_t *)adv_data);
         ad_iterator_has_more(&context);
         ad_iterator_next(&context)) {

        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t data_len = ad_iterator_get_data_len(&context);
        const uint8_t *data = ad_iterator_get_data(&context);

        switch (data_type) {
            case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
            case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
                if (data_len > 0 && data_len < name_len) {
                    memcpy(name, data, data_len);
                    name[data_len] = '\0';
                    found_name = true;
                }
                break;

            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
                // Check for Nordic NUS or u-blox SPS service UUIDs
                for (int i = 0; i + 16 <= data_len; i += 16) {
                    if (memcmp(&data[i], nordic_nus_service_uuid, 16) == 0) {
                        *sps_type = SPS_TYPE_NORDIC_NUS;
                        printf("Detected Nordic NUS service\n");
                    } else if (memcmp(&data[i], ublox_sps_service_uuid, 16) == 0) {
                        *sps_type = SPS_TYPE_UBLOX_SPS;
                        printf("Detected u-blox SPS service\n");
                    }
                }
                break;
        }
    }

    return found_name;
}

// =============================================================================
// BLE Connection Management
// =============================================================================

bool ble_connect(const bd_addr_t address, bd_addr_type_t address_type) {
    if (!initialized) {
        printf("BLE not initialized\n");
        return false;
    }

    if (connection_state.connected) {
        printf("Already connected to a device\n");
        return false;
    }

    // Stop scanning if active
    if (scanning) {
        ble_stop_scan();
    }

    printf("Connecting to BLE device...\n");

    mutex_enter_blocking(&ble_mutex);
    memcpy(connection_state.device_address, address, 6);
    connection_state.address_type = address_type;
    connection_state.connected = false;
    connection_state.service_discovered = false;
    connection_state.characteristics_discovered = false;
    mutex_exit(&ble_mutex);

    // Initiate connection
    uint8_t status = gap_connect(address, address_type);
    if (status != ERROR_CODE_SUCCESS) {
        printf("Failed to initiate connection: 0x%02x\n", status);
        return false;
    }

    return true;
}

void ble_disconnect(void) {
    if (connection_state.connected) {
        gap_disconnect(connection_state.connection_handle);
    }
}

bool ble_is_connected(void) {
    bool connected;
    mutex_enter_blocking(&ble_mutex);
    connected = connection_state.connected;
    mutex_exit(&ble_mutex);
    return connected;
}

ble_connection_state_t* ble_get_connection_state(void) {
    return &connection_state;
}

// =============================================================================
// SPS Service Discovery
// =============================================================================

bool ble_discover_sps_service(void) {
    if (!connection_state.connected) {
        printf("Not connected to any device\n");
        return false;
    }

    printf("Discovering SPS services...\n");

    // Discover primary services (will be handled in GATT event handler)
    uint8_t status = gatt_client_discover_primary_services(
        handle_gatt_client_event,
        connection_state.connection_handle
    );

    if (status != ERROR_CODE_SUCCESS) {
        printf("Failed to start service discovery: 0x%02x\n", status);
        return false;
    }

    return true;
}

bool ble_is_sps_ready(void) {
    bool ready;
    mutex_enter_blocking(&ble_mutex);
    ready = connection_state.connected &&
            connection_state.service_discovered &&
            connection_state.characteristics_discovered &&
            connection_state.notifications_enabled;
    mutex_exit(&ble_mutex);
    return ready;
}

// =============================================================================
// SPS Data Transfer
// =============================================================================

bool ble_sps_send_data(const uint8_t *data, uint16_t length) {
    if (!ble_is_sps_ready()) {
        printf("SPS not ready for data transfer\n");
        return false;
    }

    if (connection_state.rx_value_handle == 0) {
        printf("RX characteristic not available\n");
        return false;
    }

    // Write data to RX characteristic (write to device)
    uint8_t status = gatt_client_write_value_of_characteristic(
        handle_gatt_client_event,
        connection_state.connection_handle,
        connection_state.rx_value_handle,
        length,
        (uint8_t *)data
    );

    if (status != ERROR_CODE_SUCCESS) {
        printf("Failed to send data: 0x%02x\n", status);
        return false;
    }

    printf("Sent %d bytes via SPS\n", length);
    return true;
}

void ble_sps_set_data_callback(ble_data_received_callback_t callback) {
    mutex_enter_blocking(&ble_mutex);
    data_callback = callback;
    mutex_exit(&ble_mutex);
}

// =============================================================================
// BTStack Event Handlers
// =============================================================================

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);

    switch (event_type) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                printf("BTStack working\n");
            }
            break;

        case GAP_EVENT_ADVERTISING_REPORT: {
            if (!scanning || scan_state.count >= MAX_BLE_SCAN_RESULTS) break;

            bd_addr_t address;
            gap_event_advertising_report_get_address(packet, address);
            uint8_t address_type = gap_event_advertising_report_get_address_type(packet);
            int8_t rssi = gap_event_advertising_report_get_rssi(packet);
            uint8_t adv_len = gap_event_advertising_report_get_data_length(packet);
            const uint8_t *adv_data = gap_event_advertising_report_get_data(packet);

            // Parse advertisement data
            char name[BLE_DEVICE_NAME_MAX_LEN + 1] = {0};
            sps_device_type_t sps_type = SPS_TYPE_UNKNOWN;
            parse_advertisement_data(adv_data, adv_len, name, sizeof(name), &sps_type);

            // Only add devices with names (optional: filter for SPS only)
            if (strlen(name) > 0) {
                mutex_enter_blocking(&ble_mutex);
                ble_device_result_t *result = &scan_state.results[scan_state.count];
                memcpy(result->address, address, 6);
                result->address_type = address_type;
                strncpy(result->name, name, BLE_DEVICE_NAME_MAX_LEN);
                result->name[BLE_DEVICE_NAME_MAX_LEN] = '\0';
                result->rssi = rssi;
                result->sps_type = sps_type;
                result->has_sps_service = (sps_type != SPS_TYPE_UNKNOWN);
                scan_state.count++;
                mutex_exit(&ble_mutex);

                printf("Found: %s (%d dBm)%s\n", name, rssi,
                       result->has_sps_service ? " [SPS]" : "");
            }
            break;
        }

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    connection_state.connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    mutex_enter_blocking(&ble_mutex);
                    connection_state.connected = true;
                    mutex_exit(&ble_mutex);
                    printf("Connected, handle: 0x%04x\n", connection_state.connection_handle);

                    // Start service discovery
                    ble_discover_sps_service();
                    break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("Disconnected\n");
            mutex_enter_blocking(&ble_mutex);
            memset(&connection_state, 0, sizeof(connection_state));
            mutex_exit(&ble_mutex);
            break;
    }
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    gatt_client_service_t service;
    gatt_client_characteristic_t characteristic;

    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            gatt_event_service_query_result_get_service(packet, &service);

            // Check if this is Nordic NUS or u-blox SPS
            if (memcmp(service.uuid128, nordic_nus_service_uuid, 16) == 0) {
                printf("Found Nordic NUS service\n");
                connection_state.sps_service = service;
                connection_state.sps_type = SPS_TYPE_NORDIC_NUS;
            } else if (memcmp(service.uuid128, ublox_sps_service_uuid, 16) == 0) {
                printf("Found u-blox SPS service\n");
                connection_state.sps_service = service;
                connection_state.sps_type = SPS_TYPE_UBLOX_SPS;
            }
            break;

        case GATT_EVENT_QUERY_COMPLETE:
            if (connection_state.sps_type != SPS_TYPE_UNKNOWN && !connection_state.service_discovered) {
                connection_state.service_discovered = true;
                printf("Service discovery complete, discovering characteristics...\n");

                // Discover characteristics
                gatt_client_discover_characteristics_for_service(
                    handle_gatt_client_event,
                    connection_state.connection_handle,
                    &connection_state.sps_service
                );
            } else if (connection_state.service_discovered && !connection_state.characteristics_discovered) {
                connection_state.characteristics_discovered = true;
                printf("Characteristic discovery complete\n");

                // Enable notifications on TX characteristic
                if (connection_state.tx_value_handle != 0) {
                    gatt_client_write_client_characteristic_configuration(
                        handle_gatt_client_event,
                        connection_state.connection_handle,
                        &connection_state.tx_characteristic,
                        GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION
                    );
                }
            }
            break;

        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);

            // Check characteristic UUID
            if (connection_state.sps_type == SPS_TYPE_NORDIC_NUS) {
                if (memcmp(characteristic.uuid128, nordic_nus_tx_char_uuid, 16) == 0) {
                    printf("Found Nordic TX characteristic\n");
                    connection_state.tx_characteristic = characteristic;
                    connection_state.tx_value_handle = characteristic.value_handle;
                } else if (memcmp(characteristic.uuid128, nordic_nus_rx_char_uuid, 16) == 0) {
                    printf("Found Nordic RX characteristic\n");
                    connection_state.rx_characteristic = characteristic;
                    connection_state.rx_value_handle = characteristic.value_handle;
                }
            } else if (connection_state.sps_type == SPS_TYPE_UBLOX_SPS) {
                if (memcmp(characteristic.uuid128, ublox_sps_fifo_char_uuid, 16) == 0) {
                    printf("Found u-blox FIFO characteristic\n");
                    connection_state.tx_characteristic = characteristic;
                    connection_state.tx_value_handle = characteristic.value_handle;
                } else if (memcmp(characteristic.uuid128, ublox_sps_credits_char_uuid, 16) == 0) {
                    printf("Found u-blox Credits characteristic\n");
                    connection_state.rx_characteristic = characteristic;
                    connection_state.rx_value_handle = characteristic.value_handle;
                }
            }
            break;

        case GATT_EVENT_NOTIFICATION:
            if (data_callback != NULL) {
                uint16_t value_length = gatt_event_notification_get_value_length(packet);
                const uint8_t *value = gatt_event_notification_get_value(packet);
                data_callback(value, value_length);
            }
            break;

        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
            // Handle read responses if needed
            break;
    }
}

// =============================================================================
// Utility Functions
// =============================================================================

const char* ble_sps_type_to_string(sps_device_type_t type) {
    switch (type) {
        case SPS_TYPE_NORDIC_NUS:
            return "Nordic NUS";
        case SPS_TYPE_UBLOX_SPS:
            return "u-blox SPS";
        default:
            return "Unknown";
    }
}

void ble_address_to_string(const bd_addr_t address, char *str, size_t len) {
    snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
             address[0], address[1], address[2],
             address[3], address[4], address[5]);
}
