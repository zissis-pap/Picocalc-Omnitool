#include "weather_api.h"
#include "pico/stdlib.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WEATHER_API_HOST "api.openweathermap.org"
#define WEATHER_API_PORT 443
#define WEATHER_MAP_HOST "tile.openweathermap.org"

// Global weather data
static weather_data_t g_weather_data = {0};
static struct tcp_pcb *g_tcp_pcb = NULL;
static ip_addr_t g_server_ip;
static char g_request_buffer[1024];
static char g_response_buffer[16384];  // Larger for JSON + map data
static uint32_t g_response_len = 0;

// mbedTLS SSL context
static mbedtls_ssl_context g_ssl;
static mbedtls_ssl_config g_ssl_conf;
static mbedtls_entropy_context g_entropy;
static mbedtls_ctr_drbg_context g_ctr_drbg;
static bool g_ssl_initialized = false;
static bool g_handshake_done = false;

// Saved configuration
static char g_api_key[64] = {0};
static char g_current_host[64] = {0};

// Predefined cities array
static const weather_city_t predefined_cities[MAX_WEATHER_CITIES] = {
    {"New York", 40.7128, -74.0060},
    {"Los Angeles", 34.0522, -118.2437},
    {"London", 51.5074, -0.1278},
    {"Paris", 48.8566, 2.3522},
    {"Tokyo", 35.6762, 139.6503},
    {"Sydney", -33.8688, 151.2093},
    {"Athens", 37.9838, 23.7275},
    {"Mumbai", 19.0760, 72.8777},
    {"Dubai", 25.2048, 55.2708},
    {"Toronto", 43.6532, -79.3832}
};

// Forward declarations
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_client_err(void *arg, err_t err);
static void weather_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg);
static void parse_forecast_response(const char *response, uint32_t len);
static void parse_map_response(const char *response, uint32_t len);
static int ssl_send_callback(void *ctx, const unsigned char *buf, size_t len);
static int ssl_recv_callback(void *ctx, unsigned char *buf, size_t len);

// Forward declaration of SDK's hardware entropy function
extern int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen);

// Initialize weather API
void weather_api_init(void)
{
    memset(&g_weather_data, 0, sizeof(weather_data_t));
    g_weather_data.state = WEATHER_STATE_IDLE;
    g_weather_data.map_image_data = NULL;
    g_weather_data.map_loaded = false;
    g_ssl_initialized = false;
    g_handshake_done = false;
}

// Initialize SSL/TLS
static bool init_ssl(void)
{
    if (g_ssl_initialized) {
        return true;
    }

    int ret;

    mbedtls_ssl_init(&g_ssl);
    mbedtls_ssl_config_init(&g_ssl_conf);
    mbedtls_ctr_drbg_init(&g_ctr_drbg);
    mbedtls_entropy_init(&g_entropy);

    // Register the SDK's hardware entropy source
    ret = mbedtls_entropy_add_source(&g_entropy, mbedtls_hardware_poll, NULL,
                                      32, MBEDTLS_ENTROPY_SOURCE_STRONG);
    if (ret != 0) {
        printf("Failed to add entropy source: -0x%04x\n", -ret);
        return false;
    }

    // Seed the random number generator using hardware entropy
    const char *pers = "weather_client";
    ret = mbedtls_ctr_drbg_seed(&g_ctr_drbg, mbedtls_entropy_func, &g_entropy,
                                 (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        printf("mbedtls_ctr_drbg_seed failed: -0x%04x\n", -ret);
        return false;
    }

    ret = mbedtls_ssl_config_defaults(&g_ssl_conf,
                                       MBEDTLS_SSL_IS_CLIENT,
                                       MBEDTLS_SSL_TRANSPORT_STREAM,
                                       MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        printf("mbedtls_ssl_config_defaults failed: -0x%04x\n", -ret);
        return false;
    }

    // Skip certificate verification for simplicity
    mbedtls_ssl_conf_authmode(&g_ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&g_ssl_conf, mbedtls_ctr_drbg_random, &g_ctr_drbg);

    ret = mbedtls_ssl_setup(&g_ssl, &g_ssl_conf);
    if (ret != 0) {
        printf("mbedtls_ssl_setup failed: -0x%04x\n", -ret);
        return false;
    }

    // Hostname will be set dynamically before each connection
    // Set custom BIO callbacks for lwIP integration
    mbedtls_ssl_set_bio(&g_ssl, NULL, ssl_send_callback, ssl_recv_callback, NULL);

    g_ssl_initialized = true;
    printf("Weather SSL initialized successfully\n");
    return true;
}

// SSL send callback - writes to TCP
static int ssl_send_callback(void *ctx, const unsigned char *buf, size_t len)
{
    if (g_tcp_pcb == NULL) {
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }

    err_t err = tcp_write(g_tcp_pcb, buf, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("SSL send failed: %d\n", err);
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }

    tcp_output(g_tcp_pcb);
    return len;
}

// SSL receive callback - reads from response buffer
static int ssl_recv_callback(void *ctx, unsigned char *buf, size_t len)
{
    if (g_response_len == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    size_t copy_len = (len < g_response_len) ? len : g_response_len;
    memcpy(buf, g_response_buffer, copy_len);

    // Shift remaining data
    if (copy_len < g_response_len) {
        memmove(g_response_buffer, g_response_buffer + copy_len, g_response_len - copy_len);
    }
    g_response_len -= copy_len;

    return copy_len;
}

// DNS callback
static void weather_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr != NULL) {
        printf("Weather DNS resolved: %s\n", ipaddr_ntoa(ipaddr));
        g_server_ip = *ipaddr;

        // Close any existing connection first
        if (g_tcp_pcb != NULL) {
            printf("Closing existing TCP connection\n");
            tcp_abort(g_tcp_pcb);
            g_tcp_pcb = NULL;
        }

        // Create TCP connection
        g_tcp_pcb = tcp_new();
        if (g_tcp_pcb != NULL) {
            tcp_arg(g_tcp_pcb, NULL);
            tcp_err(g_tcp_pcb, tcp_client_err);
            err_t err = tcp_connect(g_tcp_pcb, &g_server_ip, WEATHER_API_PORT, tcp_client_connected);
            if (err != ERR_OK) {
                printf("Weather TCP connect failed: %d\n", err);
                g_weather_data.state = WEATHER_STATE_ERROR;
                snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                         "Connection failed");
                tcp_abort(g_tcp_pcb);
                g_tcp_pcb = NULL;
            }
        } else {
            printf("Failed to create Weather TCP PCB\n");
            g_weather_data.state = WEATHER_STATE_ERROR;
            snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                     "Out of TCP connections");
        }
    } else {
        printf("Weather DNS lookup failed for %s\n", name);
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "DNS lookup failed");
    }
}

