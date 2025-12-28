#include "telegram_api.h"
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TELEGRAM_API_HOST "api.telegram.org"
#define TELEGRAM_API_PORT 443

// Global telegram data
static telegram_data_t g_telegram_data = {0};
static struct tcp_pcb *g_tcp_pcb = NULL;
static ip_addr_t g_server_ip;
static char g_request_buffer[1024];
static char g_response_buffer[8192];
static uint16_t g_response_len = 0;

// mbedTLS SSL context
static mbedtls_ssl_context g_ssl;
static mbedtls_ssl_config g_ssl_conf;
static mbedtls_entropy_context g_entropy;
static mbedtls_ctr_drbg_context g_ctr_drbg;
static bool g_ssl_initialized = false;
static bool g_handshake_done = false;

// Saved configuration for reconnection
static char g_bot_token[128] = {0};

// Request type (for handling response appropriately)
typedef enum {
    REQUEST_TYPE_SEND_MESSAGE,
    REQUEST_TYPE_GET_UPDATES
} request_type_t;

static request_type_t g_current_request_type = REQUEST_TYPE_GET_UPDATES;

// Forward declarations
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_client_err(void *arg, err_t err);
static void telegram_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg);
static void parse_telegram_response(const char *response, uint16_t len);
static void parse_get_updates_response(const char *json);
static void parse_send_message_response(const char *json);
static void url_encode(const char *input, char *output, size_t output_size);
static int64_t parse_int64(const char *str);
static int ssl_send_callback(void *ctx, const unsigned char *buf, size_t len);
static int ssl_recv_callback(void *ctx, unsigned char *buf, size_t len);

// Forward declaration of SDK's hardware entropy function
extern int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen);

// Initialize telegram API
void telegram_api_init(void)
{
    memset(&g_telegram_data, 0, sizeof(telegram_data_t));
    g_telegram_data.state = TELEGRAM_STATE_IDLE;
    g_telegram_data.last_update_id = 0;
    g_telegram_data.polling_active = false;
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
    const char *pers = "telegram_client";
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

    // Skip certificate verification for simplicity (not recommended for production!)
    mbedtls_ssl_conf_authmode(&g_ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&g_ssl_conf, mbedtls_ctr_drbg_random, &g_ctr_drbg);

    ret = mbedtls_ssl_setup(&g_ssl, &g_ssl_conf);
    if (ret != 0) {
        printf("mbedtls_ssl_setup failed: -0x%04x\n", -ret);
        return false;
    }

    ret = mbedtls_ssl_set_hostname(&g_ssl, TELEGRAM_API_HOST);
    if (ret != 0) {
        printf("mbedtls_ssl_set_hostname failed: -0x%04x\n", -ret);
        return false;
    }

    // Set custom BIO callbacks for lwIP integration
    mbedtls_ssl_set_bio(&g_ssl, NULL, ssl_send_callback, ssl_recv_callback, NULL);

    g_ssl_initialized = true;
    printf("SSL initialized successfully\n");
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
static void telegram_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr != NULL) {
        printf("Telegram DNS resolved: %s\n", ipaddr_ntoa(ipaddr));
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
            err_t err = tcp_connect(g_tcp_pcb, &g_server_ip, TELEGRAM_API_PORT, tcp_client_connected);
            if (err != ERR_OK) {
                printf("Telegram TCP connect failed: %d\n", err);
                g_telegram_data.state = TELEGRAM_STATE_ERROR;
                snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                         "Connection failed");
                tcp_abort(g_tcp_pcb);
                g_tcp_pcb = NULL;
            }
        } else {
            printf("Failed to create Telegram TCP PCB (out of memory/connections)\n");
            g_telegram_data.state = TELEGRAM_STATE_ERROR;
            snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                     "Out of TCP connections");
        }
    } else {
        printf("Telegram DNS lookup failed for %s\n", name);
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "DNS lookup failed");
    }
}

