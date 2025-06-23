#include "calculator.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

CalculatorApp::CalculatorApp()
    : display_text("0"), waiting_for_operand(true), decimal_entered(false),
      error_state(false), memory_value(0.0), max_history_size(50),
      show_history(true), scientific_mode(false), decimal_places(10) {
    
    std::cout << "[CALCULATOR] Calculator initializing..." << std::endl;
}

CalculatorApp::~CalculatorApp() {
    std::cout << "[CALCULATOR] Calculator shutting down" << std::endl;
}

bool CalculatorApp::initialize() {
    // Create main window
    int window_width = scientific_mode ? 400 : 300;
    if (show_history) window_width += HISTORY_PANEL_WIDTH;
    
    main_window = std::make_shared<Window>("Calculator", 200, 200, window_width, 480);
    
    if (!main_window) {
        std::cerr << "[CALCULATOR] Failed to create main window" << std::endl;
        return false;
    }
    
    // Set up window properties
    main_window->set_resizable(false);
    main_window->set_background_color(Color(240, 240, 240));
    
    // Create buttons
    create_standard_buttons();
    if (scientific_mode) {
        create_scientific_buttons();
    }
    
    // Set up paint callback
    main_window->set_paint_callback([this](PixelBuffer* buffer) {
        // Clear background
        buffer->clear(Color(240, 240, 240));
        
        // Draw UI components
        draw_display(buffer);
        draw_buttons(buffer);
        draw_memory_indicator(buffer);
        
        if (show_history) {
            draw_history_panel(buffer);
        }
    });
    
    // Set up event callbacks
    main_window->set_mouse_event_callback([this](const MouseEvent& event) {
        if (event.type == MOUSE_BUTTON_PRESSED && event.button == MOUSE_BUTTON_LEFT) {
            handle_button_click(event.x, event.y);
        }
    });
    
    main_window->set_key_event_callback([this](const KeyEvent& event) {
        if (event.type == KEY_PRESSED) {
            char c = event.ascii_char;
            
            // Handle numeric input
            if (c >= '0' && c <= '9') {
                handle_number_button(std::string(1, c));
            }
            // Handle operators
            else if (c == '+' || c == '-' || c == '*' || c == '/') {
                handle_operator_button(std::string(1, c));
            }
            // Handle special keys
            else if (c == '.') {
                handle_decimal_button();
            }
            else if (c == '=' || event.keycode == KEY_ENTER) {
                handle_equals_button();
            }
            else if (c == '%') {
                handle_percent_button();
            }
            else if (event.keycode == KEY_ESCAPE) {
                handle_clear_button();
            }
            else if (event.keycode == KEY_BACKSPACE) {
                handle_backspace_button();
            }
            else if (event.keycode == KEY_DELETE) {
                handle_clear_entry_button();
            }
        }
    });
    
    std::cout << "[CALCULATOR] Calculator initialized" << std::endl;
    return true;
}

void CalculatorApp::show() {
    if (main_window) {
        main_window->show();
    }
}

void CalculatorApp::hide() {
    if (main_window) {
        main_window->hide();
    }
}

