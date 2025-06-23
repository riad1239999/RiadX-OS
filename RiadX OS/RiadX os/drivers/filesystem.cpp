#include "filesystem.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>

FileSystem::FileSystem() 
    : total_blocks(1024), free_blocks(1024), next_fd(3),
      root_path("/"), current_directory("/") {
    
    std::cout << "[FILESYSTEM] File system initializing..." << std::endl;
}

FileSystem::~FileSystem() {
    shutdown();
}

bool FileSystem::initialize() {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    try {
        // Initialize disk blocks
        disk_blocks.resize(total_blocks);
        block_allocation_table.resize(total_blocks, false);
        
        // Create root directory
        create_directory_entry("/", FILE_TYPE_DIRECTORY);
        directory_contents["/"] = std::vector<std::string>();
        
        // Create sample directory structure
        create_sample_files();
        
        std::cout << "[FILESYSTEM] File system initialized with " 
                  << total_blocks << " blocks (" << (total_blocks * BLOCK_SIZE / 1024) << "KB)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[FILESYSTEM] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void FileSystem::shutdown() {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    // Close all open files
    for (auto& handle : open_files) {
        if (handle.is_open) {
            handle.is_open = false;
        }
    }
    open_files.clear();
    
    file_contents.clear();
    file_attributes.clear();
    directory_contents.clear();
    disk_blocks.clear();
    block_allocation_table.clear();
    
    std::cout << "[FILESYSTEM] File system shutdown complete" << std::endl;
}

std::string FileSystem::normalize_path(const std::string& path) {
    if (path.empty()) return "/";
    
    std::string normalized = path;
    
    // Convert to absolute path if relative
    if (!is_absolute_path(normalized)) {
        normalized = current_directory + "/" + normalized;
    }
    
    // Remove duplicate slashes
    size_t pos = 0;
    while ((pos = normalized.find("//", pos)) != std::string::npos) {
        normalized.erase(pos, 1);
    }
    
    // Handle . and .. directories
    std::vector<std::string> components;
    std::stringstream ss(normalized);
    std::string component;
    
    while (std::getline(ss, component, '/')) {
        if (component.empty() || component == ".") {
            continue;
        } else if (component == "..") {
            if (!components.empty()) {
                components.pop_back();
            }
        } else {
            components.push_back(component);
        }
    }
    
    // Reconstruct path
    if (components.empty()) {
        return "/";
    }
    
    std::string result = "/";
    for (size_t i = 0; i < components.size(); i++) {
        result += components[i];
        if (i < components.size() - 1) {
            result += "/";
        }
    }
    
    return result;
}

std::string FileSystem::get_parent_directory(const std::string& path) {
    std::string normalized = normalize_path(path);
    if (normalized == "/") return "/";
    
    size_t last_slash = normalized.find_last_of('/');
    if (last_slash == 0) return "/";
    
    return normalized.substr(0, last_slash);
}

std::string FileSystem::get_filename(const std::string& path) {
    std::string normalized = normalize_path(path);
    if (normalized == "/") return "";
    
    size_t last_slash = normalized.find_last_of('/');
    return normalized.substr(last_slash + 1);
}

bool FileSystem::is_absolute_path(const std::string& path) {
    return !path.empty() && path[0] == '/';
}

bool FileSystem::is_valid_filename(const std::string& filename) {
    if (filename.empty() || filename.length() > MAX_FILENAME_LENGTH) {
        return false;
    }
    
    // Check for invalid characters
    const std::string invalid_chars = "\\/:*?\"<>|";
    return filename.find_first_of(invalid_chars) == std::string::npos;
}

bool FileSystem::create_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::string normalized_path = normalize_path(path);
    
    if (file_exists(normalized_path)) {
        std::cerr << "[FILESYSTEM] File already exists: " << normalized_path << std::endl;
        return false;
    }
    
    std::string parent_dir = get_parent_directory(normalized_path);
    if (!is_directory(parent_dir)) {
        std::cerr << "[FILESYSTEM] Parent directory does not exist: " << parent_dir << std::endl;
        return false;
    }
    
    // Create file entry
    if (!create_directory_entry(normalized_path, FILE_TYPE_REGULAR)) {
        return false;
    }
    
    file_contents[normalized_path] = "";
    
    // Add to parent directory
    directory_contents[parent_dir].push_back(get_filename(normalized_path));
    
    std::cout << "[FILESYSTEM] Created file: " << normalized_path << std::endl;
    return true;
}