// TCP connected callback
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK) {
        printf("Telegram connection failed: %d\n", err);
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "Connection error");
        return err;
    }

    printf("Connected to Telegram server, starting TLS handshake\n");

    // Set up receive callback
    tcp_recv(tpcb, tcp_client_recv);

    // Initialize SSL if not already done
    if (!init_ssl()) {
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "SSL init failed");
        return ERR_ABRT;
    }

    // Reset SSL session for new connection
    mbedtls_ssl_session_reset(&g_ssl);
    g_handshake_done = false;

    // Start TLS handshake
    int ret;
    while ((ret = mbedtls_ssl_handshake(&g_ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("TLS handshake failed: -0x%04x\n", -ret);
            g_telegram_data.state = TELEGRAM_STATE_ERROR;
            snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
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
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
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
        printf("Telegram connection closed by server\n");
        tcp_close(tpcb);
        g_tcp_pcb = NULL;

        // Parse the final response if we have data
        if (g_response_len > 0) {
            parse_telegram_response(g_response_buffer, g_response_len);
        }

        // Reset for next request
        g_response_len = 0;
        g_handshake_done = false;

        return ERR_OK;
    }

    if (err != ERR_OK) {
        printf("Telegram receive error: %d\n", err);
        pbuf_free(p);
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "Receive error");
        return err;
    }

    // Copy data to response buffer for SSL processing
    uint16_t copy_len = p->tot_len;
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
                g_telegram_data.state = TELEGRAM_STATE_ERROR;
                snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                         "Failed to send request");
            } else {
                printf("HTTPS request sent (%d bytes)\n", write_ret);
            }
        } else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("TLS handshake failed: -0x%04x\n", -ret);
            g_telegram_data.state = TELEGRAM_STATE_ERROR;
            snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                     "TLS handshake failed");
        }
    } else {
        // Handshake done, read decrypted data
        char decrypt_buf[1024];
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
    printf("Telegram TCP error: %d\n", err);
    g_telegram_data.state = TELEGRAM_STATE_ERROR;
    snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
             "Network error");
    g_tcp_pcb = NULL;
}

// Parse telegram response (dispatch to specific parser)
static void parse_telegram_response(const char *response, uint16_t len)
{
    printf("Parsing Telegram response (%d bytes)\n", len);

    // Find the JSON body (skip HTTP headers)
    const char *json_start = strstr(response, "\r\n\r\n");
    if (json_start == NULL) {
        printf("No JSON body found in Telegram response\n");
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "Invalid response");
        return;
    }
    json_start += 4; // Skip past "\r\n\r\n"

    // Check if API returned error
    if (strstr(json_start, "\"ok\":false") != NULL) {
        printf("Telegram API returned error\n");
        g_telegram_data.state = TELEGRAM_STATE_ERROR;

        // Try to extract error description
        const char *desc_start = strstr(json_start, "\"description\":\"");
        if (desc_start != NULL) {
            desc_start += 15;
            const char *desc_end = strchr(desc_start, '"');
            if (desc_end != NULL) {
                int desc_len = desc_end - desc_start;
                if (desc_len > 127) desc_len = 127;
                strncpy(g_telegram_data.error_message, desc_start, desc_len);
                g_telegram_data.error_message[desc_len] = '\0';
            }
        } else {
            snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                     "API error");
        }
        return;
    }

    // Dispatch to specific parser based on request type
    if (g_current_request_type == REQUEST_TYPE_GET_UPDATES) {
        parse_get_updates_response(json_start);
    } else if (g_current_request_type == REQUEST_TYPE_SEND_MESSAGE) {
        parse_send_message_response(json_start);
    }
}

