#include "window.h"
#include <iostream>
#include <algorithm>
#include <chrono>

int Window::next_window_id = 1;

Window::Window(const std::string& title, int x, int y, int width, int height, int style)
    : window_id(next_window_id++), title(title), bounds(x, y, width, height),
      state(WINDOW_NORMAL), style(style), visible(false), focused(false),
      resizable(true), closable(true), minimizable(true), maximizable(true),
      background_color(240, 240, 240), needs_redraw(true), parent_window(nullptr) {
    
    create_window_buffer();
    
    std::cout << "[WINDOW] Created window " << window_id << " \"" << title << "\" at (" 
              << x << ", " << y << ") size " << width << "x" << height << std::endl;
}

Window::~Window() {
    destroy_window_buffer();
    std::cout << "[WINDOW] Destroyed window " << window_id << " \"" << title << "\"" << std::endl;
}

void Window::create_window_buffer() {
    if (bounds.width > 0 && bounds.height > 0) {
        window_buffer = std::make_unique<PixelBuffer>(bounds.width, bounds.height);
        clear();
    }
}

void Window::destroy_window_buffer() {
    window_buffer.reset();
}

void Window::set_title(const std::string& new_title) {
    title = new_title;
    invalidate();
    
    WindowEvent event;
    event.type = WINDOW_MOVED; // Using moved as a generic change event
    event.window_id = window_id;
    event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (window_event_callback) {
        window_event_callback(event);
    }
}

void Window::set_bounds(const Rect& new_bounds) {
    Rect old_bounds = bounds;
    bounds = new_bounds;
    
    // Recreate buffer if size changed
    if (old_bounds.width != bounds.width || old_bounds.height != bounds.height) {
        create_window_buffer();
        invalidate();
        
        WindowEvent event;
        event.type = WINDOW_RESIZED;
        event.window_id = window_id;
        event.x = bounds.x;
        event.y = bounds.y;
        event.width = bounds.width;
        event.height = bounds.height;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (window_event_callback) {
            window_event_callback(event);
        }
    } else if (old_bounds.x != bounds.x || old_bounds.y != bounds.y) {
        WindowEvent event;
        event.type = WINDOW_MOVED;
        event.window_id = window_id;
        event.x = bounds.x;
        event.y = bounds.y;
        event.width = bounds.width;
        event.height = bounds.height;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (window_event_callback) {
            window_event_callback(event);
        }
    }
}

void Window::set_position(int x, int y) {
    set_bounds(Rect(x, y, bounds.width, bounds.height));
}

void Window::set_size(int width, int height) {
    set_bounds(Rect(bounds.x, bounds.y, width, height));
}

void Window::move(int delta_x, int delta_y) {
    set_position(bounds.x + delta_x, bounds.y + delta_y);
}

void Window::resize(int new_width, int new_height) {
    if (resizable) {
        set_size(new_width, new_height);
    }
}

void Window::set_state(WindowState new_state) {
    if (state == new_state) return;
    
    WindowState old_state = state;
    state = new_state;
    
    WindowEventType event_type;
    switch (new_state) {
        case WINDOW_MINIMIZED:
            event_type = WINDOW_MINIMIZED_EVENT;
            hide();
            break;
        case WINDOW_MAXIMIZED:
            event_type = WINDOW_MAXIMIZED_EVENT;
            break;
        case WINDOW_NORMAL:
            event_type = WINDOW_RESTORED;
            if (old_state == WINDOW_MINIMIZED) {
                show();
            }
            break;
        default:
            event_type = WINDOW_RESTORED;
            break;
    }
    
    WindowEvent event;
    event.type = event_type;
    event.window_id = window_id;
    event.x = bounds.x;
    event.y = bounds.y;
    event.width = bounds.width;
    event.height = bounds.height;
    event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (window_event_callback) {
        window_event_callback(event);
    }
}

void Window::minimize() {
    if (minimizable) {
        set_state(WINDOW_MINIMIZED);
    }
}

void Window::maximize() {
    if (maximizable) {
        set_state(WINDOW_MAXIMIZED);
    }
}

void Window::restore() {
    set_state(WINDOW_NORMAL);
}

