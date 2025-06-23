#include "text_editor.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cctype>

TextEditorApp::TextEditorApp(FileSystem* fs)
    : filesystem(fs), is_modified(false), is_read_only(false),
      cursor_line(0), cursor_column(0), 
      selection_start_line(0), selection_start_column(0),
      selection_end_line(0), selection_end_column(0), has_selection(false),
      scroll_line(0), scroll_column(0), visible_lines(0), visible_columns(0),
      show_line_numbers(true), word_wrap(false), syntax_highlighting(true), tab_size(4),
      find_case_sensitive(false), find_whole_word(false), find_current_match(-1),
      max_undo_levels(100) {
    
    std::cout << "[TEXT_EDITOR] Text Editor initializing..." << std::endl;
    
    // Initialize with empty document
    lines.push_back("");
}

TextEditorApp::~TextEditorApp() {
    std::cout << "[TEXT_EDITOR] Text Editor shutting down" << std::endl;
}

bool TextEditorApp::initialize() {
    // Create main window
    main_window = std::make_shared<Window>("Text Editor", 150, 150, 800, 600);
    
    if (!main_window) {
        std::cerr << "[TEXT_EDITOR] Failed to create main window" << std::endl;
        return false;
    }
    
    // Set up window properties
    main_window->set_resizable(true);
    main_window->set_background_color(Color(255, 255, 255));
    
    // Calculate visible area
    const Rect& bounds = main_window->get_bounds();
    visible_lines = (bounds.height - MENU_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT) / LINE_HEIGHT;
    visible_columns = (bounds.width - get_line_number_width()) / CHAR_WIDTH;
    
    // Set up paint callback
    main_window->set_paint_callback([this](PixelBuffer* buffer) {
        // Clear background
        buffer->clear(Color(255, 255, 255));
        
        // Draw UI components
        draw_menu_bar(buffer);
        draw_toolbar(buffer);
        draw_text_area(buffer);
        draw_status_bar(buffer);
    });
    
    // Set up event callbacks
    main_window->set_mouse_event_callback([this](const MouseEvent& event) {
        if (event.type == MOUSE_BUTTON_PRESSED && event.button == MOUSE_BUTTON_LEFT) {
            if (event.y < MENU_HEIGHT) {
                handle_menu_click(event.x, event.y);
            } else if (event.y < MENU_HEIGHT + TOOLBAR_HEIGHT) {
                handle_toolbar_click(event.x, event.y - MENU_HEIGHT);
            } else if (event.y < main_window->get_bounds().height - STATUS_BAR_HEIGHT) {
                handle_text_area_click(event.x, event.y - MENU_HEIGHT - TOOLBAR_HEIGHT);
            }
        }
    });
    
    main_window->set_key_event_callback([this](const KeyEvent& event) {
        if (event.type == KEY_PRESSED) {
            if (event.ascii_char >= 32 && event.ascii_char <= 126) {
                handle_text_input(event.ascii_char);
            } else {
                handle_key_press(event.keycode, event.shift_pressed, event.ctrl_pressed, event.alt_pressed);
            }
        }
    });
    
    std::cout << "[TEXT_EDITOR] Text Editor initialized" << std::endl;
    return true;
}

void TextEditorApp::show() {
    if (main_window) {
        main_window->show();
    }
}

void TextEditorApp::hide() {
    if (main_window) {
        main_window->hide();
    }
}

void TextEditorApp::load_content(const std::string& content) {
    // Clear existing content
    text_lines.clear();
    
    // Split content by newlines
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        text_lines.push_back(line);
    }
    
    if (text_lines.empty()) {
        text_lines.push_back(""); // Ensure at least one empty line
    }
    
    // Reset cursor and selection
    cursor_line = 0;
    cursor_column = 0;
    selection_start_line = -1;
    selection_start_column = -1;
    selection_end_line = -1;
    selection_end_column = -1;
    
    // Reset view
    scroll_offset_x = 0;
    scroll_offset_y = 0;
    
    // Mark as not modified initially
    is_modified = false;
    
    // Refresh display
    if (main_window) {
        main_window->invalidate();
    }
    
    std::cout << "[TEXT_EDITOR] Loaded content with " << text_lines.size() << " lines" << std::endl;
}