// Parse getUpdates response
static void parse_get_updates_response(const char *json)
{
    printf("Parsing getUpdates response\n");

    // Find "result" array
    const char *result_start = strstr(json, "\"result\":[");
    if (result_start == NULL) {
        printf("No result array in getUpdates response\n");
        g_telegram_data.state = TELEGRAM_STATE_SUCCESS;
        return;
    }
    result_start += 10; // Skip past "result":[

    // Parse each update in the array
    const char *search_pos = result_start;

    while (g_telegram_data.message_count < MAX_TELEGRAM_MESSAGES) {
        // Find next "update_id"
        search_pos = strstr(search_pos, "\"update_id\":");
        if (search_pos == NULL) break;

        search_pos += 12; // Skip past "update_id":

        // Parse update_id
        int64_t update_id = parse_int64(search_pos);
        if (update_id > g_telegram_data.last_update_id) {
            g_telegram_data.last_update_id = update_id;
        }

        // Find message object
        const char *msg_start = strstr(search_pos, "\"message\":{");
        if (msg_start == NULL) {
            // No message in this update (could be edited_message, etc.)
            search_pos = strstr(search_pos, "}");
            if (search_pos) search_pos++;
            continue;
        }

        // Parse message_id
        const char *msg_id_pos = strstr(msg_start, "\"message_id\":");
        if (msg_id_pos == NULL) continue;
        msg_id_pos += 13;
        int64_t message_id = parse_int64(msg_id_pos);

        // Parse chat id
        const char *chat_id_pos = strstr(msg_start, "\"chat\":{\"id\":");
        if (chat_id_pos == NULL) continue;
        chat_id_pos += 13;
        int64_t chat_id = parse_int64(chat_id_pos);

        // Parse username
        char username[TELEGRAM_USERNAME_MAX + 1] = {0};
        const char *username_pos = strstr(msg_start, "\"username\":\"");
        if (username_pos != NULL) {
            username_pos += 12;
            const char *username_end = strchr(username_pos, '"');
            if (username_end != NULL) {
                int username_len = username_end - username_pos;
                if (username_len > TELEGRAM_USERNAME_MAX) username_len = TELEGRAM_USERNAME_MAX;
                strncpy(username, username_pos, username_len);
                username[username_len] = '\0';
            }
        }

        // Parse text
        char text[TELEGRAM_MESSAGE_TEXT_MAX + 1] = {0};
        const char *text_pos = strstr(msg_start, "\"text\":\"");
        if (text_pos != NULL) {
            text_pos += 8;

            // Find end of text, handling escaped quotes
            const char *text_end = text_pos;
            while (*text_end != '\0') {
                if (*text_end == '\\' && *(text_end + 1) != '\0') {
                    text_end += 2;
                } else if (*text_end == '"') {
                    break;
                } else {
                    text_end++;
                }
            }

            if (*text_end == '"') {
                int text_len = text_end - text_pos;
                if (text_len > TELEGRAM_MESSAGE_TEXT_MAX) text_len = TELEGRAM_MESSAGE_TEXT_MAX;
                strncpy(text, text_pos, text_len);
                text[text_len] = '\0';

                // Clean up escaped characters
                int write_pos = 0;
                for (int read_pos = 0; read_pos < text_len; read_pos++) {
                    if (text[read_pos] == '\\' && read_pos + 1 < text_len) {
                        char next = text[read_pos + 1];
                        if (next == '"' || next == '\\' || next == '/' || next == 'n') {
                            if (next == 'n') {
                                text[write_pos++] = '\n';
                            } else {
                                text[write_pos++] = next;
                            }
                            read_pos++;
                        } else {
                            text[write_pos++] = text[read_pos];
                        }
                    } else {
                        text[write_pos++] = text[read_pos];
                    }
                }
                text[write_pos] = '\0';
            }
        }

        // Parse timestamp
        time_t timestamp = 0;
        const char *date_pos = strstr(msg_start, "\"date\":");
        if (date_pos != NULL) {
            date_pos += 7;
            timestamp = (time_t)parse_int64(date_pos);
        }

        // Add message to buffer
        if (strlen(text) > 0) {
            telegram_message_t *msg = &g_telegram_data.messages[g_telegram_data.message_count];
            msg->message_id = message_id;
            msg->chat_id = chat_id;
            strncpy(msg->username, username, TELEGRAM_USERNAME_MAX);
            msg->username[TELEGRAM_USERNAME_MAX] = '\0';
            strncpy(msg->text, text, TELEGRAM_MESSAGE_TEXT_MAX);
            msg->text[TELEGRAM_MESSAGE_TEXT_MAX] = '\0';
            msg->timestamp = timestamp;

            g_telegram_data.message_count++;
            printf("Parsed message from @%s: %s\n", username, text);
        }

        search_pos = msg_start + 1;
    }

    g_telegram_data.state = TELEGRAM_STATE_SUCCESS;
    printf("Parsed %d messages\n", g_telegram_data.message_count);
}

// Parse sendMessage response
static void parse_send_message_response(const char *json)
{
    printf("Parsing sendMessage response\n");

    // Check if message was sent successfully
    if (strstr(json, "\"ok\":true") != NULL) {
        printf("Message sent successfully\n");
        g_telegram_data.state = TELEGRAM_STATE_SUCCESS;
    } else {
        printf("Failed to send message\n");
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "Failed to send message");
    }
}