bool FileSystem::delete_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::string normalized_path = normalize_path(path);
    
    if (!file_exists(normalized_path)) {
        std::cerr << "[FILESYSTEM] File does not exist: " << normalized_path << std::endl;
        return false;
    }
    
    if (is_directory(normalized_path)) {
        std::cerr << "[FILESYSTEM] Cannot delete directory with delete_file: " << normalized_path << std::endl;
        return false;
    }
    
    // Remove from parent directory
    std::string parent_dir = get_parent_directory(normalized_path);
    auto& parent_contents = directory_contents[parent_dir];
    std::string filename = get_filename(normalized_path);
    
    parent_contents.erase(
        std::remove(parent_contents.begin(), parent_contents.end(), filename),
        parent_contents.end());
    
    // Remove file data
    file_contents.erase(normalized_path);
    file_attributes.erase(normalized_path);
    
    std::cout << "[FILESYSTEM] Deleted file: " << normalized_path << std::endl;
    return true;
}

bool FileSystem::file_exists(const std::string& path) {
    std::string normalized_path = normalize_path(path);
    return file_attributes.find(normalized_path) != file_attributes.end();
}

bool FileSystem::is_directory(const std::string& path) {
    std::string normalized_path = normalize_path(path);
    auto it = file_attributes.find(normalized_path);
    return it != file_attributes.end() && it->second.type == FILE_TYPE_DIRECTORY;
}

bool FileSystem::create_directory(const std::string& path) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::string normalized_path = normalize_path(path);
    
    if (file_exists(normalized_path)) {
        std::cerr << "[FILESYSTEM] Directory already exists: " << normalized_path << std::endl;
        return false;
    }
    
    std::string parent_dir = get_parent_directory(normalized_path);
    if (parent_dir != normalized_path && !is_directory(parent_dir)) {
        std::cerr << "[FILESYSTEM] Parent directory does not exist: " << parent_dir << std::endl;
        return false;
    }
    
    // Create directory entry
    if (!create_directory_entry(normalized_path, FILE_TYPE_DIRECTORY)) {
        return false;
    }
    
    directory_contents[normalized_path] = std::vector<std::string>();
    
    // Add to parent directory
    if (parent_dir != normalized_path) {
        directory_contents[parent_dir].push_back(get_filename(normalized_path));
    }
    
    std::cout << "[FILESYSTEM] Created directory: " << normalized_path << std::endl;
    return true;
}

bool FileSystem::delete_directory(const std::string& path) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::string normalized_path = normalize_path(path);
    
    if (!is_directory(normalized_path)) {
        std::cerr << "[FILESYSTEM] Not a directory: " << normalized_path << std::endl;
        return false;
    }
    
    if (normalized_path == "/") {
        std::cerr << "[FILESYSTEM] Cannot delete root directory" << std::endl;
        return false;
    }
    
    // Check if directory is empty
    if (!directory_contents[normalized_path].empty()) {
        std::cerr << "[FILESYSTEM] Directory not empty: " << normalized_path << std::endl;
        return false;
    }
    
    // Remove from parent directory
    std::string parent_dir = get_parent_directory(normalized_path);
    auto& parent_contents = directory_contents[parent_dir];
    std::string dirname = get_filename(normalized_path);
    
    parent_contents.erase(
        std::remove(parent_contents.begin(), parent_contents.end(), dirname),
        parent_contents.end());
    
    // Remove directory data
    directory_contents.erase(normalized_path);
    file_attributes.erase(normalized_path);
    
    std::cout << "[FILESYSTEM] Deleted directory: " << normalized_path << std::endl;
    return true;
}

