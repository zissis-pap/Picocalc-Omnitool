/**
 * @file psram_helper.h
 * @brief PSRAM allocation helpers for RP2350
 *
 * Provides easy access to the 8MB QSPI PSRAM on Raspberry Pi Pico 2W
 */

#ifndef PSRAM_HELPER_H
#define PSRAM_HELPER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// PSRAM memory region on RP2350
#define PSRAM_BASE_ADDR     0x11000000
#define PSRAM_SIZE          (8 * 1024 * 1024)  // 8MB

// Simple PSRAM allocator state
typedef struct {
    uint8_t* base;
    uint8_t* current;
    size_t remaining;
    bool initialized;
} psram_allocator_t;

// Global allocator instance
extern psram_allocator_t g_psram_alloc;

/**
 * @brief Initialize PSRAM allocator
 * @return true if successful, false otherwise
 */
bool psram_init(void);

/**
 * @brief Allocate memory from PSRAM
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if allocation fails
 */
void* psram_malloc(size_t size);

/**
 * @brief Get remaining PSRAM space
 * @return Number of bytes remaining
 */
size_t psram_get_free(void);

/**
 * @brief Get PSRAM usage statistics
 * @param used Output parameter for bytes used
 * @param total Output parameter for total bytes
 */
void psram_get_stats(size_t* used, size_t* total);

#endif // PSRAM_HELPER_H