// TCP connected callback
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK) {
        printf("Weather connection failed: %d\n", err);
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "Connection error");
        return err;
    }

    printf("Connected to weather server, starting TLS handshake\n");

    // Set up receive callback
    tcp_recv(tpcb, tcp_client_recv);

    // Initialize SSL if not already done
    if (!init_ssl()) {
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "SSL init failed");
        return ERR_ABRT;
    }

    // Set hostname for this connection
    mbedtls_ssl_set_hostname(&g_ssl, g_current_host);

    // Reset SSL session for new connection
    mbedtls_ssl_session_reset(&g_ssl);
    g_handshake_done = false;

    // Start TLS handshake
    int ret;
    while ((ret = mbedtls_ssl_handshake(&g_ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("TLS handshake failed: -0x%04x\n", -ret);
            g_weather_data.state = WEATHER_STATE_ERROR;
            snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                     "TLS handshake failed");
            return ERR_ABRT;
        }
        // Handshake in progress, will continue when more data arrives
        return ERR_OK;
    }

    printf("TLS handshake completed\n");
    g_handshake_done = true;

    // Send HTTP request over TLS
    int write_ret = mbedtls_ssl_write(&g_ssl, (unsigned char *)g_request_buffer, strlen(g_request_buffer));
    if (write_ret < 0) {
        printf("SSL write failed: -0x%04x\n", -write_ret);
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "Failed to send request");
        return ERR_ABRT;
    }

    printf("HTTPS request sent (%d bytes)\n", write_ret);
    return ERR_OK;
}

