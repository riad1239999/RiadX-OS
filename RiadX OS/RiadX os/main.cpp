#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include "kernel/kernel.h"
#include "boot/bootloader.h"
#include "gui/gui_manager.h"

// Global OS instance
MyOS* os_instance = nullptr;

void signal_handler(int signal) {
    if (os_instance) {
        std::cout << "\n[SYSTEM] Shutdown signal received. Shutting down OS..." << std::endl;
        os_instance->shutdown();
        exit(0);
    }
}

int main() {
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "=== MyOS Bootloader ===" << std::endl;
    std::cout << "Starting boot sequence..." << std::endl;

    // Initialize bootloader
    Bootloader bootloader;
    if (!bootloader.boot()) {
        std::cerr << "Boot failed!" << std::endl;
        return 1;
    }

    // Initialize operating system
    MyOS os;
    os_instance = &os;
    
    if (!os.initialize()) {
        std::cerr << "OS initialization failed!" << std::endl;
        return 1;
    }

    std::cout << "[BOOT] OS initialized successfully" << std::endl;
    std::cout << "[BOOT] Starting GUI..." << std::endl;

    // Start the operating system
    os.run();

    return 0;
}
