#include "keyboard.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <cstring>

KeyboardDriver::KeyboardDriver() 
    : shift_pressed(false), ctrl_pressed(false), alt_pressed(false),
      caps_lock(false), num_lock(true), scroll_lock(false),
      hardware_initialized(false) {
    
    std::cout << "[KEYBOARD] Keyboard driver initializing..." << std::endl;
    
    // Initialize key states
    std::memset(key_states, false, sizeof(key_states));
    
    // Initialize scancode to keycode mapping
    std::memset(scancode_to_keycode, KEY_UNKNOWN, sizeof(scancode_to_keycode));
    
    // Basic scancode mappings (simplified)
    scancode_to_keycode[0x01] = KEY_ESCAPE;
    scancode_to_keycode[0x02] = KEY_1;
    scancode_to_keycode[0x03] = KEY_2;
    scancode_to_keycode[0x04] = KEY_3;
    scancode_to_keycode[0x05] = KEY_4;
    scancode_to_keycode[0x06] = KEY_5;
    scancode_to_keycode[0x07] = KEY_6;
    scancode_to_keycode[0x08] = KEY_7;
    scancode_to_keycode[0x09] = KEY_8;
    scancode_to_keycode[0x0A] = KEY_9;
    scancode_to_keycode[0x0B] = KEY_0;
    scancode_to_keycode[0x0C] = KEY_MINUS;
    scancode_to_keycode[0x0D] = KEY_EQUALS;
    scancode_to_keycode[0x0E] = KEY_BACKSPACE;
    scancode_to_keycode[0x0F] = KEY_TAB;
    scancode_to_keycode[0x10] = KEY_Q;
    scancode_to_keycode[0x11] = KEY_W;
    scancode_to_keycode[0x12] = KEY_E;
    scancode_to_keycode[0x13] = KEY_R;
    scancode_to_keycode[0x14] = KEY_T;
    scancode_to_keycode[0x15] = KEY_Y;
    scancode_to_keycode[0x16] = KEY_U;
    scancode_to_keycode[0x17] = KEY_I;
    scancode_to_keycode[0x18] = KEY_O;
    scancode_to_keycode[0x19] = KEY_P;
    scancode_to_keycode[0x1C] = KEY_ENTER;
    scancode_to_keycode[0x1D] = KEY_LEFTCTRL;
    scancode_to_keycode[0x1E] = KEY_A;
    scancode_to_keycode[0x1F] = KEY_S;
    scancode_to_keycode[0x20] = KEY_D;
    scancode_to_keycode[0x21] = KEY_F;
    scancode_to_keycode[0x22] = KEY_G;
    scancode_to_keycode[0x23] = KEY_H;
    scancode_to_keycode[0x24] = KEY_J;
    scancode_to_keycode[0x25] = KEY_K;
    scancode_to_keycode[0x26] = KEY_L;
    scancode_to_keycode[0x2A] = KEY_LEFTSHIFT;
    scancode_to_keycode[0x2C] = KEY_Z;
    scancode_to_keycode[0x2D] = KEY_X;
    scancode_to_keycode[0x2E] = KEY_C;
    scancode_to_keycode[0x2F] = KEY_V;
    scancode_to_keycode[0x30] = KEY_B;
    scancode_to_keycode[0x31] = KEY_N;
    scancode_to_keycode[0x32] = KEY_M;
    scancode_to_keycode[0x36] = KEY_RIGHTSHIFT;
    scancode_to_keycode[0x38] = KEY_LEFTALT;
    scancode_to_keycode[0x39] = KEY_SPACE;
    scancode_to_keycode[0x3A] = KEY_CAPSLOCK;
    scancode_to_keycode[0x3B] = KEY_F1;
    scancode_to_keycode[0x3C] = KEY_F2;
    scancode_to_keycode[0x3D] = KEY_F3;
    scancode_to_keycode[0x3E] = KEY_F4;
    scancode_to_keycode[0x3F] = KEY_F5;
    scancode_to_keycode[0x40] = KEY_F6;
    scancode_to_keycode[0x41] = KEY_F7;
    scancode_to_keycode[0x42] = KEY_F8;
    scancode_to_keycode[0x43] = KEY_F9;
    scancode_to_keycode[0x44] = KEY_F10;
    scancode_to_keycode[0x48] = KEY_UP;
    scancode_to_keycode[0x4B] = KEY_LEFT;
    scancode_to_keycode[0x4D] = KEY_RIGHT;
    scancode_to_keycode[0x50] = KEY_DOWN;
}

