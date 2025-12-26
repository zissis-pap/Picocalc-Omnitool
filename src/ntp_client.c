#include "ntp_client.h"
#include "pico/stdlib.h"
#include "lwip/dns.h"
#include "lwip/udp.h"
#include <string.h>
#include <stdio.h>

// NTP server defaults
#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT 123
#define NTP_DELTA 2208988800ULL  // Seconds between 1900 and 1970

// NTP packet structure (48 bytes)
typedef struct {
    uint8_t li_vn_mode;      // Leap indicator, Version number, Mode
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_ts_sec;
    uint32_t ref_ts_frac;
    uint32_t orig_ts_sec;
    uint32_t orig_ts_frac;
    uint32_t recv_ts_sec;
    uint32_t recv_ts_frac;
    uint32_t tx_ts_sec;
    uint32_t tx_ts_frac;
} ntp_packet_t;

// Global state
static ntp_sync_state_t g_ntp_state = NTP_STATE_IDLE;
static struct udp_pcb *g_ntp_pcb = NULL;
static ip_addr_t g_ntp_server_ip;
static time_t g_sync_time = 0;        // Time when we synced (Unix timestamp)
static uint64_t g_sync_us = 0;        // Microsecond counter when we synced
static ntp_sync_callback_t g_sync_callback = NULL;

// Forward declarations
static void ntp_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg);
static void ntp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void ntp_send_request(void);

// Initialize NTP client
void ntp_client_init(void)
{
    g_ntp_state = NTP_STATE_IDLE;
    g_sync_time = 0;
    g_sync_us = 0;
    g_sync_callback = NULL;
}

// DNS callback
static void ntp_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr != NULL) {
        printf("NTP DNS resolved: %s\n", ipaddr_ntoa(ipaddr));
        g_ntp_server_ip = *ipaddr;
        ntp_send_request();
    } else {
        printf("NTP DNS lookup failed for %s\n", name);
        g_ntp_state = NTP_STATE_ERROR;
        if (g_sync_callback) {
            g_sync_callback(false);
        }
    }
}

// Send NTP request
static void ntp_send_request(void)
{
    if (g_ntp_pcb == NULL) {
        g_ntp_pcb = udp_new();
        if (g_ntp_pcb == NULL) {
            printf("Failed to create UDP PCB for NTP\n");
            g_ntp_state = NTP_STATE_ERROR;
            if (g_sync_callback) {
                g_sync_callback(false);
            }
            return;
        }

        // Bind to any local port
        err_t err = udp_bind(g_ntp_pcb, IP_ADDR_ANY, 0);
        if (err != ERR_OK) {
            printf("UDP bind failed: %d\n", err);
            udp_remove(g_ntp_pcb);
            g_ntp_pcb = NULL;
            g_ntp_state = NTP_STATE_ERROR;
            if (g_sync_callback) {
                g_sync_callback(false);
            }
            return;
        }

        // Set receive callback
        udp_recv(g_ntp_pcb, ntp_recv_callback, NULL);
    }

    // Create NTP request packet
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(ntp_packet_t), PBUF_RAM);
    if (p == NULL) {
        printf("Failed to allocate pbuf for NTP\n");
        g_ntp_state = NTP_STATE_ERROR;
        if (g_sync_callback) {
            g_sync_callback(false);
        }
        return;
    }

    ntp_packet_t *packet = (ntp_packet_t *)p->payload;
    memset(packet, 0, sizeof(ntp_packet_t));

    // Set version (4) and mode (3 = client)
    packet->li_vn_mode = 0x23; // 00 100 011

    // Send request
    err_t err = udp_sendto(g_ntp_pcb, p, &g_ntp_server_ip, NTP_PORT);
    pbuf_free(p);

    if (err == ERR_OK) {
        printf("NTP request sent\n");
        g_ntp_state = NTP_STATE_REQUESTING;
    } else {
        printf("NTP send failed: %d\n", err);
        g_ntp_state = NTP_STATE_ERROR;
        if (g_sync_callback) {
            g_sync_callback(false);
        }
    }
}

