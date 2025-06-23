#include "mouse.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>

MouseDriver::MouseDriver() 
    : current_x(0), current_y(0), 
      screen_width(1024), screen_height(768),
      sensitivity_x(1.0f), sensitivity_y(1.0f), acceleration_enabled(true),
      hardware_initialized(false) {
    
    std::cout << "[MOUSE] Mouse driver initializing..." << std::endl;
    
    // Initialize button states
    for (int i = 0; i < 5; i++) {
        button_states[i] = false;
    }
    
    // Set initial position to center of screen
    current_x = screen_width / 2;
    current_y = screen_height / 2;
}

MouseDriver::~MouseDriver() {
    shutdown();
}

bool MouseDriver::initialize() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    try {
        // Initialize hardware (simulated)
        hardware_initialized = true;
        
        // Start mouse input simulation thread
        std::thread input_thread(&MouseDriver::simulate_mouse_input, this);
        input_thread.detach();
        
        std::cout << "[MOUSE] Mouse driver initialized at position (" 
                  << current_x << ", " << current_y << ")" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[MOUSE] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void MouseDriver::shutdown() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    hardware_initialized = false;
    event_callbacks.clear();
    
    // Clear event queue
    while (!event_queue.empty()) {
        event_queue.pop();
    }
    
    std::cout << "[MOUSE] Mouse driver shutdown complete" << std::endl;
}

void MouseDriver::handle_interrupt() {
    // In a real system, this would read from the mouse controller
    // For simulation, we'll generate some movement events
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> move_dis(-5, 5);
    static std::uniform_int_distribution<> button_dis(0, 100);
    
    if (gen() % 100 < 10) { // 10% chance per interrupt
        int delta_x = move_dis(gen);
        int delta_y = move_dis(gen);
        
        if (delta_x != 0 || delta_y != 0) {
            inject_mouse_event(MOUSE_MOVED, current_x + delta_x, current_y + delta_y);
        }
        
        // Occasional button events
        if (button_dis(gen) > 95) {
            inject_mouse_event(MOUSE_BUTTON_PRESSED, current_x, current_y, MOUSE_BUTTON_LEFT);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            inject_mouse_event(MOUSE_BUTTON_RELEASED, current_x, current_y, MOUSE_BUTTON_LEFT);
        }
    }
}

void MouseDriver::process_mouse_packet(uint8_t packet[3]) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    // Standard PS/2 mouse packet format
    uint8_t buttons = packet[0];
    int8_t delta_x = static_cast<int8_t>(packet[1]);
    int8_t delta_y = static_cast<int8_t>(packet[2]);
    
    // Apply sensitivity and acceleration
    int adjusted_delta_x = static_cast<int>(delta_x * sensitivity_x);
    int adjusted_delta_y = static_cast<int>(delta_y * sensitivity_y);
    
    if (acceleration_enabled) {
        apply_acceleration(adjusted_delta_x, adjusted_delta_y);
    }
    
    // Update position
    int new_x = current_x + adjusted_delta_x;
    int new_y = current_y - adjusted_delta_y; // Y is inverted in mouse coordinates
    
    // Clamp to screen bounds
    new_x = std::max(0, std::min(screen_width - 1, new_x));
    new_y = std::max(0, std::min(screen_height - 1, new_y));
    
    bool moved = (new_x != current_x || new_y != current_y);
    current_x = new_x;
    current_y = new_y;
    
    // Process button states
    bool left_button = (buttons & 0x01) != 0;
    bool right_button = (buttons & 0x02) != 0;
    bool middle_button = (buttons & 0x04) != 0;
    
    // Create movement event if mouse moved
    if (moved) {
        MouseEvent event;
        event.type = MOUSE_MOVED;
        event.x = current_x;
        event.y = current_y;
        event.delta_x = adjusted_delta_x;
        event.delta_y = -adjusted_delta_y;
        event.left_pressed = left_button;
        event.right_pressed = right_button;
        event.middle_pressed = middle_button;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        event_queue.push(event);
        
        // Call event callbacks
        for (auto& callback : event_callbacks) {
            callback(event);
        }
    }
    
    // Process button changes
    if (left_button != button_states[MOUSE_BUTTON_LEFT]) {
        button_states[MOUSE_BUTTON_LEFT] = left_button;
        create_button_event(MOUSE_BUTTON_LEFT, left_button ? MOUSE_BUTTON_PRESSED : MOUSE_BUTTON_RELEASED);
    }
    
    if (right_button != button_states[MOUSE_BUTTON_RIGHT]) {
        button_states[MOUSE_BUTTON_RIGHT] = right_button;
        create_button_event(MOUSE_BUTTON_RIGHT, right_button ? MOUSE_BUTTON_PRESSED : MOUSE_BUTTON_RELEASED);
    }
    
    if (middle_button != button_states[MOUSE_BUTTON_MIDDLE]) {
        button_states[MOUSE_BUTTON_MIDDLE] = middle_button;
        create_button_event(MOUSE_BUTTON_MIDDLE, middle_button ? MOUSE_BUTTON_PRESSED : MOUSE_BUTTON_RELEASED);
    }
}

