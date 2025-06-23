#include "gui_manager.h"
#include <iostream>
#include <algorithm>
#include <chrono>

GUIManager::GUIManager(DisplayDriver* display, KeyboardDriver* keyboard, MouseDriver* mouse)
    : display_driver(display), keyboard_driver(keyboard), mouse_driver(mouse),
      show_desktop(true), show_taskbar(true), start_menu_open(false),
      last_mouse_x(0), last_mouse_y(0), mouse_dragging(false),
      drag_start_x(0), drag_start_y(0), gui_running(false) {
    
    std::cout << "[GUI] GUI Manager initializing..." << std::endl;
    
    // Initialize taskbar
    int screen_width, screen_height, bpp;
    display_driver->get_display_mode(screen_width, screen_height, bpp);
    taskbar_rect = Rect(0, screen_height - 40, screen_width, 40);
    
    // Create start menu
    start_menu.emplace_back("Applications", "applications");
    start_menu.back().submenu.emplace_back("Calculator", "launch_calculator");
    start_menu.back().submenu.emplace_back("Text Editor", "launch_editor");
    start_menu.back().submenu.emplace_back("File Manager", "launch_filemanager");
    
    start_menu.emplace_back("System", "system");
    start_menu.back().submenu.emplace_back("Task Manager", "task_manager");
    start_menu.back().submenu.emplace_back("Settings", "settings");
    start_menu.back().submenu.emplace_back("About", "about");
    
    start_menu.emplace_back("Shutdown", "shutdown");
}

GUIManager::~GUIManager() {
    shutdown();
}