void TextEditorApp::set_current_filename(const std::string& filename) {
    current_filename = filename;
    
    // Update window title
    if (main_window) {
        std::string title = "Text Editor - " + filename;
        main_window->set_title(title);
    }
    
    std::cout << "[TEXT_EDITOR] Set current filename: " << filename << std::endl;
}

bool TextEditorApp::load_file(const std::string& file_path) {
    if (!filesystem || !filesystem->file_exists(file_path)) {
        std::cerr << "[TEXT_EDITOR] File does not exist: " << file_path << std::endl;
        return false;
    }
    
    std::string content = filesystem->read_file(file_path);
    
    // Split content into lines
    lines.clear();
    std::stringstream ss(content);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) {
        lines.push_back("");
    }
    
    current_file_path = file_path;
    is_modified = false;
    cursor_line = 0;
    cursor_column = 0;
    scroll_line = 0;
    scroll_column = 0;
    clear_selection();
    clear_undo_history();
    
    // Update window title
    std::string title = "Text Editor - " + file_path;
    main_window->set_title(title);
    main_window->invalidate();
    
    std::cout << "[TEXT_EDITOR] Loaded file: " << file_path << " (" << lines.size() << " lines)" << std::endl;
    return true;
}

bool TextEditorApp::save_file() {
    if (current_file_path.empty()) {
        return false; // Should show Save As dialog
    }
    
    return save_file_as(current_file_path);
}

bool TextEditorApp::save_file_as(const std::string& file_path) {
    if (!filesystem) return false;
    
    // Combine lines into content
    std::string content;
    for (size_t i = 0; i < lines.size(); i++) {
        content += lines[i];
        if (i < lines.size() - 1) {
            content += "\n";
        }
    }
    
    if (filesystem->write_file(file_path, content)) {
        current_file_path = file_path;
        is_modified = false;
        
        // Update window title
        std::string title = "Text Editor - " + file_path;
        main_window->set_title(title);
        main_window->invalidate();
        
        std::cout << "[TEXT_EDITOR] Saved file: " << file_path << std::endl;
        return true;
    }
    
    std::cerr << "[TEXT_EDITOR] Failed to save file: " << file_path << std::endl;
    return false;
}

void TextEditorApp::new_document() {
    lines.clear();
    lines.push_back("");
    current_file_path.clear();
    is_modified = false;
    cursor_line = 0;
    cursor_column = 0;
    scroll_line = 0;
    scroll_column = 0;
    clear_selection();
    clear_undo_history();
    
    main_window->set_title("Text Editor - Untitled");
    main_window->invalidate();
    
    std::cout << "[TEXT_EDITOR] New document created" << std::endl;
}

void TextEditorApp::insert_text(const std::string& text) {
    if (has_selection) {
        delete_selection();
    }
    
    EditorAction action;
    action.type = EditorAction::INSERT;
    action.line = cursor_line;
    action.column = cursor_column;
    action.text = text;
    add_undo_action(action);
    
    std::string& current_line = lines[cursor_line];
    current_line.insert(cursor_column, text);
    cursor_column += text.length();
    
    is_modified = true;
    main_window->invalidate();
}

void TextEditorApp::delete_text(int start_line, int start_col, int end_line, int end_col) {
    if (start_line > end_line || (start_line == end_line && start_col >= end_col)) {
        return;
    }
    
    std::string deleted_text;
    
    if (start_line == end_line) {
        // Single line deletion
        std::string& line = lines[start_line];
        deleted_text = line.substr(start_col, end_col - start_col);
        line.erase(start_col, end_col - start_col);
    } else {
        // Multi-line deletion
        for (int i = start_line; i <= end_line; i++) {
            if (i == start_line) {
                deleted_text += lines[i].substr(start_col);
                lines[i].erase(start_col);
            } else if (i == end_line) {
                deleted_text += "\n" + lines[i].substr(0, end_col);
                lines[start_line] += lines[i].substr(end_col);
                lines.erase(lines.begin() + i);
                break;
            } else {
                deleted_text += "\n" + lines[i];
                lines.erase(lines.begin() + i);
                end_line--;
                i--;
            }
        }
    }
    
    EditorAction action;
    action.type = EditorAction::DELETE;
    action.line = start_line;
    action.column = start_col;
    action.text = deleted_text;
    add_undo_action(action);
    
    cursor_line = start_line;
    cursor_column = start_col;
    is_modified = true;
    main_window->invalidate();
}

