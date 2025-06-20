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
    fb_info.is_rgb = 1;  // Default to RGB

    // Check if multiboot_info is valid
    if (multiboot_info_ptr == 0) {
        print_string("Error: Multiboot info pointer is NULL!\n");
        print_string("This usually means the bootloader didn't pass proper info.\n");
        exit_critical_section();
        return;
    }

    uint32_t *multiboot_info = (uint32_t *)multiboot_info_ptr;
    
    // multiboot_info points to the multiboot2 information structure
    // The first 8 bytes are: total_size (4 bytes) + reserved (4 bytes)
    uint32_t total_size = *multiboot_info;
    uint32_t *reserved = multiboot_info + 1;
    
    print_string("Searching for framebuffer...\n");
    char buffer[32];
    print_string("Multiboot info size: ");
    itoa(total_size, buffer, 10);
    print_string(buffer);
    print_string("\n");

    // Validate total_size
    if (total_size < 16 || total_size > 0x10000) {
        print_string("Error: Invalid multiboot info size!\n");
        print_string("Expected: 16 - 65536, Got: ");
        itoa(total_size, buffer, 10);
        print_string(buffer);
        print_string("\n");
        exit_critical_section();
        return;
    }

    // Start parsing tags after the 8-byte header
    uint8_t *tag_ptr = (uint8_t *)multiboot_info + 8;
    uint8_t *end_ptr = (uint8_t *)multiboot_info + total_size;
    int tag_count = 0;

    while (tag_ptr < end_ptr && tag_count < 100) { // Safety limit
        multiboot_tag *tag = (multiboot_tag *)tag_ptr;
        
        // Validate tag pointer
        if (tag_ptr + sizeof(multiboot_tag) > end_ptr) {
            print_string("Error: Tag pointer out of bounds!\n");
            break;
        }
        
        // Validate tag size
        if (tag->size < 8) {
            print_string("Error: Invalid tag size: ");
            itoa(tag->size, buffer, 10);
            print_string(buffer);
            print_string("\n");
            break;
        }
        
        print_string("Tag ");
        itoa(tag_count, buffer, 10);
        print_string(buffer);
        print_string(": type=");
        itoa(tag->type, buffer, 10);
        print_string(buffer);
        print_string(" size=");
        itoa(tag->size, buffer, 10);
        print_string(buffer);
        print_string("\n");

        if (tag->type == 8) {  // Framebuffer tag type is 8
            print_string("Found framebuffer tag!\n");
            
            // The minimum size for a framebuffer tag should be at least 32 bytes
            if (tag->size >= 32) {
                multiboot_framebuffer_tag *fb_tag = (multiboot_framebuffer_tag *)tag;
                
                print_string("Raw framebuffer info:\n");
                print_string("  Address: 0x");
                itoa((uint32_t)fb_tag->addr, buffer, 16);
                print_string(buffer);
                print_string("\n  Width: ");
                itoa(fb_tag->width, buffer, 10);
                print_string(buffer);
                print_string("\n  Height: ");
                itoa(fb_tag->height, buffer, 10);
                print_string(buffer);
                print_string("\n  BPP: ");
                itoa(fb_tag->bpp, buffer, 10);
                print_string(buffer);
                print_string("\n  Type: ");
                itoa(fb_tag->fb_type, buffer, 10);
                print_string(buffer);
                print_string("\n");
                
                // Check if this is the VGA text buffer
                if (fb_tag->addr == 0xB8000) {
                    print_string("Error: This is VGA text mode buffer, not graphics framebuffer!\n");
                    print_string("GRUB failed to set graphics mode. This could be because:\n");
                    print_string("1. Your VM doesn't support VESA/VBE graphics\n");
                    print_string("2. GRUB configuration is incorrect\n");
                    print_string("3. The graphics drivers in GRUB are not loading\n");
                    print_string("\nTry these solutions:\n");
                    print_string("- Use 'make run-cirrus' or 'make run-vmware'\n");
                    print_string("- Check GRUB menu for 'Safe Graphics' option\n");
                } else {
                    // Validate other parameters
                    if (fb_tag->width == 0 || fb_tag->height == 0) {
                        print_string("Error: Invalid framebuffer dimensions!\n");
                    } else if (fb_tag->bpp != 24 && fb_tag->bpp != 32) {
                        print_string("Error: Unsupported color depth: ");
                        itoa(fb_tag->bpp, buffer, 10);
                        print_string(buffer);
                        print_string(" BPP. Need 24 or 32 BPP.\n");
                    } else if (fb_tag->fb_type != 2) {
                        print_string("Error: Expected RGB framebuffer (type 2), got type ");
                        itoa(fb_tag->fb_type, buffer, 10);
                        print_string(buffer);
                        print_string("\n");
                    } else {
                        // All checks passed - initialize framebuffer
                        fb_info.addr = fb_tag->addr;
                        fb_info.width = fb_tag->width;
                        fb_info.height = fb_tag->height;
                        fb_info.pitch = fb_tag->pitch;
                        fb_info.bpp = fb_tag->bpp;
                        fb_info.is_rgb = 1;
                        
                        print_string("SUCCESS: Graphics framebuffer initialized!\n");
                        print_string("Resolution: ");
                        itoa(fb_info.width, buffer, 10);
                        print_string(buffer);
                        print_string("x");
                        itoa(fb_info.height, buffer, 10);
                        print_string(buffer);
                        print_string(" @ ");
                        itoa(fb_info.bpp, buffer, 10);
                        print_string(buffer);
                        print_string(" BPP\n");
                        break;
                    }
                }
            } else {
                print_string("Error: Framebuffer tag too small! Size: ");
                itoa(tag->size, buffer, 10);
                print_string(buffer);
                print_string(" Expected >= 32\n");
            }
        } else if (tag->type == 0) {  // End tag
            print_string("Reached end tag.\n");
            break;
        }

        // Move to next tag (align to 8-byte boundary)
        uint32_t tag_size = tag->size;
        tag_size = (tag_size + 7) & ~7;  // Align to 8 bytes
        tag_ptr += tag_size;
        tag_count++;
        
        // Safety check to prevent infinite loop
        if (tag_ptr >= end_ptr) {
            print_string("Reached end of multiboot info.\n");
            break;
        }
    }

    if (fb_info.addr == 0) {
        print_string("No valid graphics framebuffer found.\n");
        print_string("Running in text mode only.\n");
        print_string("\nTo enable graphics mode:\n");
        print_string("1. Try different QEMU graphics: make run-cirrus\n");
        print_string("2. Use the 'Safe Graphics' GRUB menu option\n");
        print_string("3. Ensure your VM supports VBE/VESA\n");
    } else {
        print_string("Graphics framebuffer ready for use!\n");
    }

    exit_critical_section();
}

