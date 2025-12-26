#ifndef NEWS_API_H
#define NEWS_API_H

#include <stdint.h>
#include <stdbool.h>

// Maximum number of news articles to fetch
#define MAX_NEWS_ARTICLES 10

// Maximum lengths for news data
#define NEWS_TITLE_MAX_LEN 128
#define NEWS_SOURCE_MAX_LEN 64
#define NEWS_DESCRIPTION_MAX_LEN 256

// News article structure
typedef struct {
    char title[NEWS_TITLE_MAX_LEN + 1];
    char source[NEWS_SOURCE_MAX_LEN + 1];
    char description[NEWS_DESCRIPTION_MAX_LEN + 1];
} news_article_t;

// News fetch state
typedef enum {
    NEWS_STATE_IDLE,
    NEWS_STATE_FETCHING,
    NEWS_STATE_SUCCESS,
    NEWS_STATE_ERROR
} news_fetch_state_t;

// News data structure
typedef struct {
    news_article_t articles[MAX_NEWS_ARTICLES];
    uint8_t count;
    news_fetch_state_t state;
    char error_message[128];
} news_data_t;

// Initialize news API
void news_api_init(void);

// Fetch top headlines (non-blocking)
// Requires NewsAPI key - get one free at https://newsapi.org
void news_api_fetch_headlines(const char *api_key, const char *country);

// Get current news data
news_data_t* news_api_get_data(void);

// Get current fetch state
news_fetch_state_t news_api_get_state(void);

#endif // NEWS_API_H