std::vector<DirectoryEntry> FileSystem::list_directory(const std::string& path) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::vector<DirectoryEntry> entries;
    std::string normalized_path = normalize_path(path);
    
    if (!is_directory(normalized_path)) {
        std::cerr << "[FILESYSTEM] Not a directory: " << normalized_path << std::endl;
        return entries;
    }
    
    auto it = directory_contents.find(normalized_path);
    if (it != directory_contents.end()) {
        for (const std::string& name : it->second) {
            std::string full_path = normalized_path == "/" ? "/" + name : normalized_path + "/" + name;
            FileAttributes attr;
            if (get_file_attributes(full_path, attr)) {
                entries.emplace_back(name, attr, full_path);
            }
        }
    }
    
    return entries;
}

bool FileSystem::change_directory(const std::string& path) {
    std::string normalized_path = normalize_path(path);
    
    if (!is_directory(normalized_path)) {
        std::cerr << "[FILESYSTEM] Not a directory: " << normalized_path << std::endl;
        return false;
    }
    
    current_directory = normalized_path;
    std::cout << "[FILESYSTEM] Changed directory to: " << current_directory << std::endl;
    return true;
}

std::string FileSystem::get_current_directory() {
    return current_directory;
}

std::string FileSystem::read_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::string normalized_path = normalize_path(path);
    
    if (!file_exists(normalized_path)) {
        std::cerr << "[FILESYSTEM] File does not exist: " << normalized_path << std::endl;
        return "";
    }
    
    if (is_directory(normalized_path)) {
        std::cerr << "[FILESYSTEM] Cannot read directory as file: " << normalized_path << std::endl;
        return "";
    }
    
    update_file_times(normalized_path, true, false);
    
    auto it = file_contents.find(normalized_path);
    return (it != file_contents.end()) ? it->second : "";
}

bool FileSystem::write_file(const std::string& path, const std::string& content) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::string normalized_path = normalize_path(path);
    
    // Create file if it doesn't exist
    if (!file_exists(normalized_path)) {
        if (!create_file(normalized_path)) {
            return false;
        }
    }
    
    if (is_directory(normalized_path)) {
        std::cerr << "[FILESYSTEM] Cannot write to directory: " << normalized_path << std::endl;
        return false;
    }
    
    file_contents[normalized_path] = content;
    
    // Update file attributes
    auto it = file_attributes.find(normalized_path);
    if (it != file_attributes.end()) {
        it->second.size = content.length();
        update_file_times(normalized_path, false, true);
    }
    
    std::cout << "[FILESYSTEM] Wrote " << content.length() << " bytes to: " << normalized_path << std::endl;
    return true;
}

bool FileSystem::get_file_attributes(const std::string& path, FileAttributes& attr) {
    std::string normalized_path = normalize_path(path);
    
    auto it = file_attributes.find(normalized_path);
    if (it != file_attributes.end()) {
        attr = it->second;
        return true;
    }
    return false;
}

bool FileSystem::set_file_attributes(const std::string& path, const FileAttributes& attr) {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::string normalized_path = normalize_path(path);
    
    if (!file_exists(normalized_path)) {
        return false;
    }
    
    file_attributes[normalized_path] = attr;
    return true;
}

size_t FileSystem::get_file_size(const std::string& path) {
    FileAttributes attr;
    if (get_file_attributes(path, attr)) {
        return attr.size;
    }
    return 0;
}

bool FileSystem::copy_file(const std::string& src, const std::string& dest) {
    std::string content = read_file(src);
    if (content.empty() && get_file_size(src) == 0) {
        if (!file_exists(src)) {
            return false;
        }
    }
    
    return write_file(dest, content);
}

bool FileSystem::move_file(const std::string& src, const std::string& dest) {
    if (copy_file(src, dest)) {
        return delete_file(src);
    }
    return false;
}

bool FileSystem::rename_file(const std::string& old_name, const std::string& new_name) {
    return move_file(old_name, new_name);
}

size_t FileSystem::get_total_space() {
    return total_blocks * BLOCK_SIZE;
}

size_t FileSystem::get_free_space() {
    return free_blocks * BLOCK_SIZE;
}

size_t FileSystem::get_used_space() {
    return get_total_space() - get_free_space();
}

bool FileSystem::create_directory_entry(const std::string& path, FileType type) {
    FileAttributes attr;
    attr.type = type;
    attr.size = 0;
    
    uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    attr.creation_time = current_time;
    attr.modification_time = current_time;
    attr.access_time = current_time;
    attr.permissions = PERM_READ | PERM_WRITE;
    attr.owner_id = 0;
    attr.group_id = 0;
    
    file_attributes[path] = attr;
    return true;
}

void FileSystem::update_file_times(const std::string& path, bool access, bool modify) {
    auto it = file_attributes.find(path);
    if (it != file_attributes.end()) {
        uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (access) {
            it->second.access_time = current_time;
        }
        if (modify) {
            it->second.modification_time = current_time;
        }
    }
}

void FileSystem::print_file_system_info() {
    std::lock_guard<std::mutex> lock(fs_mutex);
    
    std::cout << "[FILESYSTEM] File System Information:" << std::endl;
    std::cout << "  Total space: " << (get_total_space() / 1024) << " KB" << std::endl;
    std::cout << "  Used space: " << (get_used_space() / 1024) << " KB" << std::endl;
    std::cout << "  Free space: " << (get_free_space() / 1024) << " KB" << std::endl;
    std::cout << "  Total files: " << file_attributes.size() << std::endl;
    std::cout << "  Current directory: " << current_directory << std::endl;
}

void FileSystem::print_directory_tree(const std::string& path, int depth) {
    std::string indent(depth * 2, ' ');
    std::string normalized_path = normalize_path(path);
    
    std::cout << indent << get_filename(normalized_path) << "/" << std::endl;
    
    auto entries = list_directory(normalized_path);
    for (const auto& entry : entries) {
        if (entry.attributes.type == FILE_TYPE_DIRECTORY) {
            print_directory_tree(entry.full_path, depth + 1);
        } else {
            std::cout << indent << "  " << entry.name << " (" << entry.attributes.size << " bytes)" << std::endl;
        }
    }
}

void FileSystem::create_sample_files() {
    // Create sample directory structure
    create_directory("/home");
    create_directory("/home/user");
    create_directory("/home/user/documents");
    create_directory("/home/user/pictures");
    create_directory("/bin");
    create_directory("/etc");
    create_directory("/var");
    create_directory("/tmp");
    
    // Create sample files
    write_file("/home/user/readme.txt", "Welcome to MyOS!\nThis is a sample text file.\n");
    write_file("/home/user/documents/note.txt", "Important notes:\n- Remember to save your work\n- Use the file manager to navigate\n");
    write_file("/etc/config.conf", "# MyOS Configuration\nversion=1.0\ndebug=false\n");
    write_file("/bin/calculator", "Calculator executable");
    write_file("/bin/editor", "Text editor executable");
    write_file("/bin/filemanager", "File manager executable");
    
    std::cout << "[FILESYSTEM] Created sample directory structure and files" << std::endl;
}

int FileSystem::allocate_block() {
    for (size_t i = 0; i < total_blocks; i++) {
        if (!block_allocation_table[i]) {
            block_allocation_table[i] = true;
            disk_blocks[i].is_allocated = true;
            free_blocks--;
            return static_cast<int>(i);
        }
    }
    return -1; // No free blocks
}

void FileSystem::free_block(int block_num) {
    if (block_num >= 0 && block_num < static_cast<int>(total_blocks)) {
        if (block_allocation_table[block_num]) {
            block_allocation_table[block_num] = false;
            disk_blocks[block_num].is_allocated = false;
            free_blocks++;
        }
    }
}

bool FileSystem::write_block(int block_num, const uint8_t* data) {
    if (block_num >= 0 && block_num < static_cast<int>(total_blocks) && data) {
        std::memcpy(disk_blocks[block_num].data, data, BLOCK_SIZE);
        return true;
    }
    return false;
}

bool FileSystem::read_block(int block_num, uint8_t* data) {
    if (block_num >= 0 && block_num < static_cast<int>(total_blocks) && data) {
        std::memcpy(data, disk_blocks[block_num].data, BLOCK_SIZE);
        return true;
    }
    return false;
}
