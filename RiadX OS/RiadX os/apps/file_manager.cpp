#include "file_manager.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <sstream>
#include <iomanip>

FileManagerApp::FileManagerApp(FileSystem* fs) 
    : filesystem(fs), current_path("/"), selected_file_index(-1), scroll_offset(0),
      show_hidden_files(false), list_view_mode(true), clipboard_cut(false),
      history_index(-1) {
    
    std::cout << "[FILE_MANAGER] File Manager initializing..." << std::endl;
}

FileManagerApp::~FileManagerApp() {
    std::cout << "[FILE_MANAGER] File Manager shutting down" << std::endl;
}

bool FileManagerApp::initialize() {
    // Create main window
    main_window = std::make_shared<Window>("File Manager", 100, 100, 800, 600);
    
    if (!main_window) {
        std::cerr << "[FILE_MANAGER] Failed to create main window" << std::endl;
        return false;
    }
    
    // Set up window properties
    main_window->set_resizable(true);
    main_window->set_background_color(Color(240, 240, 240));
    
    // Set up paint callback
    main_window->set_paint_callback([this](PixelBuffer* buffer) {
        // Clear background
        buffer->clear(Color(240, 240, 240));
        
        // Draw UI components
        draw_toolbar(buffer);
        draw_sidebar(buffer);
        draw_file_list(buffer);
        draw_status_bar(buffer);
    });
    
    // Set up event callbacks
    main_window->set_mouse_event_callback([this](const MouseEvent& event) {
        if (event.type == MOUSE_BUTTON_PRESSED && event.button == MOUSE_BUTTON_LEFT) {
            const Rect& bounds = main_window->get_bounds();
            
            // Check toolbar clicks
            if (event.y < TOOLBAR_HEIGHT) {
                handle_toolbar_click(event.x, event.y);
            }
            // Check file list clicks
            else if (event.x > SIDEBAR_WIDTH && event.y > TOOLBAR_HEIGHT && 
                     event.y < bounds.height - STATUS_BAR_HEIGHT) {
                handle_file_list_click(event.x - SIDEBAR_WIDTH, event.y - TOOLBAR_HEIGHT);
            }
        } else if (event.type == MOUSE_BUTTON_PRESSED && event.button == MOUSE_BUTTON_RIGHT) {
            handle_right_click(event.x, event.y);
        }
    });
    
    main_window->set_key_event_callback([this](const KeyEvent& event) {
        if (event.type == KEY_PRESSED) {
            switch (event.keycode) {
                case KEY_F5:
                    refresh_file_list();
                    break;
                case KEY_DELETE:
                    delete_selected_file();
                    break;
                case KEY_F2:
                    rename_selected_file();
                    break;
                case KEY_ENTER:
                    if (selected_file_index >= 0 && selected_file_index < static_cast<int>(current_files.size())) {
                        handle_file_double_click(selected_file_index);
                    }
                    break;
                case KEY_BACKSPACE:
                    go_up();
                    break;
                case KEY_UP:
                    if (selected_file_index > 0) {
                        selected_file_index--;
                        main_window->invalidate();
                    }
                    break;
                case KEY_DOWN:
                    if (selected_file_index < static_cast<int>(current_files.size()) - 1) {
                        selected_file_index++;
                        main_window->invalidate();
                    }
                    break;
                default:
                    if (event.ctrl_pressed) {
                        switch (event.keycode) {
                            case KEY_C:
                                copy_selected_file();
                                break;
                            case KEY_X:
                                cut_selected_file();
                                break;
                            case KEY_V:
                                paste_file();
                                break;
                        }
                    }
                    break;
            }
        }
    });
    
    // Initialize with current directory
    navigate_to("/");
    
    std::cout << "[FILE_MANAGER] File Manager initialized" << std::endl;
    return true;
}

void FileManagerApp::show() {
    if (main_window) {
        main_window->show();
        refresh_file_list();
    }
}

void FileManagerApp::hide() {
    if (main_window) {
        main_window->hide();
    }
}

