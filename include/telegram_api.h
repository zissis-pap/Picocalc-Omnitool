#ifndef TELEGRAM_API_H
#define TELEGRAM_API_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Maximum number of telegram messages to store
#define MAX_TELEGRAM_MESSAGES 15

// Maximum lengths for telegram data
#define TELEGRAM_MESSAGE_TEXT_MAX 256
#define TELEGRAM_USERNAME_MAX 32
#define TELEGRAM_CHAT_NAME_MAX 64

// Telegram message structure
typedef struct {
    int64_t message_id;
    int64_t chat_id;
    char username[TELEGRAM_USERNAME_MAX + 1];
    char text[TELEGRAM_MESSAGE_TEXT_MAX + 1];
    time_t timestamp;
} telegram_message_t;

// Telegram API state
typedef enum {
    TELEGRAM_STATE_IDLE,
    TELEGRAM_STATE_SENDING,
    TELEGRAM_STATE_RECEIVING,
    TELEGRAM_STATE_SUCCESS,
    TELEGRAM_STATE_ERROR
} telegram_api_state_t;

// Telegram data structure
typedef struct {
    telegram_message_t messages[MAX_TELEGRAM_MESSAGES];
    uint8_t message_count;
    telegram_api_state_t state;
    char error_message[128];

    // Polling state
    int64_t last_update_id;  // For getUpdates offset
    bool polling_active;
} telegram_data_t;

// Initialize telegram API
void telegram_api_init(void);

// Send message (non-blocking) via HTTPS
// Requires bot token and chat ID from api_tokens.h
// Connects directly to api.telegram.org using TLS
void telegram_send_message(const char *bot_token, int64_t chat_id, const char *text);

// Poll for new messages (non-blocking) via HTTPS
// Uses last_update_id to get only new messages
// Connects directly to api.telegram.org using TLS
void telegram_poll_updates(const char *bot_token);

// Get current telegram data
telegram_data_t* telegram_api_get_data(void);

// Get current API state
telegram_api_state_t telegram_api_get_state(void);

#endif // TELEGRAM_API_H