KeyboardDriver::~KeyboardDriver() {
    shutdown();
}

bool KeyboardDriver::initialize() {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    
    try {
        // Initialize hardware (simulated)
        hardware_initialized = true;
        
        // Start keyboard input simulation thread
        std::thread input_thread(&KeyboardDriver::simulate_keyboard_input, this);
        input_thread.detach();
        
        // Set initial LED state
        update_leds();
        
        std::cout << "[KEYBOARD] Keyboard driver initialized" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[KEYBOARD] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void KeyboardDriver::shutdown() {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    
    hardware_initialized = false;
    event_callbacks.clear();
    
    // Clear event queue
    while (!event_queue.empty()) {
        event_queue.pop();
    }
    
    std::cout << "[KEYBOARD] Keyboard driver shutdown complete" << std::endl;
}

void KeyboardDriver::handle_interrupt() {
    // In a real system, this would read from the keyboard controller
    // For simulation, we'll generate some events
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> key_dis(KEY_A, KEY_Z);
    static std::uniform_int_distribution<> type_dis(0, 1);
    
    if (gen() % 1000 < 5) { // 0.5% chance per interrupt
        KeyCode key = static_cast<KeyCode>(key_dis(gen));
        KeyEventType type = static_cast<KeyEventType>(type_dis(gen));
        inject_key_event(key, type);
    }
}

void KeyboardDriver::process_scancode(uint8_t scancode) {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    
    bool key_released = (scancode & 0x80) != 0;
    uint8_t key_scancode = scancode & 0x7F;
    
    KeyCode keycode = scancode_to_keycode[key_scancode];
    if (keycode == KEY_UNKNOWN) return;
    
    // Update key state
    key_states[keycode] = !key_released;
    
    // Update modifier states
    if (keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
        shift_pressed = !key_released;
    } else if (keycode == KEY_LEFTCTRL || keycode == KEY_RIGHTCTRL) {
        ctrl_pressed = !key_released;
    } else if (keycode == KEY_LEFTALT || keycode == KEY_RIGHTALT) {
        alt_pressed = !key_released;
    } else if (keycode == KEY_CAPSLOCK && !key_released) {
        caps_lock = !caps_lock;
        update_leds();
    } else if (keycode == KEY_NUMLOCK && !key_released) {
        num_lock = !num_lock;
        update_leds();
    } else if (keycode == KEY_SCROLLLOCK && !key_released) {
        scroll_lock = !scroll_lock;
        update_leds();
    }
    
    // Create key event
    KeyEvent event;
    event.keycode = keycode;
    event.type = key_released ? KEY_RELEASED : KEY_PRESSED;
    event.shift_pressed = shift_pressed;
    event.ctrl_pressed = ctrl_pressed;
    event.alt_pressed = alt_pressed;
    event.ascii_char = keycode_to_ascii(keycode, shift_pressed, caps_lock);
    event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Add to event queue
    event_queue.push(event);
    
    // Call event callbacks
    for (auto& callback : event_callbacks) {
        callback(event);
    }
    
    if (!key_released) {
        std::cout << "[KEYBOARD] Key pressed: " << keycode_to_string(keycode);
        if (event.ascii_char) {
            std::cout << " ('" << event.ascii_char << "')";
        }
        std::cout << std::endl;
    }
}

bool KeyboardDriver::has_events() {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    return !event_queue.empty();
}

KeyEvent KeyboardDriver::get_next_event() {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    
    if (event_queue.empty()) {
        return KeyEvent(); // Return empty event
    }
    
    KeyEvent event = event_queue.front();
    event_queue.pop();
    return event;
}

void KeyboardDriver::clear_events() {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    
    while (!event_queue.empty()) {
        event_queue.pop();
    }
}

bool KeyboardDriver::is_key_pressed(KeyCode keycode) {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    
    if (keycode < 256) {
        return key_states[keycode];
    }
    return false;
}

char KeyboardDriver::keycode_to_ascii(KeyCode keycode, bool shift, bool caps) {
    // Handle letter keys
    if (keycode >= KEY_A && keycode <= KEY_Z) {
        char base = 'a' + (keycode - KEY_A);
        if ((shift && !caps) || (!shift && caps)) {
            return base - 32; // Convert to uppercase
        }
        return base;
    }
    
    // Handle number keys
    if (keycode >= KEY_1 && keycode <= KEY_9) {
        if (shift) {
            const char shifted_numbers[] = "!@#$%^&*()";
            return shifted_numbers[keycode - KEY_1];
        }
        return '1' + (keycode - KEY_1);
    }
    
    if (keycode == KEY_0) {
        return shift ? ')' : '0';
    }
    
    // Handle special keys
    switch (keycode) {
        case KEY_SPACE: return ' ';
        case KEY_ENTER: return '\n';
        case KEY_TAB: return '\t';
        case KEY_BACKSPACE: return '\b';
        case KEY_MINUS: return shift ? '_' : '-';
        case KEY_EQUALS: return shift ? '+' : '=';
        case KEY_LEFTBRACKET: return shift ? '{' : '[';
        case KEY_RIGHTBRACKET: return shift ? '}' : ']';
        case KEY_BACKSLASH: return shift ? '|' : '\\';
        case KEY_SEMICOLON: return shift ? ':' : ';';
        case KEY_APOSTROPHE: return shift ? '"' : '\'';
        case KEY_GRAVE: return shift ? '~' : '`';
        case KEY_COMMA: return shift ? '<' : ',';
        case KEY_PERIOD: return shift ? '>' : '.';
        case KEY_SLASH: return shift ? '?' : '/';
        default: return 0;
    }
}

void KeyboardDriver::add_event_callback(std::function<void(const KeyEvent&)> callback) {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    event_callbacks.push_back(callback);
}

void KeyboardDriver::remove_all_callbacks() {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    event_callbacks.clear();
}

std::string KeyboardDriver::keycode_to_string(KeyCode keycode) {
    switch (keycode) {
        case KEY_A: return "A"; case KEY_B: return "B"; case KEY_C: return "C";
        case KEY_D: return "D"; case KEY_E: return "E"; case KEY_F: return "F";
        case KEY_G: return "G"; case KEY_H: return "H"; case KEY_I: return "I";
        case KEY_J: return "J"; case KEY_K: return "K"; case KEY_L: return "L";
        case KEY_M: return "M"; case KEY_N: return "N"; case KEY_O: return "O";
        case KEY_P: return "P"; case KEY_Q: return "Q"; case KEY_R: return "R";
        case KEY_S: return "S"; case KEY_T: return "T"; case KEY_U: return "U";
        case KEY_V: return "V"; case KEY_W: return "W"; case KEY_X: return "X";
        case KEY_Y: return "Y"; case KEY_Z: return "Z";
        case KEY_1: return "1"; case KEY_2: return "2"; case KEY_3: return "3";
        case KEY_4: return "4"; case KEY_5: return "5"; case KEY_6: return "6";
        case KEY_7: return "7"; case KEY_8: return "8"; case KEY_9: return "9";
        case KEY_0: return "0";
        case KEY_SPACE: return "SPACE"; case KEY_ENTER: return "ENTER";
        case KEY_BACKSPACE: return "BACKSPACE"; case KEY_TAB: return "TAB";
        case KEY_ESCAPE: return "ESCAPE";
        case KEY_LEFTSHIFT: return "LSHIFT"; case KEY_RIGHTSHIFT: return "RSHIFT";
        case KEY_LEFTCTRL: return "LCTRL"; case KEY_RIGHTCTRL: return "RCTRL";
        case KEY_LEFTALT: return "LALT"; case KEY_RIGHTALT: return "RALT";
        case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN";
        case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
        case KEY_F1: return "F1"; case KEY_F2: return "F2"; case KEY_F3: return "F3";
        case KEY_F4: return "F4"; case KEY_F5: return "F5"; case KEY_F6: return "F6";
        case KEY_F7: return "F7"; case KEY_F8: return "F8"; case KEY_F9: return "F9";
        case KEY_F10: return "F10"; case KEY_F11: return "F11"; case KEY_F12: return "F12";
        default: return "UNKNOWN";
    }
}

KeyCode KeyboardDriver::string_to_keycode(const std::string& str) {
    if (str == "A") return KEY_A; if (str == "B") return KEY_B;
    if (str == "C") return KEY_C; if (str == "D") return KEY_D;
    if (str == "E") return KEY_E; if (str == "F") return KEY_F;
    if (str == "G") return KEY_G; if (str == "H") return KEY_H;
    if (str == "I") return KEY_I; if (str == "J") return KEY_J;
    if (str == "K") return KEY_K; if (str == "L") return KEY_L;
    if (str == "M") return KEY_M; if (str == "N") return KEY_N;
    if (str == "O") return KEY_O; if (str == "P") return KEY_P;
    if (str == "Q") return KEY_Q; if (str == "R") return KEY_R;
    if (str == "S") return KEY_S; if (str == "T") return KEY_T;
    if (str == "U") return KEY_U; if (str == "V") return KEY_V;
    if (str == "W") return KEY_W; if (str == "X") return KEY_X;
    if (str == "Y") return KEY_Y; if (str == "Z") return KEY_Z;
    if (str == "SPACE") return KEY_SPACE;
    if (str == "ENTER") return KEY_ENTER;
    return KEY_UNKNOWN;
}

void KeyboardDriver::set_leds(bool caps, bool num, bool scroll) {
    caps_lock = caps;
    num_lock = num;
    scroll_lock = scroll;
    update_leds();
}

void KeyboardDriver::update_leds() {
    // In a real system, this would send commands to the keyboard controller
    std::cout << "[KEYBOARD] LEDs: CAPS=" << (caps_lock ? "ON" : "OFF")
              << " NUM=" << (num_lock ? "ON" : "OFF")
              << " SCROLL=" << (scroll_lock ? "ON" : "OFF") << std::endl;
}

void KeyboardDriver::print_keyboard_state() {
    std::lock_guard<std::mutex> lock(keyboard_mutex);
    
    std::cout << "[KEYBOARD] Keyboard State:" << std::endl;
    std::cout << "  Shift: " << (shift_pressed ? "Pressed" : "Released") << std::endl;
    std::cout << "  Ctrl: " << (ctrl_pressed ? "Pressed" : "Released") << std::endl;
    std::cout << "  Alt: " << (alt_pressed ? "Pressed" : "Released") << std::endl;
    std::cout << "  Caps Lock: " << (caps_lock ? "ON" : "OFF") << std::endl;
    std::cout << "  Num Lock: " << (num_lock ? "ON" : "OFF") << std::endl;
    std::cout << "  Scroll Lock: " << (scroll_lock ? "ON" : "OFF") << std::endl;
    std::cout << "  Events in queue: " << event_queue.size() << std::endl;
}

void KeyboardDriver::inject_key_event(KeyCode keycode, KeyEventType type) {
    uint8_t scancode = 0;
    
    // Find scancode for keycode (reverse lookup)
    for (int i = 0; i < 256; i++) {
        if (scancode_to_keycode[i] == keycode) {
            scancode = i;
            break;
        }
    }
    
    if (type == KEY_RELEASED) {
        scancode |= 0x80;
    }
    
    process_scancode(scancode);
}

void KeyboardDriver::simulate_keyboard_input() {
    // Simulate periodic keyboard input for demonstration
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dis(2000, 8000);
    std::uniform_int_distribution<> key_dis(KEY_A, KEY_Z);
    
    while (hardware_initialized) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dis(gen)));
        
        if (!hardware_initialized) break;
        
        // Simulate a key press and release
        KeyCode key = static_cast<KeyCode>(key_dis(gen));
        inject_key_event(key, KEY_PRESSED);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        inject_key_event(key, KEY_RELEASED);
    }
}
