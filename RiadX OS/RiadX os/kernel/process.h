#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

enum ProcessState {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
};

struct ProcessControlBlock {
    int pid;
    int parent_pid;
    ProcessState state;
    std::string executable_path;
    void* memory_base;
    size_t memory_size;
    std::map<std::string, std::string> environment;
    int priority;
    uint64_t cpu_time;
    uint64_t start_time;
    std::unique_ptr<std::thread> process_thread;
    std::atomic<bool> should_terminate;
};

class ProcessManager {
private:
    std::vector<std::unique_ptr<ProcessControlBlock>> process_table;
    std::map<int, ProcessControlBlock*> pid_map;
    std::mutex process_mutex;
    
    int next_pid;
    ProcessControlBlock* current_process;
    bool scheduler_running;
    
    // Process lifecycle
    ProcessControlBlock* create_pcb(const std::string& executable_path);
    bool load_executable(ProcessControlBlock* pcb);
    void execute_process(ProcessControlBlock* pcb);
    void cleanup_process(ProcessControlBlock* pcb);
    
    // Scheduling
    ProcessControlBlock* select_next_process();
    void context_switch(ProcessControlBlock* old_proc, ProcessControlBlock* new_proc);

public:
    ProcessManager();
    ~ProcessManager();
    
    bool initialize();
    void shutdown();
    
    // Process management
    int create_process(const std::string& executable_path);
    bool terminate_process(int pid);
    bool suspend_process(int pid);
    bool resume_process(int pid);
    
    // Scheduling
    void schedule();
    void set_process_priority(int pid, int priority);
    
    // Process information
    ProcessControlBlock* get_process(int pid);
    std::vector<ProcessControlBlock*> get_all_processes();
    ProcessControlBlock* get_current_process();
    
    // Inter-process communication
    bool send_signal(int pid, int signal);
    bool wait_for_process(int pid);
    
    // Debug functions
    void print_process_table();
    size_t get_process_count();
};

#endif