void Window::close() {
    if (closable) {
        WindowEvent event;
        event.type = WINDOW_CLOSED;
        event.window_id = window_id;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (window_event_callback) {
            window_event_callback(event);
        }
    }
}

void Window::show() {
    if (!visible) {
        visible = true;
        invalidate();
        
        WindowEvent event;
        event.type = WINDOW_ACTIVATED;
        event.window_id = window_id;
        event.x = bounds.x;
        event.y = bounds.y;
        event.width = bounds.width;
        event.height = bounds.height;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (window_event_callback) {
            window_event_callback(event);
        }
    }
}

void Window::hide() {
    if (visible) {
        visible = false;
        
        WindowEvent event;
        event.type = WINDOW_DEACTIVATED;
        event.window_id = window_id;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (window_event_callback) {
            window_event_callback(event);
        }
    }
}

void Window::set_focus(bool focus) {
    if (focused != focus) {
        focused = focus;
        invalidate();
        
        WindowEvent event;
        event.type = focus ? WINDOW_ACTIVATED : WINDOW_DEACTIVATED;
        event.window_id = window_id;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (window_event_callback) {
            window_event_callback(event);
        }
    }
}

void Window::set_resizable(bool resizable) {
    this->resizable = resizable;
}

void Window::set_closable(bool closable) {
    this->closable = closable;
}

void Window::set_minimizable(bool minimizable) {
    this->minimizable = minimizable;
}

void Window::set_maximizable(bool maximizable) {
    this->maximizable = maximizable;
}

void Window::set_background_color(const Color& color) {
    background_color = color;
    invalidate();
}

void Window::invalidate() {
    needs_redraw = true;
}

void Window::invalidate_rect(const Rect& rect) {
    // For simplicity, invalidate the entire window
    invalidate();
}

void Window::clear() {
    if (window_buffer) {
        window_buffer->clear(background_color);
    }
}

void Window::draw_pixel(int x, int y, const Color& color) {
    if (window_buffer) {
        window_buffer->set_pixel(x, y, color);
    }
}

void Window::draw_line(int x1, int y1, int x2, int y2, const Color& color) {
    if (window_buffer) {
        window_buffer->draw_line(x1, y1, x2, y2, color);
    }
}

void Window::draw_rect(const Rect& rect, const Color& color, bool filled) {
    if (!window_buffer) return;
    
    if (filled) {
        window_buffer->fill_rect(rect, color);
    } else {
        window_buffer->draw_line(rect.x, rect.y, rect.x + rect.width - 1, rect.y, color);
        window_buffer->draw_line(rect.x + rect.width - 1, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
        window_buffer->draw_line(rect.x + rect.width - 1, rect.y + rect.height - 1, rect.x, rect.y + rect.height - 1, color);
        window_buffer->draw_line(rect.x, rect.y + rect.height - 1, rect.x, rect.y, color);
    }
}

void Window::draw_circle(int cx, int cy, int radius, const Color& color, bool filled) {
    if (!window_buffer) return;
    
    if (filled) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) {
                    window_buffer->set_pixel(cx + x, cy + y, color);
                }
            }
        }
    } else {
        window_buffer->draw_circle(cx, cy, radius, color);
    }
}

void Window::draw_text(int x, int y, const std::string& text, const Color& color) {
    if (window_buffer) {
        window_buffer->draw_text(x, y, text, color);
    }
}

void Window::set_window_event_callback(WindowEventCallback callback) {
    window_event_callback = callback;
}

void Window::set_key_event_callback(KeyEventCallback callback) {
    key_event_callback = callback;
}

void Window::set_mouse_event_callback(MouseEventCallback callback) {
    mouse_event_callback = callback;
}

void Window::set_paint_callback(PaintCallback callback) {
    paint_callback = callback;
}

void Window::handle_window_event(const WindowEvent& event) {
    if (window_event_callback) {
        window_event_callback(event);
    }
}

void Window::handle_key_event(const KeyEvent& event) {
    if (key_event_callback) {
        key_event_callback(event);
    }
}

void Window::handle_mouse_event(const MouseEvent& event) {
    if (mouse_event_callback) {
        mouse_event_callback(event);
    }
}

