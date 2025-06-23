#include "kernel.h"
#include <iostream>
#include <thread>
#include <chrono>

RiadXOS::RiadXOS() : running(false) {
    std::cout << "[KERNEL] Initializing kernel..." << std::endl;
}

RiadXOS::~RiadXOS() {
    shutdown();
}

bool RiadXOS::initialize() {
    std::lock_guard<std::mutex> lock(kernel_mutex);
    
    try {
        // Initialize memory manager
        memory_manager = std::make_unique<MemoryManager>();
        if (!memory_manager->initialize()) {
            std::cerr << "[KERNEL] Failed to initialize memory manager" << std::endl;
            return false;
        }
        
        // Initialize process manager
        process_manager = std::make_unique<ProcessManager>();
        if (!process_manager->initialize()) {
            std::cerr << "[KERNEL] Failed to initialize process manager" << std::endl;
            return false;
        }
        
        // Initialize drivers
        display_driver = std::make_unique<DisplayDriver>();
        keyboard_driver = std::make_unique<KeyboardDriver>();
        mouse_driver = std::make_unique<MouseDriver>();
        filesystem = std::make_unique<FileSystem>();
        
        if (!display_driver->initialize() ||
            !keyboard_driver->initialize() ||
            !mouse_driver->initialize() ||
            !filesystem->initialize()) {
            std::cerr << "[KERNEL] Failed to initialize drivers" << std::endl;
            return false;
        }
        
        // Initialize system calls
        syscalls = std::make_unique<SystemCalls>(this);
        
        // Initialize GUI manager
        gui_manager = std::make_unique<GUIManager>(display_driver.get(), 
                                                   keyboard_driver.get(), 
                                                   mouse_driver.get());
        if (!gui_manager->initialize()) {
            std::cerr << "[KERNEL] Failed to initialize GUI manager" << std::endl;
            return false;
        }
        
        running = true;
        std::cout << "[KERNEL] Kernel initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[KERNEL] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void RiadXOS::run() {
    std::cout << "[KERNEL] Starting kernel main loop..." << std::endl;
    
    // Start scheduler in separate thread
    std::thread scheduler_thread([this]() {
        while (running) {
            scheduler_tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Start GUI main loop
    gui_manager->run();
    
    // Wait for scheduler to finish
    if (scheduler_thread.joinable()) {
        scheduler_thread.join();
    }
}

void RiadXOS::shutdown() {
    std::lock_guard<std::mutex> lock(kernel_mutex);
    
    if (!running) return;
    
    std::cout << "[KERNEL] Shutting down..." << std::endl;
    running = false;
    
    // Shutdown components in reverse order
    if (gui_manager) gui_manager->shutdown();
    if (process_manager) process_manager->shutdown();
    if (filesystem) filesystem->shutdown();
    
    std::cout << "[KERNEL] Shutdown complete" << std::endl;
}

void MyOS::scheduler_tick() {
    // Simple round-robin scheduler
    if (process_manager) {
        process_manager->schedule();
    }
}

void MyOS::handle_interrupt(int interrupt_id) {
    switch (interrupt_id) {
        case 0x20: // Timer interrupt
            scheduler_tick();
            break;
        case 0x21: // Keyboard interrupt
            if (keyboard_driver) {
                keyboard_driver->handle_interrupt();
            }
            break;
        case 0x2C: // Mouse interrupt
            if (mouse_driver) {
                mouse_driver->handle_interrupt();
            }
            break;
        default:
            std::cout << "[KERNEL] Unknown interrupt: " << interrupt_id << std::endl;
    }
}

int MyOS::system_call(int call_id, void* params) {
    if (syscalls) {
        return syscalls->handle_syscall(call_id, params);
    }
    return -1;
}

bool MyOS::register_driver(const std::string& name, void* driver) {
    // Driver registration logic
    std::cout << "[KERNEL] Registering driver: " << name << std::endl;
    return true;
}

void* MyOS::get_driver(const std::string& name) {
    // Driver lookup logic
    if (name == "display") return display_driver.get();
    if (name == "keyboard") return keyboard_driver.get();
    if (name == "mouse") return mouse_driver.get();
    if (name == "filesystem") return filesystem.get();
    return nullptr;
}

int MyOS::create_process(const std::string& executable_path) {
    if (process_manager) {
        return process_manager->create_process(executable_path);
    }
    return -1;
}

bool MyOS::terminate_process(int pid) {
    if (process_manager) {
        return process_manager->terminate_process(pid);
    }
    return false;
}

void* MyOS::allocate_memory(size_t size) {
    if (memory_manager) {
        return memory_manager->allocate(size);
    }
    return nullptr;
}

void MyOS::free_memory(void* ptr) {
    if (memory_manager) {
        memory_manager->deallocate(ptr);
    }
}

bool MyOS::create_file(const std::string& path) {
    if (filesystem) {
        return filesystem->create_file(path);
    }
    return false;
}

bool MyOS::delete_file(const std::string& path) {
    if (filesystem) {
        return filesystem->delete_file(path);
    }
    return false;
}

std::string MyOS::read_file(const std::string& path) {
    if (filesystem) {
        return filesystem->read_file(path);
    }
    return "";
}

bool MyOS::write_file(const std::string& path, const std::string& content) {
    if (filesystem) {
        return filesystem->write_file(path, content);
    }
    return false;
}