void FileManagerApp::refresh_file_list() {
    if (!filesystem) return;
    
    current_files = filesystem->list_directory(current_path);
    
    // Filter hidden files if needed
    if (!show_hidden_files) {
        current_files.erase(
            std::remove_if(current_files.begin(), current_files.end(),
                          [](const DirectoryEntry& entry) {
                              return entry.name[0] == '.';
                          }),
            current_files.end());
    }
    
    // Sort files: directories first, then by name
    std::sort(current_files.begin(), current_files.end(),
              [](const DirectoryEntry& a, const DirectoryEntry& b) {
                  if (a.attributes.type != b.attributes.type) {
                      return a.attributes.type == FILE_TYPE_DIRECTORY;
                  }
                  return a.name < b.name;
              });
    
    selected_file_index = -1;
    scroll_offset = 0;
    
    if (main_window) {
        main_window->invalidate();
    }
    
    std::cout << "[FILE_MANAGER] Refreshed file list: " << current_files.size() << " items" << std::endl;
}

void FileManagerApp::navigate_to(const std::string& path) {
    if (!filesystem || !filesystem->is_directory(path)) {
        show_error_message("Cannot navigate to: " + path);
        return;
    }
    
    // Add to history
    if (history_index >= 0 && history_index < static_cast<int>(navigation_history.size()) - 1) {
        // Remove forward history if we're navigating from middle
        navigation_history.erase(navigation_history.begin() + history_index + 1, navigation_history.end());
    }
    
    navigation_history.push_back(current_path);
    history_index = navigation_history.size() - 1;
    
    current_path = path;
    refresh_file_list();
    
    std::cout << "[FILE_MANAGER] Navigated to: " << current_path << std::endl;
}

void FileManagerApp::go_back() {
    if (history_index > 0) {
        history_index--;
        current_path = navigation_history[history_index];
        refresh_file_list();
    }
}

void FileManagerApp::go_forward() {
    if (history_index < static_cast<int>(navigation_history.size()) - 1) {
        history_index++;
        current_path = navigation_history[history_index];
        refresh_file_list();
    }
}

void FileManagerApp::go_up() {
    if (current_path != "/") {
        size_t last_slash = current_path.find_last_of('/');
        if (last_slash == 0) {
            navigate_to("/");
        } else {
            navigate_to(current_path.substr(0, last_slash));
        }
    }
}

void FileManagerApp::draw_toolbar(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    
    // Draw toolbar background
    buffer->fill_rect(Rect(0, 0, bounds.width, TOOLBAR_HEIGHT), Color(220, 220, 220));
    buffer->draw_line(0, TOOLBAR_HEIGHT - 1, bounds.width, TOOLBAR_HEIGHT - 1, Color(128, 128, 128));
    
    // Draw navigation buttons
    int button_x = 5;
    
    // Back button
    buffer->fill_rect(Rect(button_x, 5, 30, 30), Color(200, 200, 200));
    buffer->draw_text(button_x + 8, 15, "<", Color(0, 0, 0));
    button_x += 35;
    
    // Forward button
    buffer->fill_rect(Rect(button_x, 5, 30, 30), Color(200, 200, 200));
    buffer->draw_text(button_x + 8, 15, ">", Color(0, 0, 0));
    button_x += 35;
    
    // Up button
    buffer->fill_rect(Rect(button_x, 5, 30, 30), Color(200, 200, 200));
    buffer->draw_text(button_x + 8, 15, "^", Color(0, 0, 0));
    button_x += 40;
    
    // Address bar
    buffer->fill_rect(Rect(button_x, 8, bounds.width - button_x - 100, 24), Color(255, 255, 255));
    buffer->draw_text(button_x + 5, 18, current_path, Color(0, 0, 0));
    
    // Refresh button
    buffer->fill_rect(Rect(bounds.width - 80, 5, 30, 30), Color(200, 200, 200));
    buffer->draw_text(bounds.width - 72, 15, "R", Color(0, 0, 0));
    
    // View mode button
    buffer->fill_rect(Rect(bounds.width - 45, 5, 30, 30), Color(200, 200, 200));
    buffer->draw_text(bounds.width - 37, 15, list_view_mode ? "L" : "I", Color(0, 0, 0));
}