// TCP receive callback
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL) {
        // Connection closed by server
        printf("Weather connection closed by server\n");
        tcp_close(tpcb);
        g_tcp_pcb = NULL;

        // Parse the final response if we have data
        if (g_response_len > 0) {
            if (g_weather_data.current_request == WEATHER_REQUEST_FORECAST) {
                parse_forecast_response(g_response_buffer, g_response_len);
            } else if (g_weather_data.current_request == WEATHER_REQUEST_MAP) {
                parse_map_response(g_response_buffer, g_response_len);
            }
        }

        // Reset for next request
        g_response_len = 0;
        g_handshake_done = false;

        return ERR_OK;
    }

    if (err != ERR_OK) {
        printf("Weather receive error: %d\n", err);
        pbuf_free(p);
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "Receive error");
        return err;
    }

    // Copy data to response buffer for SSL processing
    uint32_t copy_len = p->tot_len;
    if (g_response_len + copy_len > sizeof(g_response_buffer) - 1) {
        copy_len = sizeof(g_response_buffer) - 1 - g_response_len;
    }

    pbuf_copy_partial(p, g_response_buffer + g_response_len, copy_len, 0);
    g_response_len += copy_len;

    // Tell lwIP we've processed the data
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    // Continue TLS handshake if not done
    if (!g_handshake_done) {
        int ret = mbedtls_ssl_handshake(&g_ssl);
        if (ret == 0) {
            printf("TLS handshake completed\n");
            g_handshake_done = true;

            // Send HTTP request now that handshake is done
            int write_ret = mbedtls_ssl_write(&g_ssl, (unsigned char *)g_request_buffer, strlen(g_request_buffer));
            if (write_ret < 0) {
                printf("SSL write failed: -0x%04x\n", -write_ret);
                g_weather_data.state = WEATHER_STATE_ERROR;
                snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                         "Failed to send request");
            } else {
                printf("HTTPS request sent (%d bytes)\n", write_ret);
            }
        } else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("TLS handshake failed: -0x%04x\n", -ret);
            g_weather_data.state = WEATHER_STATE_ERROR;
            snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                     "TLS handshake failed");
        }
    } else {
        // Handshake done, read decrypted data
        char decrypt_buf[2048];
        int read_ret = mbedtls_ssl_read(&g_ssl, (unsigned char *)decrypt_buf, sizeof(decrypt_buf) - 1);
        if (read_ret > 0) {
            decrypt_buf[read_ret] = '\0';
            printf("Received %d bytes of decrypted data\n", read_ret);

            // Store decrypted response
            if (g_response_len + read_ret < sizeof(g_response_buffer)) {
                memcpy(g_response_buffer + g_response_len, decrypt_buf, read_ret);
                g_response_len += read_ret;
                g_response_buffer[g_response_len] = '\0';
            }
        }
    }

    return ERR_OK;
}

// TCP error callback
static void tcp_client_err(void *arg, err_t err)
{
    printf("Weather TCP error: %d\n", err);
    g_weather_data.state = WEATHER_STATE_ERROR;
    snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
             "Network error");
    g_tcp_pcb = NULL;
}

// Fetch weather forecast
void weather_api_fetch_forecast(const char *api_key, const char *city)
{
    printf("Fetching weather forecast for: %s\n", city);

    // Save API key for subsequent map request
    strncpy(g_api_key, api_key, sizeof(g_api_key) - 1);
    g_api_key[sizeof(g_api_key) - 1] = '\0';

    // Set state
    g_weather_data.state = WEATHER_STATE_FETCHING_FORECAST;
    g_weather_data.current_request = WEATHER_REQUEST_FORECAST;
    strncpy(g_weather_data.city_name, city, WEATHER_CITY_NAME_MAX - 1);
    g_weather_data.city_name[WEATHER_CITY_NAME_MAX - 1] = '\0';
    g_weather_data.forecast_count = 0;

    // Build HTTPS GET request
    snprintf(g_request_buffer, sizeof(g_request_buffer),
             "GET /data/2.5/forecast?q=%s&appid=%s&units=metric&cnt=16 HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             city, api_key, WEATHER_API_HOST);

    // Reset response buffer
    g_response_len = 0;
    memset(g_response_buffer, 0, sizeof(g_response_buffer));

    // Set current host
    strncpy(g_current_host, WEATHER_API_HOST, sizeof(g_current_host) - 1);
    g_current_host[sizeof(g_current_host) - 1] = '\0';

    // Start DNS lookup
    err_t err = dns_gethostbyname(WEATHER_API_HOST, &g_server_ip,
                                   weather_dns_found, NULL);
    if (err == ERR_OK) {
        weather_dns_found(WEATHER_API_HOST, &g_server_ip, NULL);
    } else if (err != ERR_INPROGRESS) {
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "DNS lookup failed");
    }
}