void CalculatorApp::create_standard_buttons() {
    buttons.clear();
    
    // Row 1: Memory and Clear buttons
    buttons.emplace_back("MC", "memory_clear", 10, 80, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 200, 255));
    buttons.emplace_back("MR", "memory_recall", 80, 80, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 200, 255));
    buttons.emplace_back("M+", "memory_add", 150, 80, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 200, 255));
    buttons.emplace_back("M-", "memory_subtract", 220, 80, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 200, 255));
    
    // Row 2: Clear and operations
    buttons.emplace_back("C", "clear", 10, 130, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 200));
    buttons.emplace_back("CE", "clear_entry", 80, 130, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 200));
    buttons.emplace_back("⌫", "backspace", 150, 130, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 200));
    buttons.emplace_back("±", "sign", 220, 130, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 255, 200));
    
    // Row 3: 7, 8, 9, /
    buttons.emplace_back("7", "number_7", 10, 180);
    buttons.emplace_back("8", "number_8", 80, 180);
    buttons.emplace_back("9", "number_9", 150, 180);
    buttons.emplace_back("÷", "divide", 220, 180, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 255, 200));
    
    // Row 4: 4, 5, 6, *
    buttons.emplace_back("4", "number_4", 10, 230);
    buttons.emplace_back("5", "number_5", 80, 230);
    buttons.emplace_back("6", "number_6", 150, 230);
    buttons.emplace_back("×", "multiply", 220, 230, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 255, 200));
    
    // Row 5: 1, 2, 3, -
    buttons.emplace_back("1", "number_1", 10, 280);
    buttons.emplace_back("2", "number_2", 80, 280);
    buttons.emplace_back("3", "number_3", 150, 280);
    buttons.emplace_back("−", "subtract", 220, 280, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 255, 200));
    
    // Row 6: 0, ., =, +
    buttons.emplace_back("0", "number_0", 10, 330, BUTTON_WIDTH * 2 + BUTTON_MARGIN, BUTTON_HEIGHT);
    buttons.emplace_back(".", "decimal", 150, 330);
    buttons.emplace_back("+", "add", 220, 280, BUTTON_WIDTH, BUTTON_HEIGHT * 2 + BUTTON_MARGIN, Color(255, 255, 200));
    
    // Row 7: Scientific functions
    buttons.emplace_back("√", "sqrt", 10, 380, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 255, 200));
    buttons.emplace_back("x²", "square", 80, 380, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 255, 200));
    buttons.emplace_back("1/x", "inverse", 150, 380, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 255, 200));
    buttons.emplace_back("=", "equals", 220, 330, BUTTON_WIDTH, BUTTON_HEIGHT, Color(200, 255, 200));
}

void CalculatorApp::create_scientific_buttons() {
    // Add scientific function buttons
    int sci_x = 290;
    
    buttons.emplace_back("sin", "sin", sci_x, 80, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 255));
    buttons.emplace_back("cos", "cos", sci_x, 130, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 255));
    buttons.emplace_back("tan", "tan", sci_x, 180, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 255));
    buttons.emplace_back("log", "log", sci_x, 230, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 255));
    buttons.emplace_back("ln", "ln", sci_x, 280, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 255));
    buttons.emplace_back("x^y", "power", sci_x, 330, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 255));
    buttons.emplace_back("n!", "factorial", sci_x, 380, BUTTON_WIDTH, BUTTON_HEIGHT, Color(255, 200, 255));
}

void CalculatorApp::draw_display(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    
    // Draw display background
    buffer->fill_rect(Rect(10, 10, bounds.width - 20, DISPLAY_HEIGHT), Color(255, 255, 255));
    // Draw display border - simplified for now
    buffer->draw_text(bounds.width - 20 - 8, 35, "|", Color(0, 0, 0));
    
    // Draw display text (right-aligned)
    std::string display = error_state ? "Error" : display_text;
    int text_width = display.length() * 8; // Assuming 8-pixel wide characters
    int text_x = bounds.width - 20 - text_width;
    int text_y = 35;
    
    Color text_color = error_state ? Color(255, 0, 0) : Color(0, 0, 0);
    buffer->draw_text(text_x, text_y, display, text_color);
    
    // Draw current operation indicator
    if (!current_operator.empty() && !waiting_for_operand) {
        std::string op_indicator = stored_number + " " + current_operator;
        buffer->draw_text(15, 15, op_indicator, Color(128, 128, 128));
    }
}

void CalculatorApp::draw_buttons(PixelBuffer* buffer) {
    for (const auto& button : buttons) {
        if (!button.enabled) continue;
        
        // Draw button background
        buffer->fill_rect(Rect(button.x, button.y, button.width, button.height), button.color);
        
        // Draw button text (centered)
        int text_x = button.x + (button.width - button.text.length() * 8) / 2;
        int text_y = button.y + (button.height - 8) / 2;
        buffer->draw_text(text_x, text_y, button.text, button.text_color);
    }
}