void MouseDriver::create_button_event(MouseButton button, MouseEventType type) {
    MouseEvent event;
    event.type = type;
    event.x = current_x;
    event.y = current_y;
    event.button = button;
    event.left_pressed = button_states[MOUSE_BUTTON_LEFT];
    event.right_pressed = button_states[MOUSE_BUTTON_RIGHT];
    event.middle_pressed = button_states[MOUSE_BUTTON_MIDDLE];
    event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    event_queue.push(event);
    
    // Call event callbacks
    for (auto& callback : event_callbacks) {
        callback(event);
    }
    
    std::cout << "[MOUSE] Button " << static_cast<int>(button) 
              << (type == MOUSE_BUTTON_PRESSED ? " pressed" : " released")
              << " at (" << current_x << ", " << current_y << ")" << std::endl;
}

bool MouseDriver::has_events() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    return !event_queue.empty();
}

MouseEvent MouseDriver::get_next_event() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    if (event_queue.empty()) {
        return MouseEvent(); // Return empty event
    }
    
    MouseEvent event = event_queue.front();
    event_queue.pop();
    return event;
}

void MouseDriver::clear_events() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    while (!event_queue.empty()) {
        event_queue.pop();
    }
}

void MouseDriver::get_position(int& x, int& y) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    x = current_x;
    y = current_y;
}

void MouseDriver::set_position(int x, int y) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    current_x = std::max(0, std::min(screen_width - 1, x));
    current_y = std::max(0, std::min(screen_height - 1, y));
    
    std::cout << "[MOUSE] Position set to (" << current_x << ", " << current_y << ")" << std::endl;
}

bool MouseDriver::is_button_pressed(MouseButton button) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    if (button < 5) {
        return button_states[button];
    }
    return false;
}

void MouseDriver::set_sensitivity(float x_sens, float y_sens) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    sensitivity_x = std::max(0.1f, std::min(5.0f, x_sens));
    sensitivity_y = std::max(0.1f, std::min(5.0f, y_sens));
    
    std::cout << "[MOUSE] Sensitivity set to (" << sensitivity_x << ", " << sensitivity_y << ")" << std::endl;
}

void MouseDriver::get_sensitivity(float& x_sens, float& y_sens) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    x_sens = sensitivity_x;
    y_sens = sensitivity_y;
}

void MouseDriver::set_acceleration(bool enabled) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    acceleration_enabled = enabled;
    std::cout << "[MOUSE] Acceleration " << (enabled ? "enabled" : "disabled") << std::endl;
}

bool MouseDriver::get_acceleration() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    return acceleration_enabled;
}

void MouseDriver::set_screen_bounds(int width, int height) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    screen_width = width;
    screen_height = height;
    
    // Clamp current position to new bounds
    clamp_position();
    
    std::cout << "[MOUSE] Screen bounds set to " << width << "x" << height << std::endl;
}

void MouseDriver::clamp_position() {
    current_x = std::max(0, std::min(screen_width - 1, current_x));
    current_y = std::max(0, std::min(screen_height - 1, current_y));
}

void MouseDriver::apply_acceleration(int& delta_x, int& delta_y) {
    // Simple acceleration algorithm
    float magnitude = std::sqrt(delta_x * delta_x + delta_y * delta_y);
    if (magnitude > 1.0f) {
        float acceleration_factor = 1.0f + (magnitude - 1.0f) * 0.5f;
        delta_x = static_cast<int>(delta_x * acceleration_factor);
        delta_y = static_cast<int>(delta_y * acceleration_factor);
    }
}

