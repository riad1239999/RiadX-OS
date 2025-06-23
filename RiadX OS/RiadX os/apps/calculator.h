#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <vector>
#include <string>
#include <memory>
#include <stack>
#include "../gui/window.h"

class CalculatorApp {
private:
    std::shared_ptr<Window> main_window;
    
    // Calculator state
    std::string display_text;
    std::string current_number;
    std::string stored_number;
    std::string current_operator;
    bool waiting_for_operand;
    bool decimal_entered;
    bool error_state;
    
    // History
    std::vector<std::string> calculation_history;
    int max_history_size;
    
    // Memory functions
    double memory_value;
    
    // UI layout
    static const int BUTTON_WIDTH = 60;
    static const int BUTTON_HEIGHT = 40;
    static const int BUTTON_MARGIN = 5;
    static const int DISPLAY_HEIGHT = 60;
    static const int HISTORY_PANEL_WIDTH = 200;
    
    // Button layout
    struct CalculatorButton {
        std::string text;
        std::string action;
        int x, y, width, height;
        Color color;
        Color text_color;
        bool enabled;
        
        CalculatorButton(const std::string& t, const std::string& a, int x, int y, 
                        int w = BUTTON_WIDTH, int h = BUTTON_HEIGHT,
                        Color bg = Color(220, 220, 220), Color fg = Color(0, 0, 0))
            : text(t), action(a), x(x), y(y), width(w), height(h), 
              color(bg), text_color(fg), enabled(true) {}
    };
    
    std::vector<CalculatorButton> buttons;
    
    // Settings
    bool show_history;
    bool scientific_mode;
    int decimal_places;
    
    // Calculator operations
    double perform_calculation(double operand1, double operand2, const std::string& op);
    double parse_number(const std::string& number_str);
    std::string format_number(double number);
    bool is_valid_number(const std::string& str);
    
    // Button actions
    void handle_number_button(const std::string& digit);
    void handle_operator_button(const std::string& op);
    void handle_equals_button();
    void handle_clear_button();
    void handle_clear_entry_button();
    void handle_decimal_button();
    void handle_sign_button();
    void handle_percent_button();
    void handle_square_root_button();
    void handle_square_button();
    void handle_inverse_button();
    void handle_memory_button(const std::string& memory_op);
    void handle_backspace_button();
    
    // Scientific calculator functions
    void handle_scientific_function(const std::string& function);
    double calculate_sin(double x);
    double calculate_cos(double x);
    double calculate_tan(double x);
    double calculate_log(double x);
    double calculate_ln(double x);
    double calculate_power(double base, double exponent);
    double calculate_factorial(double n);
    
    // UI functions
    void create_standard_buttons();
    void create_scientific_buttons();
    void draw_display(PixelBuffer* buffer);
    void draw_buttons(PixelBuffer* buffer);
    void draw_history_panel(PixelBuffer* buffer);
    void draw_memory_indicator(PixelBuffer* buffer);
    
    // Event handling
    void handle_button_click(int x, int y);
    CalculatorButton* get_button_at_position(int x, int y);
    
    // History management
    void add_to_history(const std::string& calculation);
    void clear_history();
    
    // Error handling
    void set_error_state(const std::string& error_message);
    void clear_error_state();
    
    // Utility functions
    std::string trim_trailing_zeros(const std::string& str);
    bool is_integer(double value);
    double degrees_to_radians(double degrees);
    double radians_to_degrees(double radians);

public:
    CalculatorApp();
    ~CalculatorApp();
    
    bool initialize();
    void show();
    void hide();
    
    std::shared_ptr<Window> get_window() const { return main_window; }
    
    // Calculator modes
    void set_scientific_mode(bool enabled);
    void set_show_history(bool show);
    void set_decimal_places(int places);
    
    // Memory operations
    void memory_store(double value);
    void memory_recall();
    void memory_clear();
    void memory_add(double value);
    void memory_subtract(double value);
    
    // Utility functions
    void reset();
    std::string get_current_value() const;
    void set_current_value(const std::string& value);
};

#endif
