#ifndef MOUSE_H
#define MOUSE_H

#include <queue>
#include <vector>
#include <mutex>
#include <functional>

// Mouse button constants
enum MouseButton {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_X1 = 3,
    MOUSE_BUTTON_X2 = 4
};

// Mouse event types
enum MouseEventType {
    MOUSE_MOVED,
    MOUSE_BUTTON_PRESSED,
    MOUSE_BUTTON_RELEASED,
    MOUSE_WHEEL_SCROLLED
};

// Mouse event structure
struct MouseEvent {
    MouseEventType type;
    int x, y;              // Current position
    int delta_x, delta_y;  // Movement delta
    MouseButton button;    // Button involved (for button events)
    int wheel_delta;       // Wheel scroll delta
    bool left_pressed;     // Current button states
    bool right_pressed;
    bool middle_pressed;
    uint64_t timestamp;
    
    MouseEvent() : type(MOUSE_MOVED), x(0), y(0), delta_x(0), delta_y(0),
                   button(MOUSE_BUTTON_LEFT), wheel_delta(0),
                   left_pressed(false), right_pressed(false), middle_pressed(false),
                   timestamp(0) {}
};

class MouseDriver {
private:
    std::queue<MouseEvent> event_queue;
    std::mutex mouse_mutex;
    
    // Mouse state
    int current_x, current_y;
    bool button_states[5];  // Support for 5 buttons
    int screen_width, screen_height;
    
    // Sensitivity settings
    float sensitivity_x;
    float sensitivity_y;
    bool acceleration_enabled;
    
    // Event callbacks
    std::vector<std::function<void(const MouseEvent&)>> event_callbacks;
    
    // Hardware simulation
    bool hardware_initialized;
    void simulate_mouse_input();
    
    // Utility functions
    void clamp_position();
    void apply_acceleration(int& delta_x, int& delta_y);

public:
    MouseDriver();
    ~MouseDriver();
    
    bool initialize();
    void shutdown();
    
    // Event handling
    void handle_interrupt();
    void process_mouse_packet(uint8_t packet[3]);
    
    // Event queue management
    bool has_events();
    MouseEvent get_next_event();
    void clear_events();
    
    // Mouse state
    void get_position(int& x, int& y);
    void set_position(int x, int y);
    bool is_button_pressed(MouseButton button);
    
    // Settings
    void set_sensitivity(float x_sens, float y_sens);
    void get_sensitivity(float& x_sens, float& y_sens);
    void set_acceleration(bool enabled);
    bool get_acceleration();
    void set_screen_bounds(int width, int height);
    
    // Event callbacks
    void add_event_callback(std::function<void(const MouseEvent&)> callback);
    void remove_all_callbacks();
    
    // Cursor control
    void show_cursor();
    void hide_cursor();
    void set_cursor_shape(int shape);
    
    // Debug functions
    void print_mouse_state();
    void inject_mouse_event(MouseEventType type, int x = 0, int y = 0, MouseButton button = MOUSE_BUTTON_LEFT);
    
private:
    void create_button_event(MouseButton button, MouseEventType type);
};

#endif
