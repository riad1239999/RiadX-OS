#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>

// Display constants
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define BITS_PER_PIXEL 32

// Color structure
struct Color {
    uint8_t r, g, b, a;
    
    Color(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
    
    uint32_t to_uint32() const {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }
};

// Rectangle structure
struct Rect {
    int x, y, width, height;
    
    Rect(int x = 0, int y = 0, int w = 0, int h = 0)
        : x(x), y(y), width(w), height(h) {}
};

// Pixel buffer
class PixelBuffer {
private:
    uint32_t* buffer;
    int width, height;
    std::mutex buffer_mutex;

public:
    PixelBuffer(int w, int h);
    ~PixelBuffer();
    
    void set_pixel(int x, int y, const Color& color);
    Color get_pixel(int x, int y) const;
    void clear(const Color& color = Color(0, 0, 0));
    void fill_rect(const Rect& rect, const Color& color);
    void draw_line(int x1, int y1, int x2, int y2, const Color& color);
    void draw_circle(int cx, int cy, int radius, const Color& color);
    void draw_text(int x, int y, const std::string& text, const Color& color);
    
    uint32_t* get_buffer() const { return buffer; }
    int get_width() const { return width; }
    int get_height() const { return height; }
    
    void copy_to(PixelBuffer& dest, int src_x, int src_y, int dest_x, int dest_y, int w, int h);
};

class DisplayDriver {
private:
    PixelBuffer* framebuffer;
    PixelBuffer* backbuffer;
    bool double_buffering;
    std::mutex display_mutex;
    
    // Display mode
    int screen_width;
    int screen_height;
    int bits_per_pixel;
    int refresh_rate;
    
    // Hardware simulation
    bool hardware_acceleration;
    bool initialized;

public:
    DisplayDriver();
    ~DisplayDriver();
    
    bool initialize();
    void shutdown();
    
    // Display mode management
    bool set_display_mode(int width, int height, int bpp);
    void get_display_mode(int& width, int& height, int& bpp);
    
    // Framebuffer operations
    PixelBuffer* get_framebuffer();
    PixelBuffer* get_backbuffer();
    void swap_buffers();
    void present();
    
    // Drawing operations
    void clear_screen(const Color& color = Color(0, 0, 0));
    void set_pixel(int x, int y, const Color& color);
    void draw_line(int x1, int y1, int x2, int y2, const Color& color);
    void draw_rect(const Rect& rect, const Color& color, bool filled = true);
    void draw_circle(int cx, int cy, int radius, const Color& color, bool filled = false);
    void draw_text(int x, int y, const std::string& text, const Color& color);
    
    // Bitmap operations
    void draw_bitmap(int x, int y, const uint32_t* bitmap, int width, int height);
    void draw_sprite(int x, int y, const uint32_t* sprite, int width, int height, const Color& transparent_color);
    
    // Hardware info
    std::string get_hardware_info();
    bool supports_hardware_acceleration();
    
    // Interrupt handling
    void handle_interrupt();
    void enable_vsync();
    void disable_vsync();
    
    // Debug functions
    void save_screenshot(const std::string& filename);
    void print_display_info();
};

#endif
