#include "syscalls.h"
#include "kernel.h"
#include <iostream>
#include <cstring>

SystemCalls::SystemCalls(MyOS* kernel_instance) : kernel(kernel_instance) {
    std::cout << "[SYSCALLS] System call handler initialized" << std::endl;
}

SystemCalls::~SystemCalls() {
    std::cout << "[SYSCALLS] System call handler shutdown" << std::endl;
}

int SystemCalls::handle_syscall(int syscall_num, void* params) {
    syscall_params* p = static_cast<syscall_params*>(params);
    
    switch (syscall_num) {
        case SYS_READ:
            return sys_read(p);
        case SYS_WRITE:
            return sys_write(p);
        case SYS_OPEN:
            return sys_open(p);
        case SYS_CLOSE:
            return sys_close(p);
        case SYS_FORK:
            return sys_fork(p);
        case SYS_EXEC:
            return sys_exec(p);
        case SYS_EXIT:
            return sys_exit(p);
        case SYS_MALLOC:
            return sys_malloc(p);
        case SYS_FREE:
            return sys_free(p);
        case SYS_GETPID:
            return sys_getpid(p);
        case SYS_KILL:
            return sys_kill(p);
        default:
            std::cerr << "[SYSCALLS] Unknown system call: " << syscall_num << std::endl;
            return -1;
    }
}

int SystemCalls::sys_read(syscall_params* params) {
    int fd = static_cast<int>(params->arg1);
    void* buffer = params->ptr;
    size_t count = static_cast<size_t>(params->arg2);
    
    std::cout << "[SYSCALLS] sys_read(fd=" << fd << ", count=" << count << ")" << std::endl;
    
    // Simulate file read
    if (fd == 0) { // stdin
        // For now, return empty read
        return 0;
    }
    
    // Read from file system
    std::string content = kernel->read_file("file_" + std::to_string(fd));
    if (content.empty()) return 0;
    
    size_t bytes_to_copy = std::min(count, content.length());
    memcpy(buffer, content.c_str(), bytes_to_copy);
    return static_cast<int>(bytes_to_copy);
}

int SystemCalls::sys_write(syscall_params* params) {
    int fd = static_cast<int>(params->arg1);
    const void* buffer = params->ptr;
    size_t count = static_cast<size_t>(params->arg2);
    
    if (fd == 1 || fd == 2) { // stdout or stderr
        std::string output(static_cast<const char*>(buffer), count);
        std::cout << "[APP OUTPUT] " << output;
        return static_cast<int>(count);
    }
    
    // Write to file
    std::string content(static_cast<const char*>(buffer), count);
    if (kernel->write_file("file_" + std::to_string(fd), content)) {
        return static_cast<int>(count);
    }
    
    return -1;
}

int SystemCalls::sys_open(syscall_params* params) {
    const char* pathname = params->str;
    int flags = static_cast<int>(params->arg1);
    
    std::cout << "[SYSCALLS] sys_open(" << pathname << ", flags=" << flags << ")" << std::endl;
    
    // Create file if it doesn't exist
    if (!kernel->read_file(pathname).empty() || kernel->create_file(pathname)) {
        return 3; // Return a fake file descriptor
    }
    
    return -1;
}

int SystemCalls::sys_close(syscall_params* params) {
    int fd = static_cast<int>(params->arg1);
    std::cout << "[SYSCALLS] sys_close(fd=" << fd << ")" << std::endl;
    return 0; // Success
}

int SystemCalls::sys_fork(syscall_params* params) {
    std::cout << "[SYSCALLS] sys_fork()" << std::endl;
    // Create a new process
    int pid = kernel->create_process("forked_process");
    return pid;
}

int SystemCalls::sys_exec(syscall_params* params) {
    const char* pathname = params->str;
    std::cout << "[SYSCALLS] sys_exec(" << pathname << ")" << std::endl;
    
    // Replace current process with new executable
    int pid = kernel->create_process(pathname);
    return pid;
}

int SystemCalls::sys_exit(syscall_params* params) {
    int status = static_cast<int>(params->arg1);
    std::cout << "[SYSCALLS] sys_exit(status=" << status << ")" << std::endl;
    
    // Terminate current process
    return 0;
}

int SystemCalls::sys_malloc(syscall_params* params) {
    size_t size = static_cast<size_t>(params->arg1);
    void* ptr = kernel->allocate_memory(size);
    return reinterpret_cast<intptr_t>(ptr);
}

int SystemCalls::sys_free(syscall_params* params) {
    void* ptr = params->ptr;
    kernel->free_memory(ptr);
    return 0;
}

int SystemCalls::sys_getpid(syscall_params* params) {
    // Return current process ID
    return 1234; // Fake PID for demonstration
}

int SystemCalls::sys_kill(syscall_params* params) {
    int pid = static_cast<int>(params->arg1);
    int signal = static_cast<int>(params->arg2);
    
    std::cout << "[SYSCALLS] sys_kill(pid=" << pid << ", signal=" << signal << ")" << std::endl;
    
    return kernel->terminate_process(pid) ? 0 : -1;
}

bool SystemCalls::validate_user_pointer(void* ptr) {
    // Basic pointer validation
    return ptr != nullptr;
}

bool SystemCalls::validate_user_string(const char* str) {
    // Basic string validation
    return str != nullptr;
}
