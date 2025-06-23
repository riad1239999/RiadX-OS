#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <vector>
#include <string>
#include <memory>
#include "../gui/window.h"
#include "../drivers/filesystem.h"

class TextEditorApp {
private:
    std::shared_ptr<Window> main_window;
    FileSystem* filesystem;
    
    // Document state
    std::vector<std::string> lines;
    std::string current_file_path;
    bool is_modified;
    bool is_read_only;
    
    // Cursor and selection
    int cursor_line;
    int cursor_column;
    int selection_start_line;
    int selection_start_column;
    int selection_end_line;
    int selection_end_column;
    bool has_selection;
    
    // View state
    int scroll_line;
    int scroll_column;
    int visible_lines;
    int visible_columns;
    
    // UI layout
    static const int MENU_HEIGHT = 25;
    static const int TOOLBAR_HEIGHT = 30;
    static const int STATUS_BAR_HEIGHT = 25;
    static const int LINE_HEIGHT = 16;
    static const int CHAR_WIDTH = 8;
    
    // Editor settings
    bool show_line_numbers;
    bool word_wrap;
    bool syntax_highlighting;
    int tab_size;
    
    // Clipboard
    std::string clipboard_text;
    
    // Find/Replace
    std::string find_text;
    std::string replace_text_content;
    bool find_case_sensitive;
    bool find_whole_word;
    int find_current_match;
    
    // Undo/Redo
    struct EditorAction {
        enum Type { INSERT, DELETE, REPLACE } type;
        int line, column;
        std::string text;
        std::string old_text;
    };
    std::vector<EditorAction> undo_stack;
    std::vector<EditorAction> redo_stack;
    int max_undo_levels;
    
    // File operations
    bool load_file(const std::string& file_path);
    bool save_file();
    bool save_file_as(const std::string& file_path);
    void new_document();
    
    // Text operations
    void insert_text(const std::string& text);
    void delete_text(int start_line, int start_col, int end_line, int end_col);
    void replace_text(int start_line, int start_col, int end_line, int end_col, const std::string& new_text);
    
    // Selection operations
    void select_all();
    void clear_selection();
    std::string get_selected_text();
    void delete_selection();
    
    // Clipboard operations
    void copy_text();
    void cut_text();
    void paste_text();
    
    // Cursor movement
    void move_cursor(int new_line, int new_column, bool extend_selection = false);
    void move_cursor_up(bool extend_selection = false);
    void move_cursor_down(bool extend_selection = false);
    void move_cursor_left(bool extend_selection = false);
    void move_cursor_right(bool extend_selection = false);
    void move_cursor_to_line_start(bool extend_selection = false);
    void move_cursor_to_line_end(bool extend_selection = false);
    void move_cursor_to_document_start(bool extend_selection = false);
    void move_cursor_to_document_end(bool extend_selection = false);
    void move_cursor_page_up(bool extend_selection = false);
    void move_cursor_page_down(bool extend_selection = false);
    
    // Find operations
    int find_next(const std::string& text, int start_line, int start_col);
    int find_previous(const std::string& text, int start_line, int start_col);
    void replace_current_match();
    void replace_all();
    
    // Undo/Redo operations
    void add_undo_action(const EditorAction& action);
    void perform_undo();
    void perform_redo();
    void clear_undo_history();
    
    // UI rendering
    void draw_menu_bar(PixelBuffer* buffer);
    void draw_toolbar(PixelBuffer* buffer);
    void draw_text_area(PixelBuffer* buffer);
    void draw_status_bar(PixelBuffer* buffer);
    void draw_line_numbers(PixelBuffer* buffer);
    void draw_cursor(PixelBuffer* buffer);
    void draw_selection(PixelBuffer* buffer);
    void draw_scrollbars(PixelBuffer* buffer);
    
    // Event handling
    void handle_menu_click(int x, int y);
    void handle_toolbar_click(int x, int y);
    void handle_text_area_click(int x, int y);
    void handle_text_input(char c);
    void handle_key_press(KeyCode key, bool shift, bool ctrl, bool alt);
    
    // Utility functions
    void ensure_cursor_visible();
    void update_scroll_bars();
    int get_line_number_width();
    int get_text_area_x();
    int get_text_area_width();
    std::string get_file_extension(const std::string& filename);
    Color get_syntax_color(const std::string& word, const std::string& extension);
    
    // Dialog functions
    void show_about_dialog();
    void show_find_dialog();
    void show_replace_dialog();
    void show_goto_line_dialog();
    bool show_save_changes_dialog();
    
    // Settings
    void apply_settings();
    void load_settings();
    void save_settings();

public:
    TextEditorApp(FileSystem* fs);
    ~TextEditorApp();
    
    bool initialize();
    void show();
    void hide();
    
    // File operations for external access
    void load_content(const std::string& content);
    void set_current_filename(const std::string& filename);
    
    std::shared_ptr<Window> get_window() const { return main_window; }
    
    // Public interface
    bool open_file(const std::string& file_path);
    bool create_new_file();
    bool save_current_file();
    bool close_current_file();
    
    // Document properties
    bool is_document_modified() const { return is_modified; }
    std::string get_current_file() const { return current_file_path; }
    
    // Editor settings
    void set_font_size(int size);
    void set_tab_size(int size);
    void set_word_wrap(bool enabled);
    void set_line_numbers(bool show);
    void set_syntax_highlighting(bool enabled);
};

#endif