// URL encode a string (for message text in POST body)
static void url_encode(const char *input, char *output, size_t output_size)
{
    const char *hex = "0123456789ABCDEF";
    size_t pos = 0;

    while (*input && pos < output_size - 4) {
        if ((*input >= 'A' && *input <= 'Z') ||
            (*input >= 'a' && *input <= 'z') ||
            (*input >= '0' && *input <= '9') ||
            *input == '-' || *input == '_' || *input == '.' || *input == '~') {
            output[pos++] = *input;
        } else if (*input == ' ') {
            output[pos++] = '+';
        } else {
            output[pos++] = '%';
            output[pos++] = hex[(*input >> 4) & 0x0F];
            output[pos++] = hex[*input & 0x0F];
        }
        input++;
    }
    output[pos] = '\0';
}

// Parse int64 from string
static int64_t parse_int64(const char *str)
{
    int64_t result = 0;
    int sign = 1;

    // Skip whitespace
    while (*str == ' ' || *str == '\t') str++;

    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Parse digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}

// Send message (non-blocking)
void telegram_send_message(const char *bot_token, int64_t chat_id, const char *text)
{
    printf("Sending Telegram message to chat %lld: %s\n", chat_id, text);

    // Save bot token for reconnection
    strncpy(g_bot_token, bot_token, sizeof(g_bot_token) - 1);

    // Set state
    g_telegram_data.state = TELEGRAM_STATE_SENDING;
    g_current_request_type = REQUEST_TYPE_SEND_MESSAGE;

    // URL encode the message text
    char encoded_text[TELEGRAM_MESSAGE_TEXT_MAX * 3 + 1];
    url_encode(text, encoded_text, sizeof(encoded_text));

    // Build POST body
    char post_body[512];
    snprintf(post_body, sizeof(post_body),
             "chat_id=%lld&text=%s",
             chat_id, encoded_text);

    int content_length = strlen(post_body);

    // Build HTTPS POST request
    snprintf(g_request_buffer, sizeof(g_request_buffer),
             "POST /bot%s/sendMessage HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             bot_token, TELEGRAM_API_HOST, content_length, post_body);

    printf("POST request ready\n");

    // Reset response buffer
    g_response_len = 0;
    memset(g_response_buffer, 0, sizeof(g_response_buffer));

    // Start DNS lookup for api.telegram.org
    err_t err = dns_gethostbyname(TELEGRAM_API_HOST, &g_server_ip, telegram_dns_found, NULL);

    if (err == ERR_OK) {
        // IP address was cached
        printf("Using cached IP for %s\n", TELEGRAM_API_HOST);
        telegram_dns_found(TELEGRAM_API_HOST, &g_server_ip, NULL);
    } else if (err != ERR_INPROGRESS) {
        printf("DNS lookup failed immediately: %d\n", err);
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "DNS lookup failed");
    }
}

// Poll for new messages (non-blocking)
void telegram_poll_updates(const char *bot_token)
{
    printf("Polling Telegram updates (offset: %lld)\n", g_telegram_data.last_update_id + 1);

    // Save bot token for reconnection
    strncpy(g_bot_token, bot_token, sizeof(g_bot_token) - 1);

    // Set state
    g_telegram_data.state = TELEGRAM_STATE_RECEIVING;
    g_current_request_type = REQUEST_TYPE_GET_UPDATES;

    // Build HTTPS GET request with offset
    snprintf(g_request_buffer, sizeof(g_request_buffer),
             "GET /bot%s/getUpdates?offset=%lld&timeout=5 HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             bot_token, g_telegram_data.last_update_id + 1, TELEGRAM_API_HOST);

    printf("GET request ready\n");

    // Reset response buffer
    g_response_len = 0;
    memset(g_response_buffer, 0, sizeof(g_response_buffer));

    // Start DNS lookup for api.telegram.org
    err_t err = dns_gethostbyname(TELEGRAM_API_HOST, &g_server_ip, telegram_dns_found, NULL);

    if (err == ERR_OK) {
        // IP address was cached
        printf("Using cached IP for %s\n", TELEGRAM_API_HOST);
        telegram_dns_found(TELEGRAM_API_HOST, &g_server_ip, NULL);
    } else if (err != ERR_INPROGRESS) {
        printf("DNS lookup failed immediately: %d\n", err);
        g_telegram_data.state = TELEGRAM_STATE_ERROR;
        snprintf(g_telegram_data.error_message, sizeof(g_telegram_data.error_message),
                 "DNS lookup failed");
    }
}

// Get current telegram data
telegram_data_t* telegram_api_get_data(void)
{
    return &g_telegram_data;
}

// Get current API state
telegram_api_state_t telegram_api_get_state(void)
{
    return g_telegram_data.state;
}
