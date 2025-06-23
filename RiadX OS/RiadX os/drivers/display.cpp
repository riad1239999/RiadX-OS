#include "display.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>

// PixelBuffer implementation
PixelBuffer::PixelBuffer(int w, int h) : width(w), height(h) {
    buffer = new uint32_t[width * height];
    clear();
}

PixelBuffer::~PixelBuffer() {
    delete[] buffer;
}

void PixelBuffer::set_pixel(int x, int y, const Color& color) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    if (x >= 0 && x < width && y >= 0 && y < height) {
        buffer[y * width + x] = color.to_uint32();
    }
}

Color PixelBuffer::get_pixel(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        uint32_t pixel = buffer[y * width + x];
        return Color((pixel >> 16) & 0xFF, (pixel >> 8) & 0xFF, pixel & 0xFF, (pixel >> 24) & 0xFF);
    }
    return Color(0, 0, 0, 0);
}

void PixelBuffer::clear(const Color& color) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    uint32_t color_value = color.to_uint32();
    for (int i = 0; i < width * height; i++) {
        buffer[i] = color_value;
    }
}

void PixelBuffer::fill_rect(const Rect& rect, const Color& color) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    int x1 = std::max(0, rect.x);
    int y1 = std::max(0, rect.y);
    int x2 = std::min(width, rect.x + rect.width);
    int y2 = std::min(height, rect.y + rect.height);
    
    uint32_t color_value = color.to_uint32();
    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            buffer[y * width + x] = color_value;
        }
    }
}

void PixelBuffer::draw_line(int x1, int y1, int x2, int y2, const Color& color) {
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1, y = y1;
    
    while (true) {
        set_pixel(x, y, color);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void PixelBuffer::draw_circle(int cx, int cy, int radius, const Color& color) {
    // Midpoint circle algorithm
    int x = 0;
    int y = radius;
    int d = 1 - radius;
    
    while (x <= y) {
        set_pixel(cx + x, cy + y, color);
        set_pixel(cx - x, cy + y, color);
        set_pixel(cx + x, cy - y, color);
        set_pixel(cx - x, cy - y, color);
        set_pixel(cx + y, cy + x, color);
        set_pixel(cx - y, cy + x, color);
        set_pixel(cx + y, cy - x, color);
        set_pixel(cx - y, cy - x, color);
        
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}



void DisplayDriver::draw_rect(const Rect& rect, const Color& color, bool filled) {
    if (filled) {
        pixel_buffer->fill_rect(rect, color);
    } else {
        // Draw rectangle outline
        for (int i = rect.x; i < rect.x + rect.width && i < screen_width; i++) {
            pixel_buffer->set_pixel(i, rect.y, color);
            if (rect.y + rect.height - 1 < screen_height) {
                pixel_buffer->set_pixel(i, rect.y + rect.height - 1, color);
            }
        }
        for (int i = rect.y; i < rect.y + rect.height && i < screen_height; i++) {
            pixel_buffer->set_pixel(rect.x, i, color);
            if (rect.x + rect.width - 1 < screen_width) {
                pixel_buffer->set_pixel(rect.x + rect.width - 1, i, color);
            }
        }
    }
}

void PixelBuffer::draw_text(int x, int y, const std::string& text, const Color& color) {
    // Simple bitmap font rendering (8x8 characters)
    const int char_width = 8;
    const int char_height = 8;
    
    // Simple bitmap font for ASCII characters (simplified)
    static const uint8_t font_data[128][8] = {
        // ASCII 32 (space)
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        // ASCII 33 (!)
        {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00},
        // Add more characters as needed...
        // For now, we'll use a simple pattern for all characters
    };
    
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c < 32 || c > 126) c = '?'; // Use '?' for non-printable characters
        
        int char_x = x + i * char_width;
        
        // Draw character using simple pattern
        for (int cy = 0; cy < char_height; cy++) {
            for (int cx = 0; cx < char_width; cx++) {
                // Simple pattern based on character
                bool pixel_on = ((cx + cy + c) % 3 == 0);
                if (pixel_on) {
                    set_pixel(char_x + cx, y + cy, color);
                }
            }
        }
    }
}

void PixelBuffer::copy_to(PixelBuffer& dest, int src_x, int src_y, int dest_x, int dest_y, int w, int h) {
    std::lock_guard<std::mutex> lock1(buffer_mutex);
    std::lock_guard<std::mutex> lock2(dest.buffer_mutex);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (src_x + x >= 0 && src_x + x < width && src_y + y >= 0 && src_y + y < height &&
                dest_x + x >= 0 && dest_x + x < dest.width && dest_y + y >= 0 && dest_y + y < dest.height) {
                
                dest.buffer[(dest_y + y) * dest.width + (dest_x + x)] = 
                    buffer[(src_y + y) * width + (src_x + x)];
            }
        }
    }
}