bool GUIManager::initialize() {
    std::lock_guard<std::mutex> lock(gui_mutex);
    
    try {
        // Set up event callbacks
        keyboard_driver->add_event_callback([this](const KeyEvent& event) {
            handle_keyboard_event(event);
        });
        
        mouse_driver->add_event_callback([this](const MouseEvent& event) {
            handle_mouse_event(event);
        });
        
        // Create desktop icons
        add_desktop_icon("Calculator", "/bin/calculator", 50, 50);
        add_desktop_icon("Text Editor", "/bin/editor", 50, 130);
        add_desktop_icon("File Manager", "/bin/filemanager", 50, 210);
        
        // Create sample windows
        create_sample_windows();
        
        gui_running = true;
        std::cout << "[GUI] GUI Manager initialized" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[GUI] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void GUIManager::shutdown() {
    std::lock_guard<std::mutex> lock(gui_mutex);
    
    gui_running = false;
    
    // Wait for GUI thread to finish
    if (gui_thread.joinable()) {
        gui_thread.join();
    }
    
    // Close all windows
    windows.clear();
    focused_window.reset();
    dragging_window.reset();
    
    std::cout << "[GUI] GUI Manager shutdown complete" << std::endl;
}

void GUIManager::run() {
    gui_thread = std::thread(&GUIManager::gui_main_loop, this);
}

void GUIManager::gui_main_loop() {
    std::cout << "[GUI] Starting GUI main loop" << std::endl;
    
    auto last_frame_time = std::chrono::high_resolution_clock::now();
    const auto target_frame_time = std::chrono::milliseconds(16); // ~60 FPS
    
    while (gui_running) {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        // Render frame
        render_frame();
        
        // Maintain frame rate
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = frame_end - frame_start;
        
        if (frame_duration < target_frame_time) {
            std::this_thread::sleep_for(target_frame_time - frame_duration);
        }
        
        last_frame_time = frame_end;
    }
    
    std::cout << "[GUI] GUI main loop ended" << std::endl;
}

void GUIManager::render_frame() {
    // Clear screen
    display_driver->clear_screen(current_theme.desktop_background);
    
    // Draw desktop
    if (show_desktop) {
        draw_desktop();
    }
    
    // Draw windows
    composite_windows();
    
    // Draw taskbar
    if (show_taskbar) {
        draw_taskbar();
    }
    
    // Draw start menu
    if (start_menu_open) {
        draw_start_menu();
    }
    
    // Present frame
    display_driver->present();
}

void GUIManager::draw_desktop() {
    // Desktop is already cleared with background color
    draw_desktop_icons();
}

void GUIManager::draw_desktop_icons() {
    for (const auto& icon : desktop_icons) {
        Color icon_color = icon.selected ? Color(200, 200, 255) : Color(255, 255, 255);
        Color text_color = Color(0, 0, 0);
        
        // Draw icon background
        display_driver->draw_rect(Rect(icon.x, icon.y, icon.width, icon.height), icon_color, true);
        display_driver->draw_rect(Rect(icon.x, icon.y, icon.width, icon.height), Color(0, 0, 0), false);
        
        // Draw icon representation (simple rectangle for now)
        display_driver->draw_rect(Rect(icon.x + 16, icon.y + 8, 32, 32), Color(100, 100, 100), true);
        
        // Draw icon text
        display_driver->draw_text(icon.x + 4, icon.y + icon.height - 16, icon.name, text_color);
    }
}

void GUIManager::draw_taskbar() {
    // Draw taskbar background
    display_driver->draw_rect(taskbar_rect, current_theme.menu_background, true);
    display_driver->draw_rect(taskbar_rect, Color(128, 128, 128), false);
    
    // Draw start button
    Rect start_button_rect(5, taskbar_rect.y + 5, 80, 30);
    Color start_color = start_menu_open ? Color(200, 200, 200) : current_theme.button_background;
    display_driver->draw_rect(start_button_rect, start_color, true);
    display_driver->draw_rect(start_button_rect, Color(0, 0, 0), false);
    display_driver->draw_text(start_button_rect.x + 10, start_button_rect.y + 8, "Start", current_theme.button_text);
    
    // Draw window buttons in taskbar
    int button_x = 100;
    for (const auto& window : windows) {
        if (window->is_visible() && window->get_state() != WINDOW_MINIMIZED) {
            Rect button_rect(button_x, taskbar_rect.y + 5, 120, 30);
            Color button_color = (window == focused_window) ? Color(180, 180, 180) : current_theme.button_background;
            
            display_driver->draw_rect(button_rect, button_color, true);
            display_driver->draw_rect(button_rect, Color(0, 0, 0), false);
            
            // Truncate window title if too long
            std::string title = window->get_title();
            if (title.length() > 15) {
                title = title.substr(0, 12) + "...";
            }
            
            display_driver->draw_text(button_rect.x + 5, button_rect.y + 8, title, current_theme.button_text);
            button_x += 125;
        }
    }
    
    // Draw system tray area
    int tray_x = taskbar_rect.x + taskbar_rect.width - 100;
    display_driver->draw_text(tray_x, taskbar_rect.y + 12, "12:34 PM", current_theme.menu_text);
}

void GUIManager::draw_start_menu() {
    Rect menu_rect(5, taskbar_rect.y - 200, 150, 200);
    
    // Draw menu background
    display_driver->draw_rect(menu_rect, current_theme.menu_background, true);
    display_driver->draw_rect(menu_rect, Color(0, 0, 0), false);
    
    // Draw menu items
    int item_y = menu_rect.y + 10;
    for (const auto& item : start_menu) {
        Color item_color = item.enabled ? current_theme.menu_text : Color(128, 128, 128);
        display_driver->draw_text(menu_rect.x + 10, item_y, item.text, item_color);
        item_y += 20;
    }
}

void GUIManager::composite_windows() {
    // Sort windows by Z-order (back to front)
    std::vector<std::shared_ptr<Window>> sorted_windows = windows;
    
    // Render windows from back to front
    for (auto& window : sorted_windows) {
        if (window->is_visible() && window->get_state() != WINDOW_MINIMIZED) {
            window->paint();
            
            // Copy window buffer to display
            PixelBuffer* window_buffer = window->get_buffer();
            if (window_buffer) {
                const Rect& bounds = window->get_bounds();
                
                // Simple blit operation
                for (int y = 0; y < bounds.height && y < window_buffer->get_height(); y++) {
                    for (int x = 0; x < bounds.width && x < window_buffer->get_width(); x++) {
                        int screen_x = bounds.x + x;
                        int screen_y = bounds.y + y;
                        
                        if (screen_x >= 0 && screen_y >= 0) {
                            Color pixel = window_buffer->get_pixel(x, y);
                            display_driver->set_pixel(screen_x, screen_y, pixel);
                        }
                    }
                }
            }
        }
    }
}

void GUIManager::handle_mouse_event(const MouseEvent& event) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    
    last_mouse_x = event.x;
    last_mouse_y = event.y;
    
    switch (event.type) {
        case MOUSE_MOVED:
            if (mouse_dragging && dragging_window) {
                update_window_drag(event.x, event.y);
            }
            break;
            
        case MOUSE_BUTTON_PRESSED:
            if (event.button == MOUSE_BUTTON_LEFT) {
                // Check if clicking on taskbar
                if (taskbar_rect.x <= event.x && event.x < taskbar_rect.x + taskbar_rect.width &&
                    taskbar_rect.y <= event.y && event.y < taskbar_rect.y + taskbar_rect.height) {
                    
                    // Check start button
                    if (event.x >= 5 && event.x <= 85 && 
                        event.y >= taskbar_rect.y + 5 && event.y <= taskbar_rect.y + 35) {
                        toggle_start_menu();
                        break;
                    }
                    
                    // Check taskbar buttons
                    int button_x = 100;
                    for (auto& window : windows) {
                        if (window->is_visible() && window->get_state() != WINDOW_MINIMIZED) {
                            if (event.x >= button_x && event.x <= button_x + 120 &&
                                event.y >= taskbar_rect.y + 5 && event.y <= taskbar_rect.y + 35) {
                                focus_window(window);
                                break;
                            }
                            button_x += 125;
                        }
                    }
                    break;
                }
                
                // Check if clicking on start menu
                if (start_menu_open) {
                    handle_menu_click(event.x, event.y);
                    break;
                }
                
                // Check if clicking on a window
                auto window = get_window_at_point(event.x, event.y);
                if (window) {
                    focus_window(window);
                    
                    // Check if clicking on title bar for dragging
                    Rect title_bar = window->get_title_bar_rect();
                    title_bar.x += window->get_bounds().x;
                    title_bar.y += window->get_bounds().y;
                    
                    if (title_bar.x <= event.x && event.x < title_bar.x + title_bar.width &&
                        title_bar.y <= event.y && event.y < title_bar.y + title_bar.height) {
                        start_window_drag(window, event.x, event.y);
                    }
                    
                    // Forward mouse event to window
                    MouseEvent window_event = event;
                    window_event.x -= window->get_bounds().x;
                    window_event.y -= window->get_bounds().y;
                    window->handle_mouse_event(window_event);
                } else {
                    // Clicking on desktop
                    handle_desktop_click(event.x, event.y);
                }
            }
            break;
            
        case MOUSE_BUTTON_RELEASED:
            if (event.button == MOUSE_BUTTON_LEFT) {
                if (mouse_dragging) {
                    end_window_drag();
                }
            }
            break;
            
        default:
            break;
    }
}

void GUIManager::handle_keyboard_event(const KeyEvent& event) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    
    // Forward keyboard events to focused window
    if (focused_window && event.type == KEY_PRESSED) {
        focused_window->handle_key_event(event);
    }
    
    // Handle global hotkeys
    if (event.type == KEY_PRESSED) {
        if (event.ctrl_pressed && event.keycode == KEY_T) {
            // Ctrl+T: Show task manager
            show_task_manager();
        } else if (event.alt_pressed && event.keycode == KEY_F4) {
            // Alt+F4: Close focused window
            if (focused_window) {
                focused_window->close();
            }
        } else if (event.keycode == KEY_ESCAPE) {
            // Escape: Close start menu
            if (start_menu_open) {
                start_menu_open = false;
            }
        }
    }
}