void TextEditorApp::handle_text_input(char c) {
    if (c == '\t') {
        // Insert tab as spaces
        std::string spaces(tab_size, ' ');
        insert_text(spaces);
    } else if (c == '\n' || c == '\r') {
        // Insert new line
        if (has_selection) {
            delete_selection();
        }
        
        std::string& current_line = lines[cursor_line];
        std::string remaining = current_line.substr(cursor_column);
        current_line.erase(cursor_column);
        
        cursor_line++;
        cursor_column = 0;
        lines.insert(lines.begin() + cursor_line, remaining);
        
        is_modified = true;
        ensure_cursor_visible();
        main_window->invalidate();
    } else {
        // Insert regular character
        std::string text(1, c);
        insert_text(text);
    }
}

void TextEditorApp::handle_key_press(KeyCode key, bool shift, bool ctrl, bool alt) {
    switch (key) {
        case KEY_BACKSPACE:
            if (has_selection) {
                delete_selection();
            } else if (cursor_column > 0) {
                delete_text(cursor_line, cursor_column - 1, cursor_line, cursor_column);
            } else if (cursor_line > 0) {
                int prev_line_length = lines[cursor_line - 1].length();
                lines[cursor_line - 1] += lines[cursor_line];
                lines.erase(lines.begin() + cursor_line);
                cursor_line--;
                cursor_column = prev_line_length;
                is_modified = true;
                main_window->invalidate();
            }
            break;
            
        case KEY_DELETE:
            if (has_selection) {
                delete_selection();
            } else if (cursor_column < static_cast<int>(lines[cursor_line].length())) {
                delete_text(cursor_line, cursor_column, cursor_line, cursor_column + 1);
            } else if (cursor_line < static_cast<int>(lines.size()) - 1) {
                lines[cursor_line] += lines[cursor_line + 1];
                lines.erase(lines.begin() + cursor_line + 1);
                is_modified = true;
                main_window->invalidate();
            }
            break;
            
        case KEY_LEFT:
            move_cursor_left(shift);
            break;
            
        case KEY_RIGHT:
            move_cursor_right(shift);
            break;
            
        case KEY_UP:
            move_cursor_up(shift);
            break;
            
        case KEY_DOWN:
            move_cursor_down(shift);
            break;
            
        case KEY_HOME:
            if (ctrl) {
                move_cursor_to_document_start(shift);
            } else {
                move_cursor_to_line_start(shift);
            }
            break;
            
        case KEY_END:
            if (ctrl) {
                move_cursor_to_document_end(shift);
            } else {
                move_cursor_to_line_end(shift);
            }
            break;
            
        case KEY_PAGEUP:
            move_cursor_page_up(shift);
            break;
            
        case KEY_PAGEDOWN:
            move_cursor_page_down(shift);
            break;
            
        default:
            if (ctrl) {
                switch (key) {
                    case KEY_N:
                        new_document();
                        break;
                    case KEY_S:
                        save_file();
                        break;
                    case KEY_A:
                        select_all();
                        break;
                    case KEY_C:
                        copy_text();
                        break;
                    case KEY_X:
                        cut_text();
                        break;
                    case KEY_V:
                        paste_text();
                        break;
                    case KEY_Z:
                        perform_undo();
                        break;
                    case KEY_Y:
                        perform_redo();
                        break;
                    case KEY_F:
                        show_find_dialog();
                        break;
                }
            }
            break;
    }
    
    ensure_cursor_visible();
}

void TextEditorApp::move_cursor(int new_line, int new_column, bool extend_selection) {
    new_line = std::max(0, std::min(new_line, static_cast<int>(lines.size()) - 1));
    new_column = std::max(0, std::min(new_column, static_cast<int>(lines[new_line].length())));
    
    if (!extend_selection) {
        clear_selection();
    } else if (!has_selection) {
        selection_start_line = cursor_line;
        selection_start_column = cursor_column;
        has_selection = true;
    }
    
    cursor_line = new_line;
    cursor_column = new_column;
    
    if (has_selection) {
        selection_end_line = cursor_line;
        selection_end_column = cursor_column;
    }
    
    main_window->invalidate();
}

void TextEditorApp::move_cursor_left(bool extend_selection) {
    if (cursor_column > 0) {
        move_cursor(cursor_line, cursor_column - 1, extend_selection);
    } else if (cursor_line > 0) {
        move_cursor(cursor_line - 1, lines[cursor_line - 1].length(), extend_selection);
    }
}