void MouseDriver::add_event_callback(std::function<void(const MouseEvent&)> callback) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    event_callbacks.push_back(callback);
}

void MouseDriver::remove_all_callbacks() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    event_callbacks.clear();
}

void MouseDriver::show_cursor() {
    std::cout << "[MOUSE] Cursor shown" << std::endl;
}

void MouseDriver::hide_cursor() {
    std::cout << "[MOUSE] Cursor hidden" << std::endl;
}

void MouseDriver::set_cursor_shape(int shape) {
    std::cout << "[MOUSE] Cursor shape set to " << shape << std::endl;
}

void MouseDriver::print_mouse_state() {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    std::cout << "[MOUSE] Mouse State:" << std::endl;
    std::cout << "  Position: (" << current_x << ", " << current_y << ")" << std::endl;
    std::cout << "  Left Button: " << (button_states[MOUSE_BUTTON_LEFT] ? "Pressed" : "Released") << std::endl;
    std::cout << "  Right Button: " << (button_states[MOUSE_BUTTON_RIGHT] ? "Pressed" : "Released") << std::endl;
    std::cout << "  Middle Button: " << (button_states[MOUSE_BUTTON_MIDDLE] ? "Pressed" : "Released") << std::endl;
    std::cout << "  Sensitivity: (" << sensitivity_x << ", " << sensitivity_y << ")" << std::endl;
    std::cout << "  Acceleration: " << (acceleration_enabled ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Events in queue: " << event_queue.size() << std::endl;
}

void MouseDriver::inject_mouse_event(MouseEventType type, int x, int y, MouseButton button) {
    std::lock_guard<std::mutex> lock(mouse_mutex);
    
    MouseEvent event;
    event.type = type;
    event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    switch (type) {
        case MOUSE_MOVED:
            event.delta_x = x - current_x;
            event.delta_y = y - current_y;
            current_x = std::max(0, std::min(screen_width - 1, x));
            current_y = std::max(0, std::min(screen_height - 1, y));
            event.x = current_x;
            event.y = current_y;
            break;
            
        case MOUSE_BUTTON_PRESSED:
            event.x = current_x;
            event.y = current_y;
            event.button = button;
            if (button < 5) {
                button_states[button] = true;
            }
            break;
            
        case MOUSE_BUTTON_RELEASED:
            event.x = current_x;
            event.y = current_y;
            event.button = button;
            if (button < 5) {
                button_states[button] = false;
            }
            break;
            
        case MOUSE_WHEEL_SCROLLED:
            event.x = current_x;
            event.y = current_y;
            event.wheel_delta = x; // Use x parameter as wheel delta
            break;
    }
    
    event.left_pressed = button_states[MOUSE_BUTTON_LEFT];
    event.right_pressed = button_states[MOUSE_BUTTON_RIGHT];
    event.middle_pressed = button_states[MOUSE_BUTTON_MIDDLE];
    
    event_queue.push(event);
    
    // Call event callbacks
    for (auto& callback : event_callbacks) {
        callback(event);
    }
}

void MouseDriver::simulate_mouse_input() {
    // Simulate periodic mouse input for demonstration
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_dis(3000, 10000);
    std::uniform_int_distribution<> move_dis(-20, 20);
    std::uniform_int_distribution<> button_dis(0, 100);
    
    while (hardware_initialized) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dis(gen)));
        
        if (!hardware_initialized) break;
        
        // Simulate mouse movement
        int delta_x = move_dis(gen);
        int delta_y = move_dis(gen);
        
        if (delta_x != 0 || delta_y != 0) {
            inject_mouse_event(MOUSE_MOVED, current_x + delta_x, current_y + delta_y);
        }
        
        // Occasional button clicks
        if (button_dis(gen) > 90) {
            MouseButton btn = static_cast<MouseButton>(button_dis(gen) % 3);
            inject_mouse_event(MOUSE_BUTTON_PRESSED, current_x, current_y, btn);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            inject_mouse_event(MOUSE_BUTTON_RELEASED, current_x, current_y, btn);
        }
    }
}