std::shared_ptr<Window> GUIManager::get_window_at_point(int x, int y) {
    // Search from front to back
    for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
        auto window = *it;
        if (window->is_visible() && window->get_state() != WINDOW_MINIMIZED && 
            window->contains_point(x, y)) {
            return window;
        }
    }
    return nullptr;
}

void GUIManager::bring_window_to_front(std::shared_ptr<Window> window) {
    auto it = std::find(windows.begin(), windows.end(), window);
    if (it != windows.end()) {
        windows.erase(it);
        windows.push_back(window);
    }
}

void GUIManager::focus_window(std::shared_ptr<Window> window) {
    if (focused_window) {
        focused_window->set_focus(false);
    }
    
    focused_window = window;
    if (window) {
        window->set_focus(true);
        bring_window_to_front(window);
    }
}

void GUIManager::start_window_drag(std::shared_ptr<Window> window, int x, int y) {
    dragging_window = window;
    mouse_dragging = true;
    drag_start_x = x - window->get_bounds().x;
    drag_start_y = y - window->get_bounds().y;
}

void GUIManager::update_window_drag(int x, int y) {
    if (dragging_window) {
        int new_x = x - drag_start_x;
        int new_y = y - drag_start_y;
        dragging_window->set_position(new_x, new_y);
    }
}