void CalculatorApp::draw_history_panel(PixelBuffer* buffer) {
    if (!show_history) return;
    
    const Rect& bounds = main_window->get_bounds();
    int panel_x = bounds.width - HISTORY_PANEL_WIDTH - 10;
    
    // Draw history panel background
    buffer->fill_rect(Rect(panel_x, 10, HISTORY_PANEL_WIDTH, bounds.height - 20), Color(250, 250, 250));
    
    // Draw history title
    buffer->draw_text(panel_x + 5, 25, "History", Color(0, 0, 0));
    
    // Draw history items
    int item_y = 45;
    int visible_items = std::min(static_cast<int>(calculation_history.size()), 20);
    
    for (int i = calculation_history.size() - visible_items; i < static_cast<int>(calculation_history.size()); i++) {
        if (i >= 0 && item_y < bounds.height - 30) {
            std::string item = calculation_history[i];
            if (item.length() > 22) {
                item = item.substr(0, 19) + "...";
            }
            buffer->draw_text(panel_x + 5, item_y, item, Color(0, 0, 0));
            item_y += 15;
        }
    }
}

void CalculatorApp::draw_memory_indicator(PixelBuffer* buffer) {
    if (memory_value != 0.0) {
        buffer->draw_text(15, 55, "M", Color(0, 0, 255));
    }
}

void CalculatorApp::handle_button_click(int x, int y) {
    CalculatorButton* clicked_button = get_button_at_position(x, y);
    if (!clicked_button || !clicked_button->enabled) return;
    
    const std::string& action = clicked_button->action;
    
    if (action.substr(0, 7) == "number_") {
        handle_number_button(action.substr(7));
    } else if (action == "add" || action == "subtract" || action == "multiply" || action == "divide") {
        std::string op = action == "add" ? "+" : action == "subtract" ? "-" : 
                        action == "multiply" ? "*" : "/";
        handle_operator_button(op);
    } else if (action == "equals") {
        handle_equals_button();
    } else if (action == "clear") {
        handle_clear_button();
    } else if (action == "clear_entry") {
        handle_clear_entry_button();
    } else if (action == "decimal") {
        handle_decimal_button();
    } else if (action == "sign") {
        handle_sign_button();
    } else if (action == "backspace") {
        handle_backspace_button();
    } else if (action == "sqrt") {
        handle_square_root_button();
    } else if (action == "square") {
        handle_square_button();
    } else if (action == "inverse") {
        handle_inverse_button();
    } else if (action.substr(0, 7) == "memory_") {
        handle_memory_button(action.substr(7));
    } else if (scientific_mode) {
        handle_scientific_function(action);
    }
    
    main_window->invalidate();
}

CalculatorApp::CalculatorButton* CalculatorApp::get_button_at_position(int x, int y) {
    for (auto& button : buttons) {
        if (x >= button.x && x < button.x + button.width &&
            y >= button.y && y < button.y + button.height) {
            return &button;
        }
    }
    return nullptr;
}

void CalculatorApp::handle_number_button(const std::string& digit) {
    if (error_state) {
        clear_error_state();
    }
    
    if (waiting_for_operand) {
        display_text = digit;
        waiting_for_operand = false;
    } else {
        if (display_text == "0") {
            display_text = digit;
        } else {
            display_text += digit;
        }
    }
    
    current_number = display_text;
}

void CalculatorApp::handle_operator_button(const std::string& op) {
    if (error_state) return;
    
    if (!waiting_for_operand && !current_operator.empty()) {
        handle_equals_button();
    }
    
    stored_number = display_text;
    current_operator = op;
    waiting_for_operand = true;
    decimal_entered = false;
}

void CalculatorApp::handle_equals_button() {
    if (error_state || current_operator.empty() || waiting_for_operand) return;
    
    double operand1 = parse_number(stored_number);
    double operand2 = parse_number(current_number);
    
    double result = perform_calculation(operand1, operand2, current_operator);
    
    if (std::isnan(result) || std::isinf(result)) {
        set_error_state("Invalid operation");
        return;
    }
    
    // Add to history
    std::string calculation = stored_number + " " + current_operator + " " + current_number + " = " + format_number(result);
    add_to_history(calculation);
    
    display_text = format_number(result);
    current_number = display_text;
    current_operator.clear();
    stored_number.clear();
    waiting_for_operand = true;
    decimal_entered = false;
}