// Byte swap for network to host conversion
static uint32_t ntohl_custom(uint32_t n)
{
    return ((n & 0x000000FF) << 24) |
           ((n & 0x0000FF00) << 8) |
           ((n & 0x00FF0000) >> 8) |
           ((n & 0xFF000000) >> 24);
}

// UDP receive callback
static void ntp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    if (p == NULL) {
        return;
    }

    if (p->len >= sizeof(ntp_packet_t)) {
        ntp_packet_t *packet = (ntp_packet_t *)p->payload;

        // Extract timestamp (seconds since 1900)
        uint32_t ntp_time = ntohl_custom(packet->tx_ts_sec);

        // Convert to Unix timestamp (seconds since 1970)
        g_sync_time = (time_t)(ntp_time - NTP_DELTA);
        g_sync_us = time_us_64();  // Store microsecond counter at sync time

        printf("NTP time received: %lu (Unix: %lu)\n", ntp_time, g_sync_time);

        struct tm *timeinfo = gmtime(&g_sync_time);
        if (timeinfo != NULL) {
            printf("Time synced: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                   timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                   timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            g_ntp_state = NTP_STATE_SYNCED;

            if (g_sync_callback) {
                g_sync_callback(true);
            }
        } else {
            printf("Failed to convert time\n");
            g_ntp_state = NTP_STATE_ERROR;
            if (g_sync_callback) {
                g_sync_callback(false);
            }
        }
    }

    pbuf_free(p);
}

// Request time from default NTP server
void ntp_client_request(void)
{
    ntp_client_request_server(NTP_SERVER);
}

// Request time from custom server
void ntp_client_request_server(const char *server)
{
    if (g_ntp_state == NTP_STATE_REQUESTING) {
        printf("NTP request already in progress\n");
        return;
    }

    printf("Requesting time from NTP server: %s\n", server);
    g_ntp_state = NTP_STATE_REQUESTING;

    // Start DNS lookup
    err_t err = dns_gethostbyname(server, &g_ntp_server_ip, ntp_dns_found, NULL);

    if (err == ERR_OK) {
        // DNS already cached
        printf("NTP DNS cached, sending request immediately\n");
        ntp_send_request();
    } else if (err != ERR_INPROGRESS) {
        printf("NTP DNS lookup failed: %d\n", err);
        g_ntp_state = NTP_STATE_ERROR;
        if (g_sync_callback) {
            g_sync_callback(false);
        }
    }
}

// Get current sync state
ntp_sync_state_t ntp_client_get_state(void)
{
    return g_ntp_state;
}

// Get current time
bool ntp_client_get_time(struct tm *timeinfo)
{
    if (g_ntp_state != NTP_STATE_SYNCED || g_sync_us == 0) {
        return false;
    }

    // Calculate current time based on elapsed microseconds
    uint64_t now_us = time_us_64();
    uint64_t elapsed_us = now_us - g_sync_us;
    time_t elapsed_sec = (time_t)(elapsed_us / 1000000ULL);
    time_t current_time = g_sync_time + elapsed_sec;

    // Convert to tm structure
    struct tm *result = gmtime(&current_time);
    if (result == NULL) {
        return false;
    }

    *timeinfo = *result;
    return true;
}

// Get Unix timestamp
time_t ntp_client_get_timestamp(void)
{
    if (g_ntp_state != NTP_STATE_SYNCED || g_sync_us == 0) {
        return 0;
    }

    // Calculate current time based on elapsed microseconds
    uint64_t now_us = time_us_64();
    uint64_t elapsed_us = now_us - g_sync_us;
    time_t elapsed_sec = (time_t)(elapsed_us / 1000000ULL);

    return g_sync_time + elapsed_sec;
}

// Set callback
void ntp_client_set_callback(ntp_sync_callback_t callback)
{
    g_sync_callback = callback;
}