void GUIManager::end_window_drag() {
    dragging_window.reset();
    mouse_dragging = false;
}

void GUIManager::toggle_start_menu() {
    start_menu_open = !start_menu_open;
}

void GUIManager::handle_menu_click(int x, int y) {
    Rect menu_rect(5, taskbar_rect.y - 200, 150, 200);
    
    if (x >= menu_rect.x && x < menu_rect.x + menu_rect.width &&
        y >= menu_rect.y && y < menu_rect.y + menu_rect.height) {
        
        int item_index = (y - menu_rect.y - 10) / 20;
        if (item_index >= 0 && item_index < static_cast<int>(start_menu.size())) {
            execute_menu_action(start_menu[item_index].action);
        }
    }
    
    start_menu_open = false;
}

void GUIManager::execute_menu_action(const std::string& action) {
    std::cout << "[GUI] Executing menu action: " << action << std::endl;
    
    if (action == "launch_calculator") {
        launch_application("/bin/calculator");
    } else if (action == "launch_editor") {
        launch_application("/bin/editor");
    } else if (action == "launch_filemanager") {
        launch_application("/bin/filemanager");
    } else if (action == "task_manager") {
        show_task_manager();
    } else if (action == "settings") {
        show_system_settings();
    } else if (action == "about") {
        show_about_dialog();
    } else if (action == "shutdown") {
        gui_running = false;
    }
}

void GUIManager::launch_application(const std::string& executable_path) {
    std::cout << "[GUI] Launching application: " << executable_path << std::endl;
    
    // Create appropriate window based on application
    std::shared_ptr<Window> app_window = nullptr;
    
    if (executable_path.find("calculator") != std::string::npos) {
        app_window = create_window("Calculator", 200, 200, 300, 400);
        app_window->set_paint_callback([](PixelBuffer* buffer) {
            // Simple calculator interface
            buffer->fill_rect(Rect(10, 40, 280, 50), Color(255, 255, 255));
            buffer->draw_text(20, 60, "0", Color(0, 0, 0));
            
            // Draw buttons
            const char* buttons[] = {"7", "8", "9", "/", "4", "5", "6", "*", 
                                   "1", "2", "3", "-", "0", ".", "=", "+"};
            for (int i = 0; i < 16; i++) {
                int row = i / 4;
                int col = i % 4;
                int x = 10 + col * 70;
                int y = 100 + row * 50;
                
                buffer->fill_rect(Rect(x, y, 65, 45), Color(220, 220, 220));
                buffer->draw_text(x + 25, y + 20, buttons[i], Color(0, 0, 0));
            }
        });
    } else if (executable_path.find("editor") != std::string::npos) {
        app_window = create_window("Text Editor", 300, 150, 600, 500);
        app_window->set_paint_callback([](PixelBuffer* buffer) {
            // Simple text editor interface
            buffer->fill_rect(Rect(10, 40, 580, 450), Color(255, 255, 255));
            buffer->draw_text(20, 60, "Type your text here...", Color(128, 128, 128));
        });
    } else if (executable_path.find("filemanager") != std::string::npos) {
        app_window = create_window("File Manager", 250, 100, 700, 600);
        app_window->set_paint_callback([](PixelBuffer* buffer) {
            // Simple file manager interface
            buffer->fill_rect(Rect(10, 40, 680, 50), Color(240, 240, 240));
            buffer->draw_text(20, 60, "Path: /home/user", Color(0, 0, 0));
            
            // File list area
            buffer->fill_rect(Rect(10, 100, 680, 490), Color(255, 255, 255));
            
            // Sample file entries
            const char* files[] = {"Documents", "Pictures", "readme.txt", "note.txt"};
            for (int i = 0; i < 4; i++) {
                int y = 120 + i * 25;
                buffer->draw_text(20, y, files[i], Color(0, 0, 0));
            }
        });
    }
    
    if (app_window) {
        app_window->show();
        focus_window(app_window);
    }
}