void CalculatorApp::handle_clear_button() {
    display_text = "0";
    current_number.clear();
    stored_number.clear();
    current_operator.clear();
    waiting_for_operand = true;
    decimal_entered = false;
    clear_error_state();
}

void CalculatorApp::handle_clear_entry_button() {
    display_text = "0";
    current_number.clear();
    waiting_for_operand = true;
    decimal_entered = false;
    clear_error_state();
}

void CalculatorApp::handle_decimal_button() {
    if (error_state) {
        clear_error_state();
    }
    
    if (waiting_for_operand) {
        display_text = "0.";
        waiting_for_operand = false;
    } else if (!decimal_entered) {
        display_text += ".";
    }
    
    decimal_entered = true;
    current_number = display_text;
}

void CalculatorApp::handle_sign_button() {
    if (error_state) return;
    
    double value = parse_number(display_text);
    value = -value;
    display_text = format_number(value);
    current_number = display_text;
}

void CalculatorApp::handle_backspace_button() {
    if (error_state) {
        clear_error_state();
        return;
    }
    
    if (!waiting_for_operand && display_text.length() > 1) {
        display_text.pop_back();
        current_number = display_text;
    } else {
        display_text = "0";
        current_number.clear();
        waiting_for_operand = true;
    }
}

void CalculatorApp::handle_square_root_button() {
    if (error_state) return;
    
    double value = parse_number(display_text);
    if (value < 0) {
        set_error_state("Invalid input");
        return;
    }
    
    double result = std::sqrt(value);
    display_text = format_number(result);
    current_number = display_text;
    waiting_for_operand = true;
    
    add_to_history("√(" + format_number(value) + ") = " + display_text);
}

void CalculatorApp::handle_square_button() {
    if (error_state) return;
    
    double value = parse_number(display_text);
    double result = value * value;
    
    display_text = format_number(result);
    current_number = display_text;
    waiting_for_operand = true;
    
    add_to_history("(" + format_number(value) + ")² = " + display_text);
}

void CalculatorApp::handle_inverse_button() {
    if (error_state) return;
    
    double value = parse_number(display_text);
    if (value == 0) {
        set_error_state("Cannot divide by zero");
        return;
    }
    
    double result = 1.0 / value;
    display_text = format_number(result);
    current_number = display_text;
    waiting_for_operand = true;
    
    add_to_history("1/(" + format_number(value) + ") = " + display_text);
}

void CalculatorApp::handle_memory_button(const std::string& memory_op) {
    if (error_state) return;
    
    if (memory_op == "clear") {
        memory_value = 0.0;
    } else if (memory_op == "recall") {
        display_text = format_number(memory_value);
        current_number = display_text;
        waiting_for_operand = true;
    } else if (memory_op == "add") {
        double value = parse_number(display_text);
        memory_value += value;
    } else if (memory_op == "subtract") {
        double value = parse_number(display_text);
        memory_value -= value;
    }
}

void CalculatorApp::handle_scientific_function(const std::string& function) {
    if (error_state) return;
    
    double value = parse_number(display_text);
    double result = 0.0;
    std::string operation;
    
    if (function == "sin") {
        result = calculate_sin(value);
        operation = "sin(" + format_number(value) + ")";
    } else if (function == "cos") {
        result = calculate_cos(value);
        operation = "cos(" + format_number(value) + ")";
    } else if (function == "tan") {
        result = calculate_tan(value);
        operation = "tan(" + format_number(value) + ")";
    } else if (function == "log") {
        result = calculate_log(value);
        operation = "log(" + format_number(value) + ")";
    } else if (function == "ln") {
        result = calculate_ln(value);
        operation = "ln(" + format_number(value) + ")";
    } else if (function == "factorial") {
        result = calculate_factorial(value);
        operation = format_number(value) + "!";
    }
    
    if (std::isnan(result) || std::isinf(result)) {
        set_error_state("Invalid operation");
        return;
    }
    
    display_text = format_number(result);
    current_number = display_text;
    waiting_for_operand = true;
    
    add_to_history(operation + " = " + display_text);
}

