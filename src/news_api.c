#include "news_api.h"
#include "pico/stdlib.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>

// NewsAPI hostname
#define NEWS_API_HOST "newsapi.org"
#define NEWS_API_IP_ADDR "104.18.38.148"  // Fallback IP if DNS fails

// Global news data
static news_data_t g_news_data = {0};
static struct tcp_pcb *g_tcp_pcb = NULL;
static ip_addr_t g_server_ip;
static char g_request_buffer[512];
static char g_response_buffer[8192];
static uint16_t g_response_len = 0;

// Forward declarations
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_client_err(void *arg, err_t err);
static void news_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg);
static void parse_news_response(const char *response, uint16_t len);

// Initialize news API
void news_api_init(void)
{
    memset(&g_news_data, 0, sizeof(news_data_t));
    g_news_data.state = NEWS_STATE_IDLE;
}

// DNS callback
static void news_dns_found(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr != NULL) {
        printf("DNS resolved: %s\n", ipaddr_ntoa(ipaddr));
        g_server_ip = *ipaddr;

        // Create TCP connection
        g_tcp_pcb = tcp_new();
        if (g_tcp_pcb != NULL) {
            tcp_arg(g_tcp_pcb, NULL);
            tcp_err(g_tcp_pcb, tcp_client_err);
            err_t err = tcp_connect(g_tcp_pcb, &g_server_ip, 80, tcp_client_connected);
            if (err != ERR_OK) {
                printf("TCP connect failed: %d\n", err);
                g_news_data.state = NEWS_STATE_ERROR;
                snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                         "Connection failed");
                tcp_close(g_tcp_pcb);
                g_tcp_pcb = NULL;
            }
        } else {
            printf("Failed to create TCP PCB\n");
            g_news_data.state = NEWS_STATE_ERROR;
            snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                     "Failed to create connection");
        }
    } else {
        printf("DNS lookup failed for %s\n", name);
        g_news_data.state = NEWS_STATE_ERROR;
        snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                 "DNS lookup failed");
    }
}

// TCP connected callback
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK) {
        printf("Connection failed: %d\n", err);
        g_news_data.state = NEWS_STATE_ERROR;
        snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                 "Connection error");
        return err;
    }

    printf("Connected to NewsAPI server\n");

    // Set up receive callback
    tcp_recv(tpcb, tcp_client_recv);

    // Send HTTP GET request
    err_t write_err = tcp_write(tpcb, g_request_buffer, strlen(g_request_buffer), TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        printf("TCP write failed: %d\n", write_err);
        g_news_data.state = NEWS_STATE_ERROR;
        snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                 "Failed to send request");
        return write_err;
    }

    tcp_output(tpcb);
    printf("HTTP request sent\n");

    return ERR_OK;
}

// TCP receive callback
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL) {
        // Connection closed by server
        printf("Connection closed by server\n");
        tcp_close(tpcb);
        g_tcp_pcb = NULL;

        // Parse the response
        if (g_response_len > 0) {
            parse_news_response(g_response_buffer, g_response_len);
        }

        return ERR_OK;
    }

    if (err != ERR_OK) {
        printf("Receive error: %d\n", err);
        pbuf_free(p);
        g_news_data.state = NEWS_STATE_ERROR;
        snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                 "Receive error");
        return err;
    }

    // Copy data to response buffer
    uint16_t copy_len = p->tot_len;
    if (g_response_len + copy_len > sizeof(g_response_buffer) - 1) {
        copy_len = sizeof(g_response_buffer) - 1 - g_response_len;
    }

    pbuf_copy_partial(p, g_response_buffer + g_response_len, copy_len, 0);
    g_response_len += copy_len;
    g_response_buffer[g_response_len] = '\0';

    // Tell lwIP we've processed the data
    tcp_recved(tpcb, p->tot_len);

    pbuf_free(p);

    return ERR_OK;
}

// TCP error callback
static void tcp_client_err(void *arg, err_t err)
{
    printf("TCP error: %d\n", err);
    g_news_data.state = NEWS_STATE_ERROR;
    snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
             "Network error");
    g_tcp_pcb = NULL;
}