void GUIManager::handle_desktop_click(int x, int y) {
    // Check if clicking on desktop icons
    for (auto& icon : desktop_icons) {
        if (x >= icon.x && x < icon.x + icon.width &&
            y >= icon.y && y < icon.y + icon.height) {
            
            // Deselect all icons
            for (auto& other_icon : desktop_icons) {
                other_icon.selected = false;
            }
            
            // Select clicked icon
            icon.selected = true;
            return;
        }
    }
    
    // Clicking on empty desktop - deselect all icons
    for (auto& icon : desktop_icons) {
        icon.selected = false;
    }
    
    // Close start menu if open
    if (start_menu_open) {
        start_menu_open = false;
    }
}

void GUIManager::handle_icon_click(int x, int y) {
    for (const auto& icon : desktop_icons) {
        if (x >= icon.x && x < icon.x + icon.width &&
            y >= icon.y && y < icon.y + icon.height) {
            launch_application(icon.executable_path);
            break;
        }
    }
}

std::shared_ptr<Window> GUIManager::create_window(const std::string& title, int x, int y, int width, int height, int style) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    
    auto window = std::make_shared<Window>(title, x, y, width, height, style);
    windows.push_back(window);
    
    // Set up window event callbacks
    window->set_window_event_callback([this](const WindowEvent& event) {
        if (event.type == WINDOW_CLOSED) {
            close_window(event.window_id);
        }
    });
    
    std::cout << "[GUI] Created window: " << title << std::endl;
    return window;
}

void GUIManager::destroy_window(std::shared_ptr<Window> window) {
    std::lock_guard<std::mutex> lock(gui_mutex);
    
    if (window == focused_window) {
        focused_window.reset();
    }
    if (window == dragging_window) {
        dragging_window.reset();
        mouse_dragging = false;
    }
    
    windows.erase(std::remove(windows.begin(), windows.end(), window), windows.end());
}

void GUIManager::close_window(int window_id) {
    auto window = get_window(window_id);
    if (window) {
        destroy_window(window);
    }
}