void TextEditorApp::move_cursor_right(bool extend_selection) {
    if (cursor_column < static_cast<int>(lines[cursor_line].length())) {
        move_cursor(cursor_line, cursor_column + 1, extend_selection);
    } else if (cursor_line < static_cast<int>(lines.size()) - 1) {
        move_cursor(cursor_line + 1, 0, extend_selection);
    }
}

void TextEditorApp::move_cursor_up(bool extend_selection) {
    if (cursor_line > 0) {
        int new_column = std::min(cursor_column, static_cast<int>(lines[cursor_line - 1].length()));
        move_cursor(cursor_line - 1, new_column, extend_selection);
    }
}

void TextEditorApp::move_cursor_down(bool extend_selection) {
    if (cursor_line < static_cast<int>(lines.size()) - 1) {
        int new_column = std::min(cursor_column, static_cast<int>(lines[cursor_line + 1].length()));
        move_cursor(cursor_line + 1, new_column, extend_selection);
    }
}

void TextEditorApp::draw_text_area(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    int text_area_y = MENU_HEIGHT + TOOLBAR_HEIGHT;
    int text_area_height = bounds.height - MENU_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT;
    
    // Draw text area background
    buffer->fill_rect(Rect(0, text_area_y, bounds.width, text_area_height), Color(255, 255, 255));
    
    // Draw line numbers if enabled
    if (show_line_numbers) {
        draw_line_numbers(buffer);
    }
    
    // Draw text content
    int text_x = get_text_area_x();
    int line_y = text_area_y;
    
    for (int i = scroll_line; i < static_cast<int>(lines.size()) && i < scroll_line + visible_lines; i++) {
        const std::string& line = lines[i];
        int display_column = 0;
        
        // Draw line text
        for (int j = scroll_column; j < static_cast<int>(line.length()) && display_column < visible_columns; j++) {
            char c = line[j];
            Color text_color = Color(0, 0, 0);
            
            // Simple syntax highlighting
            if (syntax_highlighting) {
                if (isdigit(c)) {
                    text_color = Color(0, 0, 255); // Blue for numbers
                } else if (c == '"' || c == '\'') {
                    text_color = Color(0, 128, 0); // Green for strings
                }
            }
            
            buffer->draw_text(text_x + display_column * CHAR_WIDTH, line_y + 2, std::string(1, c), text_color);
            display_column++;
        }
        
        line_y += LINE_HEIGHT;
    }
    
    // Draw selection
    if (has_selection) {
        draw_selection(buffer);
    }
    
    // Draw cursor
    draw_cursor(buffer);
}

void TextEditorApp::draw_cursor(PixelBuffer* buffer) {
    if (cursor_line >= scroll_line && cursor_line < scroll_line + visible_lines &&
        cursor_column >= scroll_column && cursor_column < scroll_column + visible_columns) {
        
        int cursor_x = get_text_area_x() + (cursor_column - scroll_column) * CHAR_WIDTH;
        int cursor_y = MENU_HEIGHT + TOOLBAR_HEIGHT + (cursor_line - scroll_line) * LINE_HEIGHT;
        
        // Draw blinking cursor line
        buffer->draw_line(cursor_x, cursor_y + 2, cursor_x, cursor_y + LINE_HEIGHT - 2, Color(0, 0, 0));
    }
}

void TextEditorApp::draw_menu_bar(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    
    // Draw menu bar background
    buffer->fill_rect(Rect(0, 0, bounds.width, MENU_HEIGHT), Color(240, 240, 240));
    buffer->draw_line(0, MENU_HEIGHT - 1, bounds.width, MENU_HEIGHT - 1, Color(128, 128, 128));
    
    // Draw menu items
    int menu_x = 10;
    buffer->draw_text(menu_x, 8, "File", Color(0, 0, 0));
    menu_x += 40;
    buffer->draw_text(menu_x, 8, "Edit", Color(0, 0, 0));
    menu_x += 40;
    buffer->draw_text(menu_x, 8, "View", Color(0, 0, 0));
    menu_x += 40;
    buffer->draw_text(menu_x, 8, "Help", Color(0, 0, 0));
}

void TextEditorApp::draw_toolbar(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    int toolbar_y = MENU_HEIGHT;
    
    // Draw toolbar background
    buffer->fill_rect(Rect(0, toolbar_y, bounds.width, TOOLBAR_HEIGHT), Color(230, 230, 230));
    buffer->draw_line(0, toolbar_y + TOOLBAR_HEIGHT - 1, bounds.width, toolbar_y + TOOLBAR_HEIGHT - 1, Color(128, 128, 128));
    
    // Draw toolbar buttons
    int button_x = 5;
    
    // New button
    buffer->fill_rect(Rect(button_x, toolbar_y + 3, 24, 24), Color(200, 200, 200));
    buffer->draw_text(button_x + 6, toolbar_y + 13, "N", Color(0, 0, 0));
    button_x += 30;
    
    // Open button
    buffer->fill_rect(Rect(button_x, toolbar_y + 3, 24, 24), Color(200, 200, 200));
    buffer->draw_text(button_x + 6, toolbar_y + 13, "O", Color(0, 0, 0));
    button_x += 30;
    
    // Save button
    buffer->fill_rect(Rect(button_x, toolbar_y + 3, 24, 24), Color(200, 200, 200));
    buffer->draw_text(button_x + 6, toolbar_y + 13, "S", Color(0, 0, 0));
    button_x += 40;
    
    // Copy button
    buffer->fill_rect(Rect(button_x, toolbar_y + 3, 24, 24), Color(200, 200, 200));
    buffer->draw_text(button_x + 6, toolbar_y + 13, "C", Color(0, 0, 0));
    button_x += 30;
    
    // Paste button
    buffer->fill_rect(Rect(button_x, toolbar_y + 3, 24, 24), Color(200, 200, 200));
    buffer->draw_text(button_x + 6, toolbar_y + 13, "P", Color(0, 0, 0));
}

void TextEditorApp::draw_status_bar(PixelBuffer* buffer) {
    const Rect& bounds = main_window->get_bounds();
    int status_y = bounds.height - STATUS_BAR_HEIGHT;
    
    // Draw status bar background
    buffer->fill_rect(Rect(0, status_y, bounds.width, STATUS_BAR_HEIGHT), Color(240, 240, 240));
    buffer->draw_line(0, status_y, bounds.width, status_y, Color(128, 128, 128));
    
    // Draw status information
    std::ostringstream status;
    status << "Line " << (cursor_line + 1) << ", Column " << (cursor_column + 1);
    if (is_modified) {
        status << " [Modified]";
    }
    
    buffer->draw_text(10, status_y + 8, status.str(), Color(0, 0, 0));
    
    // Draw file information
    std::string file_info = current_file_path.empty() ? "Untitled" : current_file_path;
    buffer->draw_text(bounds.width - 200, status_y + 8, file_info, Color(0, 0, 0));
}

void TextEditorApp::draw_line_numbers(PixelBuffer* buffer) {
    int line_num_width = get_line_number_width();
    int text_area_y = MENU_HEIGHT + TOOLBAR_HEIGHT;
    int text_area_height = main_window->get_bounds().height - MENU_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT;
    
    // Draw line number background
    buffer->fill_rect(Rect(0, text_area_y, line_num_width, text_area_height), Color(245, 245, 245));
    buffer->draw_line(line_num_width - 1, text_area_y, line_num_width - 1, text_area_y + text_area_height, Color(200, 200, 200));
    
    // Draw line numbers
    int line_y = text_area_y;
    for (int i = scroll_line; i < static_cast<int>(lines.size()) && i < scroll_line + visible_lines; i++) {
        std::string line_num = std::to_string(i + 1);
        int text_x = line_num_width - 10 - line_num.length() * CHAR_WIDTH;
        buffer->draw_text(text_x, line_y + 2, line_num, Color(100, 100, 100));
        line_y += LINE_HEIGHT;
    }
}

int TextEditorApp::get_line_number_width() {
    if (!show_line_numbers) return 0;
    
    int max_line_digits = std::to_string(lines.size()).length();
    return std::max(40, (max_line_digits + 2) * CHAR_WIDTH);
}

int TextEditorApp::get_text_area_x() {
    return get_line_number_width();
}

void TextEditorApp::ensure_cursor_visible() {
    // Adjust vertical scroll
    if (cursor_line < scroll_line) {
        scroll_line = cursor_line;
    } else if (cursor_line >= scroll_line + visible_lines) {
        scroll_line = cursor_line - visible_lines + 1;
    }
    
    // Adjust horizontal scroll
    if (cursor_column < scroll_column) {
        scroll_column = cursor_column;
    } else if (cursor_column >= scroll_column + visible_columns) {
        scroll_column = cursor_column - visible_columns + 1;
    }
    
    main_window->invalidate();
}