// Simple JSON parser for news articles
static void parse_news_response(const char *response, uint16_t len)
{
    printf("Parsing response (%d bytes)\n", len);

    // Find the JSON body (skip HTTP headers)
    const char *json_start = strstr(response, "\r\n\r\n");
    if (json_start == NULL) {
        printf("No JSON body found\n");
        g_news_data.state = NEWS_STATE_ERROR;
        snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                 "Invalid response");
        return;
    }
    json_start += 4; // Skip past "\r\n\r\n"

    // Check for error in response
    if (strstr(json_start, "\"status\":\"error\"") != NULL) {
        printf("API returned error status\n");
        g_news_data.state = NEWS_STATE_ERROR;

        // Try to extract error message
        const char *msg_start = strstr(json_start, "\"message\":\"");
        if (msg_start != NULL) {
            msg_start += 11;
            const char *msg_end = strchr(msg_start, '"');
            if (msg_end != NULL) {
                int msg_len = msg_end - msg_start;
                if (msg_len > 127) msg_len = 127;
                strncpy(g_news_data.error_message, msg_start, msg_len);
                g_news_data.error_message[msg_len] = '\0';
            }
        } else {
            snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                     "API error");
        }
        return;
    }

    // Parse articles (simple parser - looks for title fields)
    g_news_data.count = 0;
    const char *search_pos = json_start;

    while (g_news_data.count < MAX_NEWS_ARTICLES) {
        // Find next "title" field
        search_pos = strstr(search_pos, "\"title\":\"");
        if (search_pos == NULL) break;

        search_pos += 9; // Skip past "title":"

        // Find the end of the title string, handling escaped quotes
        const char *title_end = search_pos;
        while (*title_end != '\0') {
            if (*title_end == '\\' && *(title_end + 1) != '\0') {
                // Skip escaped character
                title_end += 2;
            } else if (*title_end == '"') {
                // Found unescaped closing quote
                break;
            } else {
                title_end++;
            }
        }
        if (*title_end != '"') break;

        // Copy title
        int title_len = title_end - search_pos;
        if (title_len > NEWS_TITLE_MAX_LEN) title_len = NEWS_TITLE_MAX_LEN;
        strncpy(g_news_data.articles[g_news_data.count].title, search_pos, title_len);
        g_news_data.articles[g_news_data.count].title[title_len] = '\0';

        // Clean up escaped characters in title
        int write_pos = 0;
        for (int read_pos = 0; read_pos < title_len; read_pos++) {
            if (g_news_data.articles[g_news_data.count].title[read_pos] == '\\' &&
                read_pos + 1 < title_len) {
                char next = g_news_data.articles[g_news_data.count].title[read_pos + 1];
                if (next == '"' || next == '\\' || next == '/') {
                    // Skip the backslash, keep the escaped character
                    g_news_data.articles[g_news_data.count].title[write_pos++] = next;
                    read_pos++; // Skip the escaped char
                } else {
                    g_news_data.articles[g_news_data.count].title[write_pos++] =
                        g_news_data.articles[g_news_data.count].title[read_pos];
                }
            } else {
                g_news_data.articles[g_news_data.count].title[write_pos++] =
                    g_news_data.articles[g_news_data.count].title[read_pos];
            }
        }
        g_news_data.articles[g_news_data.count].title[write_pos] = '\0';

        // Try to find source name (look backwards from title for source object)
        const char *source_search = search_pos - 200;
        if (source_search < json_start) source_search = json_start;
        const char *source_start = strstr(source_search, "\"name\":\"");
        if (source_start != NULL && source_start < search_pos) {
            source_start += 8;

            // Find the end of the source string, handling escaped quotes
            const char *source_end = source_start;
            while (*source_end != '\0') {
                if (*source_end == '\\' && *(source_end + 1) != '\0') {
                    source_end += 2;
                } else if (*source_end == '"') {
                    break;
                } else {
                    source_end++;
                }
            }

            if (*source_end == '"') {
                int source_len = source_end - source_start;
                if (source_len > NEWS_SOURCE_MAX_LEN) source_len = NEWS_SOURCE_MAX_LEN;
                strncpy(g_news_data.articles[g_news_data.count].source, source_start, source_len);
                g_news_data.articles[g_news_data.count].source[source_len] = '\0';

                // Clean up escaped characters in source
                int write_pos = 0;
                for (int read_pos = 0; read_pos < source_len; read_pos++) {
                    if (g_news_data.articles[g_news_data.count].source[read_pos] == '\\' &&
                        read_pos + 1 < source_len) {
                        char next = g_news_data.articles[g_news_data.count].source[read_pos + 1];
                        if (next == '"' || next == '\\' || next == '/') {
                            // Skip the backslash, keep the escaped character
                            g_news_data.articles[g_news_data.count].source[write_pos++] = next;
                            read_pos++; // Skip the escaped char
                        } else {
                            g_news_data.articles[g_news_data.count].source[write_pos++] =
                                g_news_data.articles[g_news_data.count].source[read_pos];
                        }
                    } else {
                        g_news_data.articles[g_news_data.count].source[write_pos++] =
                            g_news_data.articles[g_news_data.count].source[read_pos];
                    }
                }
                g_news_data.articles[g_news_data.count].source[write_pos] = '\0';
            }
        }

        // Try to find description (look forward from title)
        const char *desc_start = strstr(title_end, "\"description\":\"");
        if (desc_start != NULL && desc_start < title_end + 1000) {
            desc_start += 15; // Skip past "description":"

            // Find the end of the description string, handling escaped quotes
            const char *desc_end = desc_start;
            while (*desc_end != '\0') {
                if (*desc_end == '\\' && *(desc_end + 1) != '\0') {
                    // Skip escaped character (e.g., \", \\, \n)
                    desc_end += 2;
                } else if (*desc_end == '"') {
                    // Found unescaped closing quote
                    break;
                } else {
                    desc_end++;
                }
            }

            if (*desc_end == '"') {
                int desc_len = desc_end - desc_start;
                if (desc_len > NEWS_DESCRIPTION_MAX_LEN) desc_len = NEWS_DESCRIPTION_MAX_LEN;
                strncpy(g_news_data.articles[g_news_data.count].description, desc_start, desc_len);
                g_news_data.articles[g_news_data.count].description[desc_len] = '\0';

                // Replace escaped sequences for better display
                int write_pos = 0;
                for (int read_pos = 0; read_pos < desc_len; read_pos++) {
                    if (g_news_data.articles[g_news_data.count].description[read_pos] == '\\' &&
                        read_pos + 1 < desc_len) {
                        char next = g_news_data.articles[g_news_data.count].description[read_pos + 1];
                        if (next == 'n') {
                            // Replace \n with space
                            g_news_data.articles[g_news_data.count].description[write_pos++] = ' ';
                            read_pos++; // Skip the 'n'
                        } else if (next == '"' || next == '\\' || next == '/') {
                            // Skip the backslash, keep the escaped character
                            g_news_data.articles[g_news_data.count].description[write_pos++] = next;
                            read_pos++; // Skip the escaped char
                        } else {
                            // Keep the backslash for unknown escapes
                            g_news_data.articles[g_news_data.count].description[write_pos++] =
                                g_news_data.articles[g_news_data.count].description[read_pos];
                        }
                    } else {
                        g_news_data.articles[g_news_data.count].description[write_pos++] =
                            g_news_data.articles[g_news_data.count].description[read_pos];
                    }
                }
                g_news_data.articles[g_news_data.count].description[write_pos] = '\0';
            }
        }

        g_news_data.count++;
        search_pos = title_end + 1;
    }

    printf("Parsed %d articles\n", g_news_data.count);

    if (g_news_data.count > 0) {
        g_news_data.state = NEWS_STATE_SUCCESS;
    } else {
        g_news_data.state = NEWS_STATE_ERROR;
        snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                 "No articles found");
    }
}