// DisplayDriver implementation
DisplayDriver::DisplayDriver() 
    : framebuffer(nullptr), backbuffer(nullptr), double_buffering(true),
      screen_width(SCREEN_WIDTH), screen_height(SCREEN_HEIGHT), 
      bits_per_pixel(BITS_PER_PIXEL), refresh_rate(60),
      hardware_acceleration(false), initialized(false) {
    
    std::cout << "[DISPLAY] Display driver initializing..." << std::endl;
}

DisplayDriver::~DisplayDriver() {
    shutdown();
}

bool DisplayDriver::initialize() {
    std::lock_guard<std::mutex> lock(display_mutex);
    
    try {
        // Initialize framebuffer
        framebuffer = new PixelBuffer(screen_width, screen_height);
        
        if (double_buffering) {
            backbuffer = new PixelBuffer(screen_width, screen_height);
        }
        
        // Clear screen
        clear_screen(Color(0, 0, 0)); // Black
        
        initialized = true;
        std::cout << "[DISPLAY] Display driver initialized (" 
                  << screen_width << "x" << screen_height << "x" << bits_per_pixel << ")" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[DISPLAY] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void DisplayDriver::shutdown() {
    std::lock_guard<std::mutex> lock(display_mutex);
    
    delete framebuffer;
    delete backbuffer;
    framebuffer = nullptr;
    backbuffer = nullptr;
    
    initialized = false;
    std::cout << "[DISPLAY] Display driver shutdown complete" << std::endl;
}

bool DisplayDriver::set_display_mode(int width, int height, int bpp) {
    std::lock_guard<std::mutex> lock(display_mutex);
    
    if (width <= 0 || height <= 0 || (bpp != 16 && bpp != 24 && bpp != 32)) {
        return false;
    }
    
    // Shutdown current mode
    delete framebuffer;
    delete backbuffer;
    
    // Set new mode
    screen_width = width;
    screen_height = height;
    bits_per_pixel = bpp;
    
    // Reinitialize buffers
    framebuffer = new PixelBuffer(screen_width, screen_height);
    if (double_buffering) {
        backbuffer = new PixelBuffer(screen_width, screen_height);
    }
    
    std::cout << "[DISPLAY] Display mode set to " << width << "x" << height << "x" << bpp << std::endl;
    return true;
}

void DisplayDriver::get_display_mode(int& width, int& height, int& bpp) {
    width = screen_width;
    height = screen_height;
    bpp = bits_per_pixel;
}

PixelBuffer* DisplayDriver::get_framebuffer() {
    return framebuffer;
}

PixelBuffer* DisplayDriver::get_backbuffer() {
    return double_buffering ? backbuffer : framebuffer;
}

void DisplayDriver::swap_buffers() {
    if (double_buffering && backbuffer) {
        std::swap(framebuffer, backbuffer);
    }
}

void DisplayDriver::present() {
    if (double_buffering && backbuffer && framebuffer) {
        // Copy backbuffer to framebuffer
        std::memcpy(framebuffer->get_buffer(), backbuffer->get_buffer(),
                   screen_width * screen_height * sizeof(uint32_t));
    }
    
    // In a real system, this would trigger hardware to update the display
    // For simulation purposes, we'll just mark the frame as presented
}

void DisplayDriver::clear_screen(const Color& color) {
    if (framebuffer) {
        framebuffer->clear(color);
    }
    if (backbuffer) {
        backbuffer->clear(color);
    }
}

void DisplayDriver::set_pixel(int x, int y, const Color& color) {
    PixelBuffer* buffer = get_backbuffer();
    if (buffer) {
        buffer->set_pixel(x, y, color);
    }
}

void DisplayDriver::draw_line(int x1, int y1, int x2, int y2, const Color& color) {
    PixelBuffer* buffer = get_backbuffer();
    if (buffer) {
        buffer->draw_line(x1, y1, x2, y2, color);
    }
}

void DisplayDriver::draw_rect(const Rect& rect, const Color& color, bool filled) {
    PixelBuffer* buffer = get_backbuffer();
    if (!buffer) return;
    
    if (filled) {
        buffer->fill_rect(rect, color);
    } else {
        // Draw outline
        buffer->draw_line(rect.x, rect.y, rect.x + rect.width - 1, rect.y, color);
        buffer->draw_line(rect.x + rect.width - 1, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
        buffer->draw_line(rect.x + rect.width - 1, rect.y + rect.height - 1, rect.x, rect.y + rect.height - 1, color);
        buffer->draw_line(rect.x, rect.y + rect.height - 1, rect.x, rect.y, color);
    }
}

void DisplayDriver::draw_circle(int cx, int cy, int radius, const Color& color, bool filled) {
    PixelBuffer* buffer = get_backbuffer();
    if (!buffer) return;
    
    if (filled) {
        // Fill circle
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) {
                    buffer->set_pixel(cx + x, cy + y, color);
                }
            }
        }
    } else {
        buffer->draw_circle(cx, cy, radius, color);
    }
}