void Window::paint() {
    if (needs_redraw && window_buffer) {
        clear();
        
        // Draw window frame if not borderless
        if (!(style & WINDOW_STYLE_BORDERLESS)) {
            // Draw title bar
            Color title_color = focused ? Color(0, 120, 215) : Color(128, 128, 128);
            draw_rect(Rect(0, 0, bounds.width, 30), title_color, true);
            
            // Draw title text
            draw_text(10, 8, title, Color(255, 255, 255));
            
            // Draw window border
            Color border_color = focused ? Color(0, 0, 0) : Color(128, 128, 128);
            draw_rect(Rect(0, 0, bounds.width, bounds.height), border_color, false);
            
            // Draw close button
            if (closable) {
                Color close_color = Color(255, 0, 0);
                draw_rect(Rect(bounds.width - 25, 5, 20, 20), close_color, true);
                draw_text(bounds.width - 20, 10, "X", Color(255, 255, 255));
            }
        }
        
        // Call custom paint callback
        if (paint_callback) {
            paint_callback(window_buffer.get());
        }
        
        needs_redraw = false;
    }
}

bool Window::contains_point(int x, int y) const {
    return x >= bounds.x && x < bounds.x + bounds.width &&
           y >= bounds.y && y < bounds.y + bounds.height;
}

bool Window::intersects_rect(const Rect& rect) const {
    return !(bounds.x >= rect.x + rect.width ||
             bounds.x + bounds.width <= rect.x ||
             bounds.y >= rect.y + rect.height ||
             bounds.y + bounds.height <= rect.y);
}

void Window::add_child_window(std::shared_ptr<Window> child) {
    if (child) {
        child->set_parent_window(this);
        child_windows.push_back(child);
    }
}

void Window::remove_child_window(std::shared_ptr<Window> child) {
    if (child) {
        child->set_parent_window(nullptr);
        child_windows.erase(
            std::remove(child_windows.begin(), child_windows.end(), child),
            child_windows.end());
    }
}

std::vector<std::shared_ptr<Window>> Window::get_child_windows() const {
    return child_windows;
}

void Window::set_parent_window(Window* parent) {
    parent_window = parent;
}

void Window::bring_to_front() {
    // This would be implemented by the window manager
    std::cout << "[WINDOW] Bringing window " << window_id << " to front" << std::endl;
}

void Window::send_to_back() {
    // This would be implemented by the window manager
    std::cout << "[WINDOW] Sending window " << window_id << " to back" << std::endl;
}

void Window::center_on_screen(int screen_width, int screen_height) {
    int x = (screen_width - bounds.width) / 2;
    int y = (screen_height - bounds.height) / 2;
    set_position(x, y);
}

void Window::center_on_parent() {
    if (parent_window) {
        const Rect& parent_bounds = parent_window->get_bounds();
        int x = parent_bounds.x + (parent_bounds.width - bounds.width) / 2;
        int y = parent_bounds.y + (parent_bounds.height - bounds.height) / 2;
        set_position(x, y);
    }
}

Rect Window::get_client_rect() const {
    if (style & WINDOW_STYLE_BORDERLESS) {
        return Rect(0, 0, bounds.width, bounds.height);
    } else {
        return Rect(1, 31, bounds.width - 2, bounds.height - 32);
    }
}

Rect Window::get_title_bar_rect() const {
    if (style & WINDOW_STYLE_BORDERLESS) {
        return Rect(0, 0, 0, 0);
    } else {
        return Rect(0, 0, bounds.width, 30);
    }
}

void Window::print_window_info() const {
    std::cout << "[WINDOW] Window " << window_id << " \"" << title << "\":" << std::endl;
    std::cout << "  Position: (" << bounds.x << ", " << bounds.y << ")" << std::endl;
    std::cout << "  Size: " << bounds.width << "x" << bounds.height << std::endl;
    std::cout << "  State: " << static_cast<int>(state) << std::endl;
    std::cout << "  Visible: " << (visible ? "Yes" : "No") << std::endl;
    std::cout << "  Focused: " << (focused ? "Yes" : "No") << std::endl;
    std::cout << "  Child windows: " << child_windows.size() << std::endl;
}