void FileManagerApp::draw_sidebar(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    
    // Draw sidebar background
    buffer->fill_rect(Rect(0, TOOLBAR_HEIGHT, SIDEBAR_WIDTH, bounds.height - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT), 
                     Color(230, 230, 230));
    buffer->draw_line(SIDEBAR_WIDTH - 1, TOOLBAR_HEIGHT, SIDEBAR_WIDTH - 1, bounds.height - STATUS_BAR_HEIGHT, 
                     Color(128, 128, 128));
    
    // Draw quick access items
    int y = TOOLBAR_HEIGHT + 10;
    
    buffer->draw_text(10, y, "Quick Access", Color(0, 0, 0));
    y += 25;
    
    buffer->draw_text(15, y, "Home", Color(0, 0, 0));
    y += 20;
    buffer->draw_text(15, y, "Documents", Color(0, 0, 0));
    y += 20;
    buffer->draw_text(15, y, "Pictures", Color(0, 0, 0));
    y += 20;
    buffer->draw_text(15, y, "Desktop", Color(0, 0, 0));
    y += 30;
    
    buffer->draw_text(10, y, "System", Color(0, 0, 0));
    y += 25;
    buffer->draw_text(15, y, "Root (/)", Color(0, 0, 0));
    y += 20;
    buffer->draw_text(15, y, "Bin", Color(0, 0, 0));
    y += 20;
    buffer->draw_text(15, y, "Etc", Color(0, 0, 0));
}

void FileManagerApp::draw_file_list(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    
    int list_x = SIDEBAR_WIDTH;
    int list_y = TOOLBAR_HEIGHT;
    int list_width = bounds.width - SIDEBAR_WIDTH;
    int list_height = bounds.height - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT;
    
    // Draw file list background
    buffer->fill_rect(Rect(list_x, list_y, list_width, list_height), Color(255, 255, 255));
    
    // Draw column headers
    if (list_view_mode) {
        buffer->fill_rect(Rect(list_x, list_y, list_width, 25), Color(240, 240, 240));
        buffer->draw_line(list_x, list_y + 24, list_x + list_width, list_y + 24, Color(128, 128, 128));
        
        buffer->draw_text(list_x + 5, list_y + 8, "Name", Color(0, 0, 0));
        buffer->draw_text(list_x + 300, list_y + 8, "Size", Color(0, 0, 0));
        buffer->draw_text(list_x + 400, list_y + 8, "Type", Color(0, 0, 0));
        buffer->draw_text(list_x + 500, list_y + 8, "Modified", Color(0, 0, 0));
        
        list_y += 25;
        list_height -= 25;
    }
    
    // Draw files
    int item_y = list_y;
    int visible_items = list_height / FILE_ITEM_HEIGHT;
    
    for (int i = scroll_offset; i < static_cast<int>(current_files.size()) && i < scroll_offset + visible_items; i++) {
        const DirectoryEntry& entry = current_files[i];
        
        // Highlight selected item
        if (i == selected_file_index) {
            buffer->fill_rect(Rect(list_x, item_y, list_width, FILE_ITEM_HEIGHT), Color(200, 220, 255));
        }
        
        // Draw file icon
        std::string icon = get_file_icon(entry);
        buffer->draw_text(list_x + 5, item_y + 5, icon, Color(0, 0, 0));
        
        // Draw file name
        buffer->draw_text(list_x + 25, item_y + 5, entry.name, Color(0, 0, 0));
        
        if (list_view_mode) {
            // Draw file size
            if (entry.attributes.type != FILE_TYPE_DIRECTORY) {
                std::string size_str = format_file_size(entry.attributes.size);
                buffer->draw_text(list_x + 300, item_y + 5, size_str, Color(0, 0, 0));
            }
            
            // Draw file type
            std::string type = (entry.attributes.type == FILE_TYPE_DIRECTORY) ? "Folder" : "File";
            buffer->draw_text(list_x + 400, item_y + 5, type, Color(0, 0, 0));
            
            // Draw modification date
            std::string date_str = format_file_date(entry.attributes.modification_time);
            buffer->draw_text(list_x + 500, item_y + 5, date_str, Color(0, 0, 0));
        }
        
        item_y += FILE_ITEM_HEIGHT;
    }
    
    // Draw scrollbar if needed
    if (static_cast<int>(current_files.size()) > visible_items) {
        int scrollbar_x = list_x + list_width - 15;
        int scrollbar_height = list_height - 20;
        int thumb_height = std::max(20, scrollbar_height * visible_items / static_cast<int>(current_files.size()));
        int thumb_y = list_y + 10 + (scrollbar_height - thumb_height) * scroll_offset / 
                      (static_cast<int>(current_files.size()) - visible_items);
        
        // Scrollbar track
        buffer->fill_rect(Rect(scrollbar_x, list_y + 10, 10, scrollbar_height), Color(240, 240, 240));
        
        // Scrollbar thumb
        buffer->fill_rect(Rect(scrollbar_x, thumb_y, 10, thumb_height), Color(180, 180, 180));
    }
}