void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_info.width || y >= fb_info.height || fb_info.addr == 0) {
        return;
    }

    enter_critical_section();
    uint8_t *fb = (uint8_t *)fb_info.addr;
    uint32_t index = y * fb_info.pitch + x * (fb_info.bpp / 8);
    
    // Bounds check for the framebuffer memory
    uint32_t max_index = fb_info.height * fb_info.pitch;
    if (index >= max_index) {
        exit_critical_section();
        return;
    }
    
    if (fb_info.bpp == 32) {
        uint32_t *pixel = (uint32_t *)(fb + index);
        *pixel = color;
    } else if (fb_info.bpp == 24) {
        // Assume RGB order for now
        fb[index + 0] = (color >> 16) & 0xFF;  // Red
        fb[index + 1] = (color >> 8) & 0xFF;   // Green
        fb[index + 2] = color & 0xFF;          // Blue
    }
    exit_critical_section();
}

void fb_clear(uint32_t color) {
    if (fb_info.addr == 0) return;

    enter_critical_section();
    
    // More efficient clearing for 32-bit framebuffers
    if (fb_info.bpp == 32) {
        uint32_t *fb = (uint32_t *)fb_info.addr;
        uint32_t total_pixels = (fb_info.height * fb_info.pitch) / 4;
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
    
    for (uint32_t dy = 0; dy < height && (y + dy) < fb_info.height; dy++) {
        for (uint32_t dx = 0; dx < width && (x + dx) < fb_info.width; dx++) {
            fb_draw_pixel(x + dx, y + dy, color);
        }
    }
}