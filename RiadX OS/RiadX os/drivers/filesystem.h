#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <fstream>
#include <cstring>

// File system constants
#define BLOCK_SIZE 4096
#define MAX_FILENAME_LENGTH 255
#define MAX_PATH_LENGTH 4096

// File types
enum FileType {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_DEVICE
};

// File permissions
enum FilePermission {
    PERM_READ = 1,
    PERM_WRITE = 2,
    PERM_EXECUTE = 4
};

// File attributes
struct FileAttributes {
    FileType type;
    size_t size;
    uint64_t creation_time;
    uint64_t modification_time;
    uint64_t access_time;
    int permissions;
    int owner_id;
    int group_id;
    
    FileAttributes() : type(FILE_TYPE_REGULAR), size(0), 
                      creation_time(0), modification_time(0), access_time(0),
                      permissions(PERM_READ | PERM_WRITE), owner_id(0), group_id(0) {}
};

// Directory entry
struct DirectoryEntry {
    std::string name;
    FileAttributes attributes;
    std::string full_path;
    
    DirectoryEntry(const std::string& n, const FileAttributes& attr, const std::string& path)
        : name(n), attributes(attr), full_path(path) {}
};

// File handle
struct FileHandle {
    int fd;
    std::string path;
    int flags;
    size_t position;
    bool is_open;
    
    FileHandle() : fd(-1), flags(0), position(0), is_open(false) {}
};

// Disk block
struct DiskBlock {
    uint8_t data[BLOCK_SIZE];
    bool is_allocated;
    int next_block;
    
    DiskBlock() : is_allocated(false), next_block(-1) {
        memset(data, 0, BLOCK_SIZE);
    }
};

class FileSystem {
private:
    std::map<std::string, std::string> file_contents;
    std::map<std::string, FileAttributes> file_attributes;
    std::map<std::string, std::vector<std::string>> directory_contents;
    std::vector<FileHandle> open_files;
    std::mutex fs_mutex;
    
    // Disk simulation
    std::vector<DiskBlock> disk_blocks;
    std::vector<bool> block_allocation_table;
    size_t total_blocks;
    size_t free_blocks;
    
    int next_fd;
    std::string root_path;
    std::string current_directory;
    
    // Internal utilities
    std::string normalize_path(const std::string& path);
    std::string get_parent_directory(const std::string& path);
    std::string get_filename(const std::string& path);
    bool is_absolute_path(const std::string& path);
    bool is_valid_filename(const std::string& filename);
    
    // Block management
    int allocate_block();
    void free_block(int block_num);
    bool write_block(int block_num, const uint8_t* data);
    bool read_block(int block_num, uint8_t* data);
    
    // File operations helpers
    bool create_directory_entry(const std::string& path, FileType type);
    bool remove_directory_entry(const std::string& path);
    void update_file_times(const std::string& path, bool access = true, bool modify = false);

public:
    FileSystem();
    ~FileSystem();
    
    bool initialize();
    void shutdown();
    
    // File operations
    bool create_file(const std::string& path);
    bool delete_file(const std::string& path);
    bool file_exists(const std::string& path);
    bool is_directory(const std::string& path);
    
    // Directory operations
    bool create_directory(const std::string& path);
    bool delete_directory(const std::string& path);
    std::vector<DirectoryEntry> list_directory(const std::string& path);
    bool change_directory(const std::string& path);
    std::string get_current_directory();
    
    // File I/O
    int open_file(const std::string& path, int flags);
    bool close_file(int fd);
    std::string read_file(const std::string& path);
    bool write_file(const std::string& path, const std::string& content);
    ssize_t read_file_fd(int fd, void* buffer, size_t count);
    ssize_t write_file_fd(int fd, const void* buffer, size_t count);
    
    // File attributes
    bool get_file_attributes(const std::string& path, FileAttributes& attr);
    bool set_file_attributes(const std::string& path, const FileAttributes& attr);
    size_t get_file_size(const std::string& path);
    
    // File operations
    bool copy_file(const std::string& src, const std::string& dest);
    bool move_file(const std::string& src, const std::string& dest);
    bool rename_file(const std::string& old_name, const std::string& new_name);
    
    // File system info
    size_t get_total_space();
    size_t get_free_space();
    size_t get_used_space();
    
    // Path operations
    std::string resolve_path(const std::string& path);
    bool is_valid_path(const std::string& path);
    
    // Disk operations
    bool format_disk();
    bool check_disk();
    void defragment_disk();
    
    // Debug functions
    void print_file_system_info();
    void print_directory_tree(const std::string& path = "/", int depth = 0);
    void create_sample_files();
};

#endif