void FileManagerApp::draw_status_bar(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    int status_y = bounds.height - STATUS_BAR_HEIGHT;
    
    // Draw status bar background
    buffer->fill_rect(Rect(0, status_y, bounds.width, STATUS_BAR_HEIGHT), Color(220, 220, 220));
    buffer->draw_line(0, status_y, bounds.width, status_y, Color(128, 128, 128));
    
    // Draw status text
    std::ostringstream status;
    status << current_files.size() << " items";
    if (selected_file_index >= 0) {
        status << " | " << current_files[selected_file_index].name << " selected";
    }
    
    buffer->draw_text(10, status_y + 8, status.str(), Color(0, 0, 0));
    
    // Draw disk space info
    if (filesystem) {
        size_t free_space = filesystem->get_free_space();
        size_t total_space = filesystem->get_total_space();
        
        std::ostringstream disk_info;
        disk_info << format_file_size(free_space) << " free of " << format_file_size(total_space);
        
        buffer->draw_text(bounds.width - 200, status_y + 8, disk_info.str(), Color(0, 0, 0));
    }
}

void FileManagerApp::handle_toolbar_click(int x, int y) {
    if (y >= 5 && y <= 35) {
        if (x >= 5 && x <= 35) {
            // Back button
            go_back();
        } else if (x >= 40 && x <= 70) {
            // Forward button
            go_forward();
        } else if (x >= 75 && x <= 105) {
            // Up button
            go_up();
        } else if (x >= main_window->get_bounds().width - 80 && x <= main_window->get_bounds().width - 50) {
            // Refresh button
            refresh_file_list();
        } else if (x >= main_window->get_bounds().width - 45 && x <= main_window->get_bounds().width - 15) {
            // View mode button
            list_view_mode = !list_view_mode;
            main_window->invalidate();
        }
    }
}

void FileManagerApp::handle_file_list_click(int x, int y) {
    int header_offset = list_view_mode ? 25 : 0;
    int item_index = (y - header_offset) / FILE_ITEM_HEIGHT + scroll_offset;
    
    if (item_index >= 0 && item_index < static_cast<int>(current_files.size())) {
        selected_file_index = item_index;
        main_window->invalidate();
    } else {
        selected_file_index = -1;
        main_window->invalidate();
    }
}

void FileManagerApp::handle_file_double_click(int file_index) {
    if (file_index < 0 || file_index >= static_cast<int>(current_files.size())) return;
    
    const DirectoryEntry& entry = current_files[file_index];
    
    if (entry.attributes.type == FILE_TYPE_DIRECTORY) {
        navigate_to(entry.full_path);
    } else {
        open_file(entry.full_path);
    }
}

void FileManagerApp::handle_right_click(int x, int y) {
    // Show context menu
    // For now, just print debug info
    std::cout << "[FILE_MANAGER] Right click at (" << x << ", " << y << ")" << std::endl;
}

std::string FileManagerApp::get_file_icon(const DirectoryEntry& entry) {
    if (entry.attributes.type == FILE_TYPE_DIRECTORY) {
        return "[D]";
    } else if (is_executable_file(entry.name)) {
        return "[E]";
    } else if (is_image_file(entry.name)) {
        return "[I]";
    } else if (is_text_file(entry.name)) {
        return "[T]";
    } else {
        return "[F]";
    }
}

