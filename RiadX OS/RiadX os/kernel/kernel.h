#ifndef KERNEL_H
#define KERNEL_H

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <mutex>
#include "syscalls.h"
#include "memory.h"
#include "process.h"
#include "../drivers/display.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"
#include "../drivers/filesystem.h"
#include "../gui/gui_manager.h"

class RiadXOS {
private:
    std::unique_ptr<SystemCalls> syscalls;
    std::unique_ptr<MemoryManager> memory_manager;
    std::unique_ptr<ProcessManager> process_manager;
    std::unique_ptr<DisplayDriver> display_driver;
    std::unique_ptr<KeyboardDriver> keyboard_driver;
    std::unique_ptr<MouseDriver> mouse_driver;
    std::unique_ptr<FileSystem> filesystem;
    std::unique_ptr<GUIManager> gui_manager;
    
    bool running;
    std::mutex kernel_mutex;
    
    // Interrupt handling
    void handle_interrupt(int interrupt_id);
    void scheduler_tick();

public:
    MyOS();
    ~MyOS();
    
    bool initialize();
    void run();
    void shutdown();
    
    // System call interface
    int system_call(int call_id, void* params);
    
    // Driver management
    bool register_driver(const std::string& name, void* driver);
    void* get_driver(const std::string& name);
    
    // Process management
    int create_process(const std::string& executable_path);
    bool terminate_process(int pid);
    
    // Memory management
    void* allocate_memory(size_t size);
    void free_memory(void* ptr);
    
    // File system operations
    bool create_file(const std::string& path);
    bool delete_file(const std::string& path);
    std::string read_file(const std::string& path);
    bool write_file(const std::string& path, const std::string& content);
};

#endif