// Fetch weather map
void weather_api_fetch_map(const char *api_key, float lat, float lon)
{
    printf("Fetching weather map for: %.4f, %.4f\n", lat, lon);

    g_weather_data.state = WEATHER_STATE_FETCHING_MAP;
    g_weather_data.current_request = WEATHER_REQUEST_MAP;

    // Allocate map image buffer if not done
    if (g_weather_data.map_image_data == NULL) {
        g_weather_data.map_image_data = (uint8_t *)malloc(WEATHER_MAP_IMAGE_MAX_SIZE);
        if (g_weather_data.map_image_data == NULL) {
            printf("Failed to allocate map buffer\n");
            g_weather_data.state = WEATHER_STATE_ERROR;
            snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                     "Out of memory");
            return;
        }
    }

    // Build HTTPS GET request for PNG map
    // Using temperature layer at zoom 5
    int x = (int)((lon + 180.0) / 360.0 * (1 << 5));
    int y = (int)((1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * (1 << 5));

    snprintf(g_request_buffer, sizeof(g_request_buffer),
             "GET /map/temp_new/5/%d/%d.png?appid=%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             x, y, api_key, WEATHER_MAP_HOST);

    g_response_len = 0;
    memset(g_response_buffer, 0, sizeof(g_response_buffer));

    // Set current host
    strncpy(g_current_host, WEATHER_MAP_HOST, sizeof(g_current_host) - 1);
    g_current_host[sizeof(g_current_host) - 1] = '\0';

    // DNS lookup for tile server
    err_t err = dns_gethostbyname(WEATHER_MAP_HOST, &g_server_ip,
                                   weather_dns_found, NULL);
    if (err == ERR_OK) {
        weather_dns_found(WEATHER_MAP_HOST, &g_server_ip, NULL);
    } else if (err != ERR_INPROGRESS) {
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "DNS lookup failed");
    }
}

// Parse forecast response
static void parse_forecast_response(const char *response, uint32_t len)
{
    printf("Parsing weather forecast response (%d bytes)\n", len);

    // Find JSON body (skip HTTP headers)
    const char *json_start = strstr(response, "\r\n\r\n");
    if (!json_start) {
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "Invalid response");
        return;
    }
    json_start += 4;

    // Check for API errors
    if (strstr(json_start, "\"cod\":\"404\"") != NULL) {
        const char *msg = strstr(json_start, "\"message\":\"");
        if (msg) {
            msg += 11;
            const char *msg_end = strchr(msg, '"');
            if (msg_end) {
                int msg_len = msg_end - msg;
                if (msg_len > 127) msg_len = 127;
                strncpy(g_weather_data.error_message, msg, msg_len);
                g_weather_data.error_message[msg_len] = '\0';
            }
        } else {
            snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                     "City not found");
        }
        g_weather_data.state = WEATHER_STATE_ERROR;
        return;
    }

    if (strstr(json_start, "\"cod\":401") != NULL) {
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "Invalid API key");
        return;
    }

    // Extract city coordinates (for map request)
    const char *coord = strstr(json_start, "\"coord\":{");
    if (coord) {
        const char *lat_pos = strstr(coord, "\"lat\":");
        const char *lon_pos = strstr(coord, "\"lon\":");
        if (lat_pos && lon_pos) {
            g_weather_data.latitude = atof(lat_pos + 6);
            g_weather_data.longitude = atof(lon_pos + 6);
            printf("City coordinates: %.4f, %.4f\n", g_weather_data.latitude, g_weather_data.longitude);
        }
    }

    // Parse forecast list array
    const char *list_start = strstr(json_start, "\"list\":[");
    if (!list_start) {
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "Invalid forecast data");
        return;
    }
    list_start += 8;

    g_weather_data.forecast_count = 0;
    const char *search_pos = list_start;

    while (g_weather_data.forecast_count < MAX_WEATHER_FORECASTS) {
        // Find next forecast object
        search_pos = strstr(search_pos, "\"dt\":");
        if (!search_pos) break;

        weather_forecast_t *fc = &g_weather_data.forecasts[g_weather_data.forecast_count];

        // Parse timestamp
        fc->timestamp = (time_t)atol(search_pos + 5);

        // Parse temperature: "main":{"temp":15.3,...
        const char *temp_pos = strstr(search_pos, "\"temp\":");
        if (temp_pos && temp_pos < search_pos + 500) {
            fc->temp = atof(temp_pos + 7);
        }

        // Parse feels_like
        const char *feels_pos = strstr(search_pos, "\"feels_like\":");
        if (feels_pos && feels_pos < search_pos + 500) {
            fc->feels_like = atof(feels_pos + 13);
        }

        // Parse humidity
        const char *humid_pos = strstr(search_pos, "\"humidity\":");
        if (humid_pos && humid_pos < search_pos + 500) {
            fc->humidity = atoi(humid_pos + 11);
        }

        // Parse weather description: "weather":[{"description":"light rain"
        const char *desc_pos = strstr(search_pos, "\"description\":\"");
        if (desc_pos && desc_pos < search_pos + 600) {
            desc_pos += 15;
            const char *desc_end = strchr(desc_pos, '"');
            if (desc_end) {
                int desc_len = desc_end - desc_pos;
                if (desc_len >= WEATHER_DESCRIPTION_MAX) desc_len = WEATHER_DESCRIPTION_MAX - 1;
                strncpy(fc->description, desc_pos, desc_len);
                fc->description[desc_len] = '\0';
            }
        }

        // Parse icon code: "icon":"10d"
        const char *icon_pos = strstr(search_pos, "\"icon\":\"");
        if (icon_pos && icon_pos < search_pos + 600) {
            icon_pos += 8;
            strncpy(fc->icon, icon_pos, 3);  // "10d" = 3 chars
            fc->icon[3] = '\0';
        }

        g_weather_data.forecast_count++;
        search_pos += 100;  // Move forward to avoid re-parsing
    }

    printf("Parsed %d forecasts\n", g_weather_data.forecast_count);

    // Now fetch map with coordinates
    if (g_weather_data.latitude != 0.0 && g_weather_data.longitude != 0.0) {
        weather_api_fetch_map(g_api_key, g_weather_data.latitude, g_weather_data.longitude);
    } else {
        // Skip map if we don't have coordinates
        g_weather_data.state = WEATHER_STATE_SUCCESS;
        g_weather_data.map_loaded = false;
    }
}