std::string FileManagerApp::format_file_size(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double size_double = static_cast<double>(size);
    
    while (size_double >= 1024 && unit_index < 3) {
        size_double /= 1024;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size_double << " " << units[unit_index];
    return oss.str();
}

std::string FileManagerApp::format_file_date(uint64_t timestamp) {
    // Simple date formatting
    time_t time = static_cast<time_t>(timestamp);
    struct tm* tm_info = localtime(&time);
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << tm_info->tm_mon + 1 << "/"
        << std::setw(2) << tm_info->tm_mday << "/"
        << (tm_info->tm_year + 1900) << " "
        << std::setw(2) << tm_info->tm_hour << ":"
        << std::setw(2) << tm_info->tm_min;
    
    return oss.str();
}

bool FileManagerApp::is_image_file(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" || ext == "bmp";
}

bool FileManagerApp::is_text_file(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "txt" || ext == "cpp" || ext == "h" || ext == "c" || ext == "py" || ext == "js" || ext == "html";
}

bool FileManagerApp::is_executable_file(const std::string& filename) {
    return filename.find_last_of('.') == std::string::npos; // Simple check for extensionless files
}

bool FileManagerApp::open_file(const std::string& path) {
    std::cout << "[FILE_MANAGER] Opening file: " << path << std::endl;
    
    if (is_executable_file(path)) {
        return execute_file(path);
    } else if (is_text_file(path)) {
        // Launch text editor
        std::cout << "[FILE_MANAGER] Opening text file in editor: " << path << std::endl;
        return true;
    } else if (is_image_file(path)) {
        // Launch image viewer
        std::cout << "[FILE_MANAGER] Opening image file: " << path << std::endl;
        return true;
    } else {
        show_error_message("Cannot open file: " + path);
        return false;
    }
}

bool FileManagerApp::execute_file(const std::string& path) {
    std::cout << "[FILE_MANAGER] Executing file: " << path << std::endl;
    // In a real OS, this would launch the executable
    return true;
}

void FileManagerApp::show_error_message(const std::string& message) {
    std::cerr << "[FILE_MANAGER] Error: " << message << std::endl;
    // In a real implementation, this would show a dialog box
}

void FileManagerApp::delete_selected_file() {
    if (selected_file_index >= 0 && selected_file_index < static_cast<int>(current_files.size())) {
        const DirectoryEntry& entry = current_files[selected_file_index];
        
        if (entry.attributes.type == FILE_TYPE_DIRECTORY) {
            if (filesystem->delete_directory(entry.full_path)) {
                refresh_file_list();
            } else {
                show_error_message("Cannot delete directory: " + entry.name);
            }
        } else {
            if (filesystem->delete_file(entry.full_path)) {
                refresh_file_list();
            } else {
                show_error_message("Cannot delete file: " + entry.name);
            }
        }
    }
}

void FileManagerApp::copy_selected_file() {
    if (selected_file_index >= 0 && selected_file_index < static_cast<int>(current_files.size())) {
        clipboard_file = current_files[selected_file_index].full_path;
        clipboard_cut = false;
        std::cout << "[FILE_MANAGER] Copied: " << clipboard_file << std::endl;
    }
}

void FileManagerApp::cut_selected_file() {
    if (selected_file_index >= 0 && selected_file_index < static_cast<int>(current_files.size())) {
        clipboard_file = current_files[selected_file_index].full_path;
        clipboard_cut = true;
        std::cout << "[FILE_MANAGER] Cut: " << clipboard_file << std::endl;
    }
}

void FileManagerApp::paste_file() {
    if (clipboard_file.empty()) return;
    
    std::string filename = clipboard_file.substr(clipboard_file.find_last_of('/') + 1);
    std::string dest_path = current_path + "/" + filename;
    
    if (clipboard_cut) {
        if (filesystem->move_file(clipboard_file, dest_path)) {
            clipboard_file.clear();
            refresh_file_list();
        } else {
            show_error_message("Cannot move file");
        }
    } else {
        if (filesystem->copy_file(clipboard_file, dest_path)) {
            refresh_file_list();
        } else {
            show_error_message("Cannot copy file");
        }
    }
}

void FileManagerApp::open_path(const std::string& path) {
    navigate_to(path);
}

void FileManagerApp::select_file(const std::string& filename) {
    for (int i = 0; i < static_cast<int>(current_files.size()); i++) {
        if (current_files[i].name == filename) {
            selected_file_index = i;
            main_window->invalidate();
            break;
        }
    }
}

void FileManagerApp::set_show_hidden_files(bool show) {
    show_hidden_files = show;
    refresh_file_list();
}

void FileManagerApp::set_view_mode(bool list_mode) {
    list_view_mode = list_mode;
    main_window->invalidate();
}