void DisplayDriver::draw_text(int x, int y, const std::string& text, const Color& color) {
    PixelBuffer* buffer = get_backbuffer();
    if (buffer) {
        buffer->draw_text(x, y, text, color);
    }
}

void DisplayDriver::draw_bitmap(int x, int y, const uint32_t* bitmap, int width, int height) {
    PixelBuffer* buffer = get_backbuffer();
    if (!buffer || !bitmap) return;
    
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            uint32_t pixel = bitmap[py * width + px];
            Color color((pixel >> 16) & 0xFF, (pixel >> 8) & 0xFF, pixel & 0xFF, (pixel >> 24) & 0xFF);
            buffer->set_pixel(x + px, y + py, color);
        }
    }
}

void DisplayDriver::draw_sprite(int x, int y, const uint32_t* sprite, int width, int height, const Color& transparent_color) {
    PixelBuffer* buffer = get_backbuffer();
    if (!buffer || !sprite) return;
    
    uint32_t transparent_value = transparent_color.to_uint32();
    
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            uint32_t pixel = sprite[py * width + px];
            if (pixel != transparent_value) {
                Color color((pixel >> 16) & 0xFF, (pixel >> 8) & 0xFF, pixel & 0xFF, (pixel >> 24) & 0xFF);
                buffer->set_pixel(x + px, y + py, color);
            }
        }
    }
}

std::string DisplayDriver::get_hardware_info() {
    return "MyOS Display Driver v1.0 - Software Rendering";
}

bool DisplayDriver::supports_hardware_acceleration() {
    return hardware_acceleration;
}

void DisplayDriver::handle_interrupt() {
    // Handle display-related interrupts (VSync, etc.)
    present();
}

void DisplayDriver::enable_vsync() {
    std::cout << "[DISPLAY] VSync enabled" << std::endl;
}

void DisplayDriver::disable_vsync() {
    std::cout << "[DISPLAY] VSync disabled" << std::endl;
}

void DisplayDriver::save_screenshot(const std::string& filename) {
    std::cout << "[DISPLAY] Screenshot saved to " << filename << std::endl;
    // In a real implementation, this would save the framebuffer to a file
}

void DisplayDriver::print_display_info() {
    std::cout << "[DISPLAY] Display Information:" << std::endl;
    std::cout << "  Resolution: " << screen_width << "x" << screen_height << std::endl;
    std::cout << "  Bits per pixel: " << bits_per_pixel << std::endl;
    std::cout << "  Refresh rate: " << refresh_rate << "Hz" << std::endl;
    std::cout << "  Double buffering: " << (double_buffering ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Hardware acceleration: " << (hardware_acceleration ? "Enabled" : "Disabled") << std::endl;
}