std::shared_ptr<Window> GUIManager::get_window(int window_id) {
    for (auto& window : windows) {
        if (window->get_id() == window_id) {
            return window;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Window>> GUIManager::get_all_windows() {
    return windows;
}

void GUIManager::add_desktop_icon(const std::string& name, const std::string& executable, int x, int y) {
    desktop_icons.emplace_back(name, executable, x, y);
}

void GUIManager::remove_desktop_icon(const std::string& name) {
    desktop_icons.erase(
        std::remove_if(desktop_icons.begin(), desktop_icons.end(),
                      [&name](const DesktopIcon& icon) { return icon.name == name; }),
        desktop_icons.end());
}

void GUIManager::create_sample_windows() {
    // Create a welcome window
    auto welcome_window = create_window("Welcome to MyOS", 100, 100, 500, 350);
    welcome_window->set_paint_callback([](PixelBuffer* buffer) {
        buffer->draw_text(20, 60, "Welcome to MyOS!", Color(0, 0, 0));
        buffer->draw_text(20, 90, "This is a demonstration operating system", Color(0, 0, 0));
        buffer->draw_text(20, 120, "with a graphical user interface.", Color(0, 0, 0));
        buffer->draw_text(20, 160, "Features:", Color(0, 0, 0));
        buffer->draw_text(30, 190, "- Window management", Color(0, 0, 0));
        buffer->draw_text(30, 210, "- File system", Color(0, 0, 0));
        buffer->draw_text(30, 230, "- Process management", Color(0, 0, 0));
        buffer->draw_text(30, 250, "- Device drivers", Color(0, 0, 0));
        buffer->draw_text(20, 290, "Click on desktop icons to launch applications!", Color(0, 0, 0));
    });
    welcome_window->show();
    focus_window(welcome_window);
}

void GUIManager::show_task_manager() {
    auto task_window = create_window("Task Manager", 200, 150, 600, 400);
    task_window->set_paint_callback([this](PixelBuffer* buffer) {
        buffer->draw_text(20, 60, "Running Processes:", Color(0, 0, 0));
        
        int y = 90;
        for (const auto& window : windows) {
            if (window->is_visible()) {
                std::string info = "Window: " + window->get_title() + " (ID: " + std::to_string(window->get_id()) + ")";
                buffer->draw_text(30, y, info, Color(0, 0, 0));
                y += 20;
            }
        }
    });
    task_window->show();
    focus_window(task_window);
}

void GUIManager::show_system_settings() {
    show_message_box("Settings", "System settings not implemented yet");
}

void GUIManager::show_about_dialog() {
    auto about_window = create_window("About MyOS", 300, 250, 400, 300);
    about_window->set_paint_callback([](PixelBuffer* buffer) {
        buffer->draw_text(20, 60, "MyOS v1.0", Color(0, 0, 0));
        buffer->draw_text(20, 90, "A demonstration operating system", Color(0, 0, 0));
        buffer->draw_text(20, 120, "Built with C++ and assembly", Color(0, 0, 0));
        buffer->draw_text(20, 160, "Features:", Color(0, 0, 0));
        buffer->draw_text(30, 180, "- Kernel with system calls", Color(0, 0, 0));
        buffer->draw_text(30, 200, "- Memory management", Color(0, 0, 0));
        buffer->draw_text(30, 220, "- Process scheduling", Color(0, 0, 0));
        buffer->draw_text(30, 240, "- GUI with window manager", Color(0, 0, 0));
    });
    about_window->show();
    focus_window(about_window);
}

void GUIManager::show_message_box(const std::string& title, const std::string& message) {
    auto msg_window = create_window(title, 300, 300, 350, 150, WINDOW_STYLE_DIALOG);
    msg_window->set_paint_callback([message](PixelBuffer* buffer) {
        buffer->draw_text(20, 60, message, Color(0, 0, 0));
        
        // OK button
        buffer->fill_rect(Rect(275, 100, 60, 30), Color(220, 220, 220));
        buffer->draw_text(295, 115, "OK", Color(0, 0, 0));
    });
    msg_window->show();
    focus_window(msg_window);
}

void GUIManager::show_error_dialog(const std::string& title, const std::string& error) {
    show_message_box("Error: " + title, error);
}

void GUIManager::print_gui_state() {
    std::lock_guard<std::mutex> lock(gui_mutex);
    
    std::cout << "[GUI] GUI State:" << std::endl;
    std::cout << "  Windows: " << windows.size() << std::endl;
    std::cout << "  Desktop icons: " << desktop_icons.size() << std::endl;
    std::cout << "  Start menu open: " << (start_menu_open ? "Yes" : "No") << std::endl;
    std::cout << "  Focused window: " << (focused_window ? focused_window->get_title() : "None") << std::endl;
    std::cout << "  Mouse dragging: " << (mouse_dragging ? "Yes" : "No") << std::endl;
}
