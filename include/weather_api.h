#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Maximum forecast entries (16 = 48 hours at 3-hour intervals)
#define MAX_WEATHER_FORECASTS 16
#define MAX_WEATHER_CITIES 10

// String length constants
#define WEATHER_CITY_NAME_MAX 64
#define WEATHER_DESCRIPTION_MAX 64
#define WEATHER_ICON_CODE_MAX 8

// Map image buffer size (PNG images are typically 10-50KB)
#define WEATHER_MAP_IMAGE_MAX_SIZE (64 * 1024)  // 64KB buffer

// Predefined city list with coordinates (for quick selection)
typedef struct {
    char name[WEATHER_CITY_NAME_MAX];
    float lat;
    float lon;
} weather_city_t;

// Individual forecast entry (3-hour interval)
typedef struct {
    time_t timestamp;           // Unix timestamp
    float temp;                 // Temperature in Celsius
    float feels_like;          // Feels like temperature
    int humidity;              // Humidity percentage
    char description[WEATHER_DESCRIPTION_MAX];  // "clear sky", "light rain"
    char icon[WEATHER_ICON_CODE_MAX];          // "01d", "10n" etc.
} weather_forecast_t;

// Weather API state machine
typedef enum {
    WEATHER_STATE_IDLE,
    WEATHER_STATE_FETCHING_FORECAST,
    WEATHER_STATE_FETCHING_MAP,
    WEATHER_STATE_SUCCESS,
    WEATHER_STATE_ERROR
} weather_api_state_t;

// Request type (for handling responses appropriately)
typedef enum {
    WEATHER_REQUEST_FORECAST,
    WEATHER_REQUEST_MAP
} weather_request_type_t;

// Main weather data structure
typedef struct {
    // Current request state
    weather_api_state_t state;
    weather_request_type_t current_request;
    char error_message[128];

    // City information
    char city_name[WEATHER_CITY_NAME_MAX];
    float latitude;
    float longitude;

    // Forecast data
    weather_forecast_t forecasts[MAX_WEATHER_FORECASTS];
    uint8_t forecast_count;

    // Map image data
    uint8_t *map_image_data;
    uint32_t map_image_size;
    bool map_loaded;
} weather_data_t;

// API Functions
void weather_api_init(void);
void weather_api_fetch_forecast(const char *api_key, const char *city);
void weather_api_fetch_map(const char *api_key, float lat, float lon);
weather_data_t* weather_api_get_data(void);
weather_api_state_t weather_api_get_state(void);
void weather_api_cleanup(void);  // Free map image memory

// Utility: Get weather emoji from icon code
const char* weather_get_emoji(const char *icon_code);

// Get predefined cities array
const weather_city_t* weather_get_cities(void);

#endif // WEATHER_API_H