double CalculatorApp::perform_calculation(double operand1, double operand2, const std::string& op) {
    if (op == "+") return operand1 + operand2;
    if (op == "-") return operand1 - operand2;
    if (op == "*") return operand1 * operand2;
    if (op == "/") {
        if (operand2 == 0) return std::numeric_limits<double>::quiet_NaN();
        return operand1 / operand2;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double CalculatorApp::parse_number(const std::string& number_str) {
    try {
        return std::stod(number_str);
    } catch (...) {
        return 0.0;
    }
}

std::string CalculatorApp::format_number(double number) {
    if (is_integer(number) && std::abs(number) < 1e15) {
        return std::to_string(static_cast<long long>(number));
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimal_places) << number;
    std::string result = oss.str();
    
    return trim_trailing_zeros(result);
}

bool CalculatorApp::is_integer(double value) {
    return std::floor(value) == value;
}

std::string CalculatorApp::trim_trailing_zeros(const std::string& str) {
    std::string result = str;
    if (result.find('.') != std::string::npos) {
        result.erase(result.find_last_not_of('0') + 1, std::string::npos);
        result.erase(result.find_last_not_of('.') + 1, std::string::npos);
    }
    return result;
}

void CalculatorApp::add_to_history(const std::string& calculation) {
    calculation_history.push_back(calculation);
    if (calculation_history.size() > static_cast<size_t>(max_history_size)) {
        calculation_history.erase(calculation_history.begin());
    }
}

void CalculatorApp::set_error_state(const std::string& error_message) {
    error_state = true;
    display_text = error_message;
    std::cerr << "[CALCULATOR] Error: " << error_message << std::endl;
}

void CalculatorApp::clear_error_state() {
    error_state = false;
    if (display_text.find("Error") != std::string::npos) {
        display_text = "0";
        waiting_for_operand = true;
    }
}

double CalculatorApp::calculate_sin(double x) {
    return std::sin(degrees_to_radians(x));
}

double CalculatorApp::calculate_cos(double x) {
    return std::cos(degrees_to_radians(x));
}

double CalculatorApp::calculate_tan(double x) {
    return std::tan(degrees_to_radians(x));
}

double CalculatorApp::calculate_log(double x) {
    if (x <= 0) return std::numeric_limits<double>::quiet_NaN();
    return std::log10(x);
}

double CalculatorApp::calculate_ln(double x) {
    if (x <= 0) return std::numeric_limits<double>::quiet_NaN();
    return std::log(x);
}

double CalculatorApp::calculate_factorial(double n) {
    if (n < 0 || n > 170 || !is_integer(n)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double result = 1.0;
    for (int i = 2; i <= static_cast<int>(n); i++) {
        result *= i;
    }
    return result;
}

double CalculatorApp::degrees_to_radians(double degrees) {
    return degrees * M_PI / 180.0;
}

void CalculatorApp::set_scientific_mode(bool enabled) {
    scientific_mode = enabled;
    
    // Recreate window with new size
    int window_width = scientific_mode ? 400 : 300;
    if (show_history) window_width += HISTORY_PANEL_WIDTH;
    
    main_window->set_size(window_width, 480);
    
    // Recreate buttons
    create_standard_buttons();
    if (scientific_mode) {
        create_scientific_buttons();
    }
    
    main_window->invalidate();
}

void CalculatorApp::reset() {
    handle_clear_button();
    memory_value = 0.0;
    clear_history();
}

std::string CalculatorApp::get_current_value() const {
    return display_text;
}

void CalculatorApp::set_current_value(const std::string& value) {
    if (is_valid_number(value)) {
        display_text = value;
        current_number = value;
        waiting_for_operand = true;
        main_window->invalidate();
    }
}

bool CalculatorApp::is_valid_number(const std::string& str) {
    try {
        std::stod(str);
        return true;
    } catch (...) {
        return false;
    }
}