// Parse map response
static void parse_map_response(const char *response, uint32_t len)
{
    printf("Parsing map image response (%d bytes)\n", len);

    // Find PNG binary data (skip HTTP headers)
    const char *png_start = strstr(response, "\r\n\r\n");
    if (!png_start) {
        g_weather_data.state = WEATHER_STATE_ERROR;
        snprintf(g_weather_data.error_message, sizeof(g_weather_data.error_message),
                 "Invalid map response");
        return;
    }
    png_start += 4;

    // Check PNG magic header (89 50 4E 47)
    if ((unsigned char)png_start[0] != 0x89 || png_start[1] != 'P' ||
        png_start[2] != 'N' || png_start[3] != 'G') {
        printf("Not a valid PNG image\n");
        // Don't fail completely, just skip the map
        g_weather_data.map_loaded = false;
        g_weather_data.state = WEATHER_STATE_SUCCESS;
        return;
    }

    // Calculate PNG size
    uint32_t png_size = len - (png_start - response);
    if (png_size > WEATHER_MAP_IMAGE_MAX_SIZE) {
        printf("Warning: Map image truncated\n");
        png_size = WEATHER_MAP_IMAGE_MAX_SIZE;
    }

    // Copy PNG data to buffer
    memcpy(g_weather_data.map_image_data, png_start, png_size);
    g_weather_data.map_image_size = png_size;
    g_weather_data.map_loaded = true;

    g_weather_data.state = WEATHER_STATE_SUCCESS;
    printf("Map loaded successfully: %d bytes\n", png_size);
}

// Get weather emoji from icon code
const char* weather_get_emoji(const char *icon_code)
{
    if (!icon_code || strlen(icon_code) < 2) return "?";

    // Map OpenWeather icon codes to simple text icons
    // 01d/01n = clear sky, 02d/02n = few clouds, 03d/03n = scattered clouds
    // 04d/04n = broken clouds, 09d/09n = shower rain, 10d/10n = rain
    // 11d/11n = thunderstorm, 13d/13n = snow, 50d/50n = mist

    if (icon_code[0] == '0' && icon_code[1] == '1') {
        return "*";  // Clear sky / sun
    } else if (icon_code[0] == '0' && (icon_code[1] == '2' || icon_code[1] == '3' || icon_code[1] == '4')) {
        return "o";  // Clouds
    } else if (icon_code[0] == '0' && icon_code[1] == '9') {
        return "~";  // Shower rain
    } else if (icon_code[0] == '1' && icon_code[1] == '0') {
        return "~";  // Rain
    } else if (icon_code[0] == '1' && icon_code[1] == '1') {
        return "!";  // Thunderstorm
    } else if (icon_code[0] == '1' && icon_code[1] == '3') {
        return "S";  // Snow
    } else if (icon_code[0] == '5' && icon_code[1] == '0') {
        return "F";  // Fog/Mist
    }

    return "?";
}

// Get predefined cities array
const weather_city_t* weather_get_cities(void)
{
    return predefined_cities;
}

// Get weather data
weather_data_t* weather_api_get_data(void)
{
    return &g_weather_data;
}

// Get weather state
weather_api_state_t weather_api_get_state(void)
{
    return g_weather_data.state;
}

// Cleanup
void weather_api_cleanup(void)
{
    if (g_weather_data.map_image_data != NULL) {
        free(g_weather_data.map_image_data);
        g_weather_data.map_image_data = NULL;
    }
    g_weather_data.map_loaded = false;
    g_weather_data.map_image_size = 0;
}