void TextEditorApp::select_all() {
    has_selection = true;
    selection_start_line = 0;
    selection_start_column = 0;
    selection_end_line = lines.size() - 1;
    selection_end_column = lines.back().length();
    
    main_window->invalidate();
}

void TextEditorApp::clear_selection() {
    has_selection = false;
    main_window->invalidate();
}

void TextEditorApp::copy_text() {
    if (has_selection) {
        clipboard_text = get_selected_text();
        std::cout << "[TEXT_EDITOR] Copied text to clipboard" << std::endl;
    }
}

void TextEditorApp::cut_text() {
    if (has_selection) {
        clipboard_text = get_selected_text();
        delete_selection();
        std::cout << "[TEXT_EDITOR] Cut text to clipboard" << std::endl;
    }
}

void TextEditorApp::paste_text() {
    if (!clipboard_text.empty()) {
        insert_text(clipboard_text);
        std::cout << "[TEXT_EDITOR] Pasted text from clipboard" << std::endl;
    }
}

std::string TextEditorApp::get_selected_text() {
    if (!has_selection) return "";
    
    int start_line = std::min(selection_start_line, selection_end_line);
    int end_line = std::max(selection_start_line, selection_end_line);
    int start_col = (selection_start_line < selection_end_line) ? selection_start_column : 
                    (selection_start_line > selection_end_line) ? selection_end_column : 
                    std::min(selection_start_column, selection_end_column);
    int end_col = (selection_start_line < selection_end_line) ? selection_end_column : 
                  (selection_start_line > selection_end_line) ? selection_start_column : 
                  std::max(selection_start_column, selection_end_column);
    
    if (start_line == end_line) {
        return lines[start_line].substr(start_col, end_col - start_col);
    }
    
    std::string result;
    for (int i = start_line; i <= end_line; i++) {
        if (i == start_line) {
            result += lines[i].substr(start_col);
        } else if (i == end_line) {
            result += "\n" + lines[i].substr(0, end_col);
        } else {
            result += "\n" + lines[i];
        }
    }
    
    return result;
}

void TextEditorApp::delete_selection() {
    if (!has_selection) return;
    
    int start_line = std::min(selection_start_line, selection_end_line);
    int end_line = std::max(selection_start_line, selection_end_line);
    int start_col = (selection_start_line < selection_end_line) ? selection_start_column : 
                    (selection_start_line > selection_end_line) ? selection_end_column : 
                    std::min(selection_start_column, selection_end_column);
    int end_col = (selection_start_line < selection_end_line) ? selection_end_column : 
                  (selection_start_line > selection_end_line) ? selection_start_column : 
                  std::max(selection_start_column, selection_end_column);
    
    delete_text(start_line, start_col, end_line, end_col);
    clear_selection();
}

void TextEditorApp::draw_selection(PixelBuffer* buffer) {
    if (!has_selection) return;
    
    int start_line = std::min(selection_start_line, selection_end_line);
    int end_line = std::max(selection_start_line, selection_end_line);
    int start_col = (selection_start_line < selection_end_line) ? selection_start_column : 
                    (selection_start_line > selection_end_line) ? selection_end_column : 
                    std::min(selection_start_column, selection_end_column);
    int end_col = (selection_start_line < selection_end_line) ? selection_end_column : 
                  (selection_start_line > selection_end_line) ? selection_start_column : 
                  std::max(selection_start_column, selection_end_column);
    
    int text_area_y = MENU_HEIGHT + TOOLBAR_HEIGHT;
    int text_x = get_text_area_x();
    
    for (int line = start_line; line <= end_line; line++) {
        if (line >= scroll_line && line < scroll_line + visible_lines) {
            int sel_start_col = (line == start_line) ? start_col : 0;
            int sel_end_col = (line == end_line) ? end_col : lines[line].length();
            
            sel_start_col = std::max(sel_start_col, scroll_column);
            sel_end_col = std::min(sel_end_col, scroll_column + visible_columns);
            
            if (sel_start_col < sel_end_col) {
                int sel_x = text_x + (sel_start_col - scroll_column) * CHAR_WIDTH;
                int sel_y = text_area_y + (line - scroll_line) * LINE_HEIGHT;
                int sel_width = (sel_end_col - sel_start_col) * CHAR_WIDTH;
                
                buffer->fill_rect(Rect(sel_x, sel_y + 2, sel_width, LINE_HEIGHT - 4), Color(200, 220, 255));
            }
        }
    }
}

