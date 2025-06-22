#include "framebuffer.h"
#include "vga.h"
#include "task.h"

framebuffer_info fb_info = {0};

// Multiboot2 tag structure
typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot_tag;

void fb_init(void *multiboot_info_ptr) {
    enter_critical_section();

    // Initialize framebuffer info to zero
    fb_info.addr = 0;
    fb_info.width = 0;
    fb_info.height = 0;
    fb_info.pitch = 0;
    fb_info.bpp = 0;
    fb_info.is_rgb = 1;

    // Check if multiboot_info is valid
    if (multiboot_info_ptr == 0) {
        exit_critical_section();
        return;
    }

    uint32_t *multiboot_info = (uint32_t *)multiboot_info_ptr;
    
    // multiboot_info points to the multiboot2 information structure
    uint32_t total_size = *multiboot_info;
    
    // Validate total_size
    if (total_size < 16 || total_size > 0x10000) {
        exit_critical_section();
        return;
    }

    // Start parsing tags after the 8-byte header
    uint8_t *tag_ptr = (uint8_t *)multiboot_info + 8;
    uint8_t *end_ptr = (uint8_t *)multiboot_info + total_size;
    int tag_count = 0;
    int found_framebuffer = 0;

    while (tag_ptr < end_ptr && tag_count < 100) {
        multiboot_tag *tag = (multiboot_tag *)tag_ptr;
        
        // Validate tag pointer
        if (tag_ptr + sizeof(multiboot_tag) > end_ptr) {
            break;
        }
        
        // Validate tag size
        if (tag->size < 8) {
            break;
        }
        
        if (tag->type == 8) {  // Framebuffer tag
            found_framebuffer = 1;
            multiboot_framebuffer_tag *fb_tag = (multiboot_framebuffer_tag *)tag;
            
            // Only accept valid framebuffer configurations
            if (fb_tag->bpp == 32 && fb_tag->width >= 640 && fb_tag->height >= 480) {
                // Validate framebuffer address is reasonable
                if (fb_tag->addr >= 0x100000 && fb_tag->addr < 0x100000000ULL) {
                    fb_info.addr = fb_tag->addr;
                    fb_info.width = fb_tag->width;
                    fb_info.height = fb_tag->height;
                    fb_info.pitch = fb_tag->pitch;
                    fb_info.bpp = fb_tag->bpp;
                    fb_info.is_rgb = 1;
                    
                    // Print framebuffer info to VGA text mode for debugging
                    print_string("Framebuffer found: ");
                    print_hex(fb_tag->addr);
                    print_string(" ");
                    print_dec(fb_tag->width);
                    print_string("x");
                    print_dec(fb_tag->height);
                    print_string(" @ ");
                    print_dec(fb_tag->bpp);
                    print_string("bpp pitch=");
                    print_dec(fb_tag->pitch);
                    print_string("\n");
                } else {
                    print_string("Invalid framebuffer address: ");
                    print_hex(fb_tag->addr);
                    print_string("\n");
                }
            }
            break;
        } else if (tag->type == 0) {  // End tag
            break;
        }

        // Move to next tag (align to 8-byte boundary)
        uint32_t tag_size = tag->size;
        tag_size = (tag_size + 7) & ~7;  // Align to 8 bytes
        tag_ptr += tag_size;
        tag_count++;
        
        if (tag_ptr >= end_ptr) {
            break;
        }
    }

    if (!found_framebuffer) {
        print_string("No framebuffer found - text mode only\n");
    }

    exit_critical_section();
}

void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_info.width || y >= fb_info.height || fb_info.addr == 0) {
        return;
    }

    enter_critical_section();
    
    // Calculate the memory address for this pixel
    uint8_t *fb = (uint8_t *)fb_info.addr;
    uint32_t bytes_per_pixel = fb_info.bpp / 8;
    uint32_t index = y * fb_info.pitch + x * bytes_per_pixel;
    
    // Bounds check for the framebuffer memory
    uint32_t max_index = fb_info.height * fb_info.pitch;
    if (index >= max_index || index + bytes_per_pixel > max_index) {
        exit_critical_section();
        return;
    }
    
    if (fb_info.bpp == 32) {
        // 32-bit RGBA/RGBX format
        uint32_t *pixel = (uint32_t *)(fb + index);
        *pixel = color;  // Don't force alpha - let the graphics system handle it
    } else if (fb_info.bpp == 24) {
        // 24-bit RGB format
        fb[index + 0] = color & 0xFF;          // Blue
        fb[index + 1] = (color >> 8) & 0xFF;   // Green  
        fb[index + 2] = (color >> 16) & 0xFF;  // Red
    }
    
    exit_critical_section();
}

void fb_clear(uint32_t color) {
    if (fb_info.addr == 0) return;

    enter_critical_section();
    
    // More efficient clearing for 32-bit framebuffers
    if (fb_info.bpp == 32) {
        uint32_t *fb = (uint32_t *)fb_info.addr;
        uint32_t total_pixels = fb_info.height * fb_info.width;  // Fixed calculation
        
        for (uint32_t i = 0; i < total_pixels; i++) {
            fb[i] = color;
        }
    } else {
        // Fallback to pixel-by-pixel for other bit depths
        for (uint32_t y = 0; y < fb_info.height; y++) {
            for (uint32_t x = 0; x < fb_info.width; x++) {
                fb_draw_pixel(x, y, color);
            }
        }
    }
    
    exit_critical_section();
}

void fb_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (fb_info.addr == 0) return;
    
    // Clip rectangle to screen bounds
    if (x >= fb_info.width || y >= fb_info.height) return;
    
    uint32_t max_width = (x + width > fb_info.width) ? fb_info.width - x : width;
    uint32_t max_height = (y + height > fb_info.height) ? fb_info.height - y : height;
    
    for (uint32_t dy = 0; dy < max_height; dy++) {
        for (uint32_t dx = 0; dx < max_width; dx++) {
            fb_draw_pixel(x + dx, y + dy, color);
        }
    }
}