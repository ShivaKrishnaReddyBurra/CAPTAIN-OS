#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

// Correct Multiboot2 framebuffer tag structure
typedef struct {
    uint32_t type;         // Tag type (8 for framebuffer)
    uint32_t size;         // Size of this tag
    uint64_t addr;         // Physical address of framebuffer
    uint32_t pitch;        // Bytes per scanline
    uint32_t width;        // Width in pixels
    uint32_t height;       // Height in pixels
    uint8_t bpp;           // Bits per pixel
    uint8_t fb_type;       // Framebuffer type (1 = indexed, 2 = RGB, 3 = text)
    uint16_t reserved;     // Reserved (must be 0)
    // Color info follows but we don't need it for basic RGB mode
} __attribute__((packed)) multiboot_framebuffer_tag;

typedef struct {
    uint64_t addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    uint8_t is_rgb;        // Changed from is_bgr to is_rgb for clarity
} framebuffer_info;

extern framebuffer_info fb_info;

void fb_init(void *multiboot_info);
void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

#endif