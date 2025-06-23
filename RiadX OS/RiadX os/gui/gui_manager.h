#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include "window.h"
#include "graphics.h"
#include "../drivers/display.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"

// Desktop wallpaper and theme
struct Theme {
    Color desktop_background;
    Color window_title_active;
    Color window_title_inactive;
    Color window_border_active;
    Color window_border_inactive;
    Color menu_background;
    Color menu_text;
    Color button_background;
    Color button_text;
    
    Theme() : desktop_background(0, 100, 150),
              window_title_active(0, 120, 215),
              window_title_inactive(128, 128, 128),
              window_border_active(0, 0, 0),
              window_border_inactive(128, 128, 128),
              menu_background(240, 240, 240),
              menu_text(0, 0, 0),
              button_background(225, 225, 225),
              button_text(0, 0, 0) {}
};

// Desktop icons
struct DesktopIcon {
    std::string name;
    std::string executable_path;
    int x, y;
    int width, height;
    bool selected;
    
    DesktopIcon(const std::string& n, const std::string& exec, int x, int y)
        : name(n), executable_path(exec), x(x), y(y), width(64), height(64), selected(false) {}
};

// System menu
struct MenuItem {
    std::string text;
    std::string action;
    bool enabled;
    std::vector<MenuItem> submenu;
    
    MenuItem(const std::string& t, const std::string& a, bool e = true)
        : text(t), action(a), enabled(e) {}
};

class GUIManager {
private:
    DisplayDriver* display_driver;
    KeyboardDriver* keyboard_driver;
    MouseDriver* mouse_driver;
    
    std::vector<std::shared_ptr<Window>> windows;
    std::shared_ptr<Window> focused_window;
    std::shared_ptr<Window> dragging_window;
    std::mutex gui_mutex;
    
    // Desktop
    std::vector<DesktopIcon> desktop_icons;
    Theme current_theme;
    bool show_desktop;
    
    // System UI
    bool show_taskbar;
    Rect taskbar_rect;
    std::vector<MenuItem> start_menu;
    bool start_menu_open;
    
    // Mouse state
    int last_mouse_x, last_mouse_y;
    bool mouse_dragging;
    int drag_start_x, drag_start_y;
    
    // GUI thread
    std::atomic<bool> gui_running;
    std::thread gui_thread;
    
    // Event handling
    void handle_mouse_event(const MouseEvent& event);
    void handle_keyboard_event(const KeyEvent& event);
    
    // Window management
    std::shared_ptr<Window> get_window_at_point(int x, int y);
    void bring_window_to_front(std::shared_ptr<Window> window);
    void focus_window(std::shared_ptr<Window> window);
    
    // Desktop management
    void draw_desktop();
    void draw_desktop_icons();
    void draw_taskbar();
    void draw_start_menu();
    void handle_icon_click(int x, int y);
    void handle_desktop_click(int x, int y);
    
    // Window operations
    void start_window_drag(std::shared_ptr<Window> window, int x, int y);
    void update_window_drag(int x, int y);
    void end_window_drag();
    
    // System menu operations
    void toggle_start_menu();
    void handle_menu_click(int x, int y);
    void execute_menu_action(const std::string& action);
    
    // Application launching
    void launch_application(const std::string& executable_path);
    void create_sample_windows();
    
    // Rendering
    void render_frame();
    void composite_windows();
    
    // GUI main loop
    void gui_main_loop();

public:
    GUIManager(DisplayDriver* display, KeyboardDriver* keyboard, MouseDriver* mouse, FileSystem* fs = nullptr);
    ~GUIManager();
    
    bool initialize();
    void shutdown();
    void run();
    
    // Window management
    std::shared_ptr<Window> create_window(const std::string& title, int x, int y, int width, int height, int style = WINDOW_STYLE_NORMAL);
    void destroy_window(std::shared_ptr<Window> window);
    void close_window(int window_id);
    std::shared_ptr<Window> get_window(int window_id);
    std::vector<std::shared_ptr<Window>> get_all_windows();
    
    // Desktop management
    void add_desktop_icon(const std::string& name, const std::string& executable, int x, int y);
    void remove_desktop_icon(const std::string& name);
    void set_wallpaper_color(const Color& color);
    void set_theme(const Theme& theme);
    
    // System UI
    void show_task_manager();
    void show_system_settings();
    void show_about_dialog();
    
    // Utility functions
    void show_message_box(const std::string& title, const std::string& message);
    void show_error_dialog(const std::string& title, const std::string& error);
    void show_file_dialog(const std::string& title, bool save_mode = false);
    
    // Debug functions
    void print_gui_state();
    void toggle_debug_mode();
};

#endif
