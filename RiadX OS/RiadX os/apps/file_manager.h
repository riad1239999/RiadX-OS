#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <vector>
#include <string>
#include <memory>
#include "../gui/window.h"
#include "../drivers/filesystem.h"

class FileManagerApp {
private:
    std::shared_ptr<Window> main_window;
    FileSystem* filesystem;
    
    // Current state
    std::string current_path;
    std::vector<DirectoryEntry> current_files;
    int selected_file_index;
    int scroll_offset;
    
    // UI layout
    static const int TOOLBAR_HEIGHT = 40;
    static const int STATUS_BAR_HEIGHT = 25;
    static const int FILE_ITEM_HEIGHT = 20;
    static const int SIDEBAR_WIDTH = 150;
    
    // UI state
    bool show_hidden_files;
    bool list_view_mode;
    std::string clipboard_file;
    bool clipboard_cut;
    
    // Navigation history
    std::vector<std::string> navigation_history;
    int history_index;
    
    // File operations
    void refresh_file_list();
    void navigate_to(const std::string& path);
    void go_back();
    void go_forward();
    void go_up();
    void create_new_folder();
    void delete_selected_file();
    void rename_selected_file();
    void copy_selected_file();
    void cut_selected_file();
    void paste_file();
    
    // UI rendering
    void draw_toolbar(PixelBuffer* buffer);
    void draw_sidebar(PixelBuffer* buffer);
    void draw_file_list(PixelBuffer* buffer);
    void draw_status_bar(PixelBuffer* buffer);
    void draw_context_menu(PixelBuffer* buffer, int x, int y);
    
    // Event handling
    void handle_toolbar_click(int x, int y);
    void handle_file_list_click(int x, int y);
    void handle_file_double_click(int file_index);
    void handle_right_click(int x, int y);
    
    // File operations helpers
    std::string get_file_icon(const DirectoryEntry& entry);
    std::string format_file_size(size_t size);
    std::string format_file_date(uint64_t timestamp);
    bool is_image_file(const std::string& filename);
    bool is_text_file(const std::string& filename);
    bool is_executable_file(const std::string& filename);
    
    // Dialog boxes
    void show_properties_dialog(const DirectoryEntry& entry);
    void show_rename_dialog();
    void show_new_folder_dialog();
    void show_error_message(const std::string& message);

public:
    FileManagerApp(FileSystem* fs);
    ~FileManagerApp();
    
    // Set callback for opening files
    void set_file_open_callback(std::function<void(const std::string&, const std::string&)> callback);
    
    bool initialize();
    void show();
    void hide();
    
    std::shared_ptr<Window> get_window() const { return main_window; }
    
    // Public interface
    void open_path(const std::string& path);
    void select_file(const std::string& filename);
    
    // Settings
    void set_show_hidden_files(bool show);
    void set_view_mode(bool list_mode);
    
    // File operations
    bool open_file(const std::string& path);
    bool execute_file(const std::string& path);
};

#endif
