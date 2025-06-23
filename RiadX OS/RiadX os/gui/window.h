#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "../drivers/display.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"

// Window states
enum WindowState {
    WINDOW_NORMAL,
    WINDOW_MINIMIZED,
    WINDOW_MAXIMIZED,
    WINDOW_FULLSCREEN
};

// Window styles
enum WindowStyle {
    WINDOW_STYLE_NORMAL = 0,
    WINDOW_STYLE_BORDERLESS = 1,
    WINDOW_STYLE_DIALOG = 2,
    WINDOW_STYLE_POPUP = 4
};

// Window events
enum WindowEventType {
    WINDOW_CREATED,
    WINDOW_DESTROYED,
    WINDOW_MOVED,
    WINDOW_RESIZED,
    WINDOW_ACTIVATED,
    WINDOW_DEACTIVATED,
    WINDOW_MINIMIZED_EVENT,
    WINDOW_MAXIMIZED_EVENT,
    WINDOW_RESTORED,
    WINDOW_CLOSED
};

struct WindowEvent {
    WindowEventType type;
    int window_id;
    int x, y, width, height;
    uint64_t timestamp;
    
    WindowEvent() : type(WINDOW_CREATED), window_id(0), x(0), y(0), width(0), height(0), timestamp(0) {}
};

// Forward declaration
class Window;

// Window event callback
using WindowEventCallback = std::function<void(const WindowEvent&)>;
using KeyEventCallback = std::function<void(const KeyEvent&)>;
using MouseEventCallback = std::function<void(const MouseEvent&)>;
using PaintCallback = std::function<void(PixelBuffer*)>;

class Window {
private:
    static int next_window_id;
    
    int window_id;
    std::string title;
    Rect bounds;
    WindowState state;
    int style;
    bool visible;
    bool focused;
    bool resizable;
    bool closable;
    bool minimizable;
    bool maximizable;
    
    // Graphics
    std::unique_ptr<PixelBuffer> window_buffer;
    Color background_color;
    bool needs_redraw;
    
    // Event callbacks
    WindowEventCallback window_event_callback;
    KeyEventCallback key_event_callback;
    MouseEventCallback mouse_event_callback;
    PaintCallback paint_callback;
    
    // Child windows
    std::vector<std::shared_ptr<Window>> child_windows;
    Window* parent_window;
    
    // Window management
    void create_window_buffer();
    void destroy_window_buffer();

public:
    Window(const std::string& title, int x, int y, int width, int height, int style = WINDOW_STYLE_NORMAL);
    ~Window();
    
    // Basic properties
    int get_id() const { return window_id; }
    const std::string& get_title() const { return title; }
    void set_title(const std::string& new_title);
    
    // Position and size
    const Rect& get_bounds() const { return bounds; }
    void set_bounds(const Rect& new_bounds);
    void set_position(int x, int y);
    void set_size(int width, int height);
    void move(int delta_x, int delta_y);
    void resize(int new_width, int new_height);
    
    // State management
    WindowState get_state() const { return state; }
    void set_state(WindowState new_state);
    void minimize();
    void maximize();
    void restore();
    void close();
    
    // Visibility
    bool is_visible() const { return visible; }
    void show();
    void hide();
    
    // Focus
    bool is_focused() const { return focused; }
    void set_focus(bool focus);
    
    // Window properties
    bool is_resizable() const { return resizable; }
    void set_resizable(bool resizable);
    bool is_closable() const { return closable; }
    void set_closable(bool closable);
    bool is_minimizable() const { return minimizable; }
    void set_minimizable(bool minimizable);
    bool is_maximizable() const { return maximizable; }
    void set_maximizable(bool maximizable);
    
    // Graphics
    PixelBuffer* get_buffer() { return window_buffer.get(); }
    void set_background_color(const Color& color);
    Color get_background_color() const { return background_color; }
    void invalidate();
    void invalidate_rect(const Rect& rect);
    bool needs_repaint() const { return needs_redraw; }
    
    // Drawing operations
    void clear();
    void draw_pixel(int x, int y, const Color& color);
    void draw_line(int x1, int y1, int x2, int y2, const Color& color);
    void draw_rect(const Rect& rect, const Color& color, bool filled = false);
    void draw_circle(int cx, int cy, int radius, const Color& color, bool filled = false);
    void draw_text(int x, int y, const std::string& text, const Color& color);
    
    // Event handling
    void set_window_event_callback(WindowEventCallback callback);
    void set_key_event_callback(KeyEventCallback callback);
    void set_mouse_event_callback(MouseEventCallback callback);
    void set_paint_callback(PaintCallback callback);
    
    // Event processing
    void handle_window_event(const WindowEvent& event);
    void handle_key_event(const KeyEvent& event);
    void handle_mouse_event(const MouseEvent& event);
    void paint();
    
    // Hit testing
    bool contains_point(int x, int y) const;
    bool intersects_rect(const Rect& rect) const;
    
    // Child window management
    void add_child_window(std::shared_ptr<Window> child);
    void remove_child_window(std::shared_ptr<Window> child);
    std::vector<std::shared_ptr<Window>> get_child_windows() const;
    Window* get_parent_window() const { return parent_window; }
    void set_parent_window(Window* parent);
    
    // Z-order
    void bring_to_front();
    void send_to_back();
    
    // Utility functions
    void center_on_screen(int screen_width, int screen_height);
    void center_on_parent();
    Rect get_client_rect() const;
    Rect get_title_bar_rect() const;
    
    // Debug functions
    void print_window_info() const;
};

#endif
