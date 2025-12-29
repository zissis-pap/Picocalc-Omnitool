/**
 * @file psram_helper.c
 * @brief PSRAM allocation implementation for RP2350
 */

#include "psram_helper.h"
#include <stdio.h>
#include <string.h>

// Global allocator instance
psram_allocator_t g_psram_alloc = {0};

bool psram_init(void)
{
    if (g_psram_alloc.initialized) {
        return true;  // Already initialized
    }

    // PSRAM is automatically initialized by the SDK when PICO_RP2350_PSRAM=1
    // We just need to set up our simple allocator
    g_psram_alloc.base = (uint8_t*)PSRAM_BASE_ADDR;
    g_psram_alloc.current = g_psram_alloc.base;
    g_psram_alloc.remaining = PSRAM_SIZE;
    g_psram_alloc.initialized = true;

    printf("PSRAM initialized: %d MB at 0x%08X\n",
           PSRAM_SIZE / (1024 * 1024),
           PSRAM_BASE_ADDR);

    // Test PSRAM with a simple read/write test
    volatile uint32_t *test_addr = (volatile uint32_t*)PSRAM_BASE_ADDR;
    uint32_t test_pattern = 0xDEADBEEF;

    *test_addr = test_pattern;
    uint32_t read_back = *test_addr;

    if (read_back == test_pattern) {
        printf("PSRAM test PASSED: wrote 0x%08X, read 0x%08X\n", test_pattern, read_back);
    } else {
        printf("PSRAM test FAILED: wrote 0x%08X, read 0x%08X\n", test_pattern, read_back);
        printf("WARNING: PSRAM may not be working correctly!\n");
    }

    return true;
}

void* psram_malloc(size_t size)
{
    if (!g_psram_alloc.initialized) {
        printf("ERROR: PSRAM not initialized!\n");
        return NULL;
    }

    // Align to 4-byte boundary
    size = (size + 3) & ~3;

    if (size > g_psram_alloc.remaining) {
        printf("ERROR: PSRAM allocation failed - requested %zu bytes, only %zu available\n",
               size, g_psram_alloc.remaining);
        return NULL;
    }

    void* ptr = g_psram_alloc.current;
    g_psram_alloc.current += size;
    g_psram_alloc.remaining -= size;

    printf("PSRAM allocated %zu bytes at 0x%08X (remaining: %zu KB)\n",
           size, (uintptr_t)ptr, g_psram_alloc.remaining / 1024);

    // Clear the allocated memory
    memset(ptr, 0, size);

    return ptr;
}

size_t psram_get_free(void)
{
    return g_psram_alloc.initialized ? g_psram_alloc.remaining : 0;
}

void psram_get_stats(size_t* used, size_t* total)
{
    if (total) {
        *total = PSRAM_SIZE;
    }
    if (used) {
        *used = g_psram_alloc.initialized ?
                (PSRAM_SIZE - g_psram_alloc.remaining) : 0;
    }
}
