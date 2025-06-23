#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <vector>
#include <queue>
#include <string>
#include <mutex>
#include <functional>

// Key codes
enum KeyCode {
    KEY_UNKNOWN = 0,
    KEY_ESCAPE = 1,
    KEY_1 = 2, KEY_2 = 3, KEY_3 = 4, KEY_4 = 5, KEY_5 = 6,
    KEY_6 = 7, KEY_7 = 8, KEY_8 = 9, KEY_9 = 10, KEY_0 = 11,
    KEY_MINUS = 12, KEY_EQUALS = 13, KEY_BACKSPACE = 14,
    KEY_TAB = 15,
    KEY_Q = 16, KEY_W = 17, KEY_E = 18, KEY_R = 19, KEY_T = 20,
    KEY_Y = 21, KEY_U = 22, KEY_I = 23, KEY_O = 24, KEY_P = 25,
    KEY_LEFTBRACKET = 26, KEY_RIGHTBRACKET = 27, KEY_ENTER = 28,
    KEY_LEFTCTRL = 29,
    KEY_A = 30, KEY_S = 31, KEY_D = 32, KEY_F = 33, KEY_G = 34,
    KEY_H = 35, KEY_J = 36, KEY_K = 37, KEY_L = 38,
    KEY_SEMICOLON = 39, KEY_APOSTROPHE = 40, KEY_GRAVE = 41,
    KEY_LEFTSHIFT = 42, KEY_BACKSLASH = 43,
    KEY_Z = 44, KEY_X = 45, KEY_C = 46, KEY_V = 47, KEY_B = 48,
    KEY_N = 49, KEY_M = 50, KEY_COMMA = 51, KEY_PERIOD = 52,
    KEY_SLASH = 53, KEY_RIGHTSHIFT = 54,
    KEY_MULTIPLY = 55, KEY_LEFTALT = 56, KEY_SPACE = 57,
    KEY_CAPSLOCK = 58,
    KEY_F1 = 59, KEY_F2 = 60, KEY_F3 = 61, KEY_F4 = 62,
    KEY_F5 = 63, KEY_F6 = 64, KEY_F7 = 65, KEY_F8 = 66,
    KEY_F9 = 67, KEY_F10 = 68,
    KEY_NUMLOCK = 69, KEY_SCROLLLOCK = 70,
    KEY_HOME = 71, KEY_UP = 72, KEY_PAGEUP = 73,
    KEY_LEFT = 75, KEY_RIGHT = 77,
    KEY_END = 79, KEY_DOWN = 80, KEY_PAGEDOWN = 81,
    KEY_INSERT = 82, KEY_DELETE = 83,
    KEY_F11 = 87, KEY_F12 = 88,
    KEY_RIGHTCTRL = 157, KEY_RIGHTALT = 184
};

// Key event types
enum KeyEventType {
    KEY_PRESSED,
    KEY_RELEASED
};

// Key event structure
struct KeyEvent {
    KeyCode keycode;
    KeyEventType type;
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    char ascii_char;
    uint64_t timestamp;
    
    KeyEvent() : keycode(KEY_UNKNOWN), type(KEY_PRESSED), 
                 shift_pressed(false), ctrl_pressed(false), alt_pressed(false),
                 ascii_char(0), timestamp(0) {}
};

class KeyboardDriver {
private:
    std::queue<KeyEvent> event_queue;
    std::mutex keyboard_mutex;
    
    // Keyboard state
    bool key_states[256];
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
    
    // Event callbacks
    std::vector<std::function<void(const KeyEvent&)>> event_callbacks;
    
    // Scancode to keycode mapping
    KeyCode scancode_to_keycode[256];
    
    // ASCII conversion
    char keycode_to_ascii(KeyCode keycode, bool shift, bool caps);
    
    // Hardware simulation
    bool hardware_initialized;
    void simulate_keyboard_input();

public:
    KeyboardDriver();
    ~KeyboardDriver();
    
    bool initialize();
    void shutdown();
    
    // Event handling
    void handle_interrupt();
    void process_scancode(uint8_t scancode);
    
    // Event queue management
    bool has_events();
    KeyEvent get_next_event();
    void clear_events();
    
    // Keyboard state
    bool is_key_pressed(KeyCode keycode);
    bool is_shift_pressed() const { return shift_pressed; }
    bool is_ctrl_pressed() const { return ctrl_pressed; }
    bool is_alt_pressed() const { return alt_pressed; }
    bool is_caps_lock() const { return caps_lock; }
    bool is_num_lock() const { return num_lock; }
    bool is_scroll_lock() const { return scroll_lock; }
    
    // Event callbacks
    void add_event_callback(std::function<void(const KeyEvent&)> callback);
    void remove_all_callbacks();
    
    // Utility functions
    std::string keycode_to_string(KeyCode keycode);
    KeyCode string_to_keycode(const std::string& str);
    
    // LED control
    void set_leds(bool caps, bool num, bool scroll);
    void update_leds();
    
    // Debug functions
    void print_keyboard_state();
    void inject_key_event(KeyCode keycode, KeyEventType type);
};

#endif