// Fetch top headlines
void news_api_fetch_headlines(const char *api_key, const char *country)
{
    if (g_news_data.state == NEWS_STATE_FETCHING) {
        printf("Already fetching news\n");
        return;
    }

    printf("Fetching news headlines for country: %s\n", country);

    // Reset state
    g_news_data.state = NEWS_STATE_FETCHING;
    g_news_data.count = 0;
    g_response_len = 0;
    memset(g_response_buffer, 0, sizeof(g_response_buffer));

    // Build HTTP request
    snprintf(g_request_buffer, sizeof(g_request_buffer),
             "GET /v2/top-headlines?country=%s&pageSize=20&apiKey=%s HTTP/1.1\r\n"
             "Host: newsapi.org\r\n"
             "Connection: close\r\n"
             "User-Agent: PicoCalc-Omnitool\r\n"
             "\r\n",
             country, api_key);

    printf("Request: %s\n", g_request_buffer);

    // Start DNS lookup
    err_t err = dns_gethostbyname(NEWS_API_HOST, &g_server_ip, news_dns_found, NULL);

    if (err == ERR_OK) {
        // DNS already cached, connect immediately
        printf("DNS cached, connecting immediately\n");
        news_dns_found(NEWS_API_HOST, &g_server_ip, NULL);
    } else if (err != ERR_INPROGRESS) {
        printf("DNS lookup failed: %d\n", err);
        g_news_data.state = NEWS_STATE_ERROR;
        snprintf(g_news_data.error_message, sizeof(g_news_data.error_message),
                 "DNS lookup failed");
    }
}

// Get current news data
news_data_t* news_api_get_data(void)
{
    return &g_news_data;
}

// Get current fetch state
news_fetch_state_t news_api_get_state(void)
{
    return g_news_data.state;
}
