#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// NTP sync state
typedef enum {
    NTP_STATE_IDLE,
    NTP_STATE_REQUESTING,
    NTP_STATE_SYNCED,
    NTP_STATE_ERROR
} ntp_sync_state_t;

// NTP callback type
typedef void (*ntp_sync_callback_t)(bool success);

// Initialize NTP client
void ntp_client_init(void);

// Request time from NTP server (non-blocking)
// Default server: pool.ntp.org
void ntp_client_request(void);

// Request time with custom server
void ntp_client_request_server(const char *server);

// Get current sync state
ntp_sync_state_t ntp_client_get_state(void);

// Get current time (returns true if time is valid/synced)
bool ntp_client_get_time(struct tm *timeinfo);

// Get Unix timestamp (seconds since 1970-01-01)
time_t ntp_client_get_timestamp(void);

// Set callback for sync completion
void ntp_client_set_callback(ntp_sync_callback_t callback);

#endif // NTP_CLIENT_H