void TextEditorApp::add_undo_action(const EditorAction& action) {
    undo_stack.push_back(action);
    if (undo_stack.size() > static_cast<size_t>(max_undo_levels)) {
        undo_stack.erase(undo_stack.begin());
    }
    redo_stack.clear();
}

void TextEditorApp::perform_undo() {
    if (undo_stack.empty()) return;
    
    EditorAction action = undo_stack.back();
    undo_stack.pop_back();
    
    // Reverse the action
    if (action.type == EditorAction::INSERT) {
        delete_text(action.line, action.column, action.line, action.column + action.text.length());
    } else if (action.type == EditorAction::DELETE) {
        cursor_line = action.line;
        cursor_column = action.column;
        insert_text(action.text);
    }
    
    redo_stack.push_back(action);
    main_window->invalidate();
}

void TextEditorApp::perform_redo() {
    if (redo_stack.empty()) return;
    
    EditorAction action = redo_stack.back();
    redo_stack.pop_back();
    
    // Redo the action
    if (action.type == EditorAction::INSERT) {
        cursor_line = action.line;
        cursor_column = action.column;
        insert_text(action.text);
    } else if (action.type == EditorAction::DELETE) {
        delete_text(action.line, action.column, action.line, action.column + action.text.length());
    }
    
    undo_stack.push_back(action);
    main_window->invalidate();
}

void TextEditorApp::clear_undo_history() {
    undo_stack.clear();
    redo_stack.clear();
}

bool TextEditorApp::open_file(const std::string& file_path) {
    if (is_modified) {
        // Should show save changes dialog
        std::cout << "[TEXT_EDITOR] Warning: Current document has unsaved changes" << std::endl;
    }
    
    return load_file(file_path);
}

bool TextEditorApp::create_new_file() {
    if (is_modified) {
        // Should show save changes dialog
        std::cout << "[TEXT_EDITOR] Warning: Current document has unsaved changes" << std::endl;
    }
    
    new_document();
    return true;
}

bool TextEditorApp::save_current_file() {
    return save_file();
}

void TextEditorApp::handle_menu_click(int x, int y) {
    // Simple menu handling
    if (x >= 10 && x <= 50) {
        // File menu
        std::cout << "[TEXT_EDITOR] File menu clicked" << std::endl;
    } else if (x >= 50 && x <= 90) {
        // Edit menu
        std::cout << "[TEXT_EDITOR] Edit menu clicked" << std::endl;
    } else if (x >= 90 && x <= 130) {
        // View menu
        std::cout << "[TEXT_EDITOR] View menu clicked" << std::endl;
    } else if (x >= 130 && x <= 170) {
        // Help menu
        std::cout << "[TEXT_EDITOR] Help menu clicked" << std::endl;
    }
}

void TextEditorApp::handle_toolbar_click(int x, int y) {
    if (y >= 3 && y <= 27) {
        if (x >= 5 && x <= 29) {
            // New button
            new_document();
        } else if (x >= 35 && x <= 59) {
            // Open button
            std::cout << "[TEXT_EDITOR] Open button clicked" << std::endl;
        } else if (x >= 65 && x <= 89) {
            // Save button
            save_file();
        } else if (x >= 105 && x <= 129) {
            // Copy button
            copy_text();
        } else if (x >= 135 && x <= 159) {
            // Paste button
            paste_text();
        }
    }
}

void TextEditorApp::handle_text_area_click(int x, int y) {
    int text_x = get_text_area_x();
    
    if (x >= text_x) {
        int clicked_column = (x - text_x) / CHAR_WIDTH + scroll_column;
        int clicked_line = y / LINE_HEIGHT + scroll_line;
        
        if (clicked_line >= 0 && clicked_line < static_cast<int>(lines.size())) {
            clicked_column = std::min(clicked_column, static_cast<int>(lines[clicked_line].length()));
            move_cursor(clicked_line, clicked_column);
        }
    }
}

void TextEditorApp::show_find_dialog() {
    std::cout << "[TEXT_EDITOR] Find dialog requested" << std::endl;
    // In a real implementation, this would show a find dialog
}
