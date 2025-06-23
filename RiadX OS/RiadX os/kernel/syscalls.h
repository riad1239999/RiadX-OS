#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <cstdint>
#include <string>

// System call numbers
#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_OPEN    2
#define SYS_CLOSE   3
#define SYS_FORK    4
#define SYS_EXEC    5
#define SYS_EXIT    6
#define SYS_MALLOC  7
#define SYS_FREE    8
#define SYS_GETPID  9
#define SYS_KILL    10

class MyOS; // Forward declaration

struct syscall_params {
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    void* ptr;
    char* str;
};

class SystemCalls {
private:
    MyOS* kernel;
    
    // Individual system call handlers
    int sys_read(syscall_params* params);
    int sys_write(syscall_params* params);
    int sys_open(syscall_params* params);
    int sys_close(syscall_params* params);
    int sys_fork(syscall_params* params);
    int sys_exec(syscall_params* params);
    int sys_exit(syscall_params* params);
    int sys_malloc(syscall_params* params);
    int sys_free(syscall_params* params);
    int sys_getpid(syscall_params* params);
    int sys_kill(syscall_params* params);

public:
    SystemCalls(MyOS* kernel_instance);
    ~SystemCalls();
    
    // Main system call handler
    int handle_syscall(int syscall_num, void* params);
    
    // Utility functions
    bool validate_user_pointer(void* ptr);
    bool validate_user_string(const char* str);
};

#endif
