#include "process.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <random>
#include <cstring>

ProcessManager::ProcessManager() 
    : next_pid(1), current_process(nullptr), scheduler_running(false) {
    std::cout << "[PROCESS] Process manager initializing..." << std::endl;
}

ProcessManager::~ProcessManager() {
    shutdown();
}

bool ProcessManager::initialize() {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    try {
        scheduler_running = true;
        std::cout << "[PROCESS] Process manager initialized" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PROCESS] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void ProcessManager::shutdown() {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    scheduler_running = false;
    
    // Terminate all processes
    for (auto& pcb : process_table) {
        if (pcb && pcb->state != PROCESS_TERMINATED) {
            pcb->should_terminate = true;
            if (pcb->process_thread && pcb->process_thread->joinable()) {
                pcb->process_thread->join();
            }
        }
    }
    
    process_table.clear();
    pid_map.clear();
    current_process = nullptr;
    
    std::cout << "[PROCESS] Process manager shutdown complete" << std::endl;
}

int ProcessManager::create_process(const std::string& executable_path) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    // Create process control block
    auto pcb = create_pcb(executable_path);
    if (!pcb) {
        std::cerr << "[PROCESS] Failed to create PCB for " << executable_path << std::endl;
        return -1;
    }
    
    // Load executable
    if (!load_executable(pcb)) {
        std::cerr << "[PROCESS] Failed to load executable " << executable_path << std::endl;
        cleanup_process(pcb);
        return -1;
    }
    
    int pid = pcb->pid;
    pid_map[pid] = pcb;
    process_table.push_back(std::unique_ptr<ProcessControlBlock>(pcb));
    
    // Start process execution
    pcb->process_thread = std::make_unique<std::thread>(&ProcessManager::execute_process, this, pcb);
    
    std::cout << "[PROCESS] Created process " << pid << " (" << executable_path << ")" << std::endl;
    return pid;
}

bool ProcessManager::terminate_process(int pid) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    auto it = pid_map.find(pid);
    if (it == pid_map.end()) {
        std::cerr << "[PROCESS] Process " << pid << " not found" << std::endl;
        return false;
    }
    
    ProcessControlBlock* pcb = it->second;
    pcb->should_terminate = true;
    pcb->state = PROCESS_TERMINATED;
    
    if (pcb->process_thread && pcb->process_thread->joinable()) {
        pcb->process_thread->join();
    }
    
    cleanup_process(pcb);
    pid_map.erase(it);
    
    // Remove from process table
    process_table.erase(
        std::remove_if(process_table.begin(), process_table.end(),
                      [pid](const std::unique_ptr<ProcessControlBlock>& p) {
                          return p->pid == pid;
                      }),
        process_table.end());
    
    std::cout << "[PROCESS] Terminated process " << pid << std::endl;
    return true;
}

bool ProcessManager::suspend_process(int pid) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    auto it = pid_map.find(pid);
    if (it != pid_map.end()) {
        it->second->state = PROCESS_BLOCKED;
        std::cout << "[PROCESS] Suspended process " << pid << std::endl;
        return true;
    }
    return false;
}

bool ProcessManager::resume_process(int pid) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    auto it = pid_map.find(pid);
    if (it != pid_map.end() && it->second->state == PROCESS_BLOCKED) {
        it->second->state = PROCESS_READY;
        std::cout << "[PROCESS] Resumed process " << pid << std::endl;
        return true;
    }
    return false;
}

ProcessControlBlock* ProcessManager::create_pcb(const std::string& executable_path) {
    ProcessControlBlock* pcb = new ProcessControlBlock();
    pcb->pid = next_pid++;
    pcb->parent_pid = current_process ? current_process->pid : 0;
    pcb->state = PROCESS_READY;
    pcb->executable_path = executable_path;
    pcb->memory_base = nullptr;
    pcb->memory_size = 0;
    pcb->priority = 1;
    pcb->cpu_time = 0;
    pcb->start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    pcb->should_terminate = false;
    
    // Set up environment variables
    pcb->environment["PATH"] = "/bin:/usr/bin";
    pcb->environment["HOME"] = "/home/user";
    pcb->environment["USER"] = "user";
    
    return pcb;
}

bool ProcessManager::load_executable(ProcessControlBlock* pcb) {
    // Simulate loading an executable
    std::cout << "[PROCESS] Loading executable: " << pcb->executable_path << std::endl;
    
    // Allocate memory for the process (simulated)
    pcb->memory_size = 1024 * 64; // 64KB
    pcb->memory_base = std::malloc(pcb->memory_size);
    
    if (!pcb->memory_base) {
        std::cerr << "[PROCESS] Failed to allocate memory for process" << std::endl;
        return false;
    }
    
    // Initialize memory
    memset(pcb->memory_base, 0, pcb->memory_size);
    
    std::cout << "[PROCESS] Allocated " << (pcb->memory_size / 1024) 
              << "KB memory at " << pcb->memory_base << std::endl;
    return true;
}

void ProcessManager::execute_process(ProcessControlBlock* pcb) {
    pcb->state = PROCESS_RUNNING;
    
    std::cout << "[PROCESS] Executing process " << pcb->pid 
              << " (" << pcb->executable_path << ")" << std::endl;
    
    // Simulate process execution
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 5000);
    
    while (!pcb->should_terminate && pcb->state != PROCESS_TERMINATED) {
        // Simulate CPU work
        auto work_time = std::chrono::milliseconds(dis(gen));
        std::this_thread::sleep_for(work_time);
        
        pcb->cpu_time += work_time.count();
        
        // Check if process should yield
        if (pcb->state == PROCESS_BLOCKED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Simulate some output
        if (pcb->executable_path.find("calculator") != std::string::npos) {
            std::cout << "[CALC-" << pcb->pid << "] Performing calculations..." << std::endl;
        } else if (pcb->executable_path.find("editor") != std::string::npos) {
            std::cout << "[EDITOR-" << pcb->pid << "] Text editing operations..." << std::endl;
        } else {
            std::cout << "[PROC-" << pcb->pid << "] Process running..." << std::endl;
        }
        
        // Random chance to terminate
        if (dis(gen) > 4500) {
            std::cout << "[PROCESS] Process " << pcb->pid << " completed execution" << std::endl;
            break;
        }
    }
    
    pcb->state = PROCESS_TERMINATED;
}

void ProcessManager::cleanup_process(ProcessControlBlock* pcb) {
    if (pcb->memory_base) {
        std::free(pcb->memory_base);
        pcb->memory_base = nullptr;
    }
    
    std::cout << "[PROCESS] Cleaned up process " << pcb->pid << std::endl;
}

void ProcessManager::schedule() {
    if (!scheduler_running) return;
    
    ProcessControlBlock* next_process = select_next_process();
    if (next_process && next_process != current_process) {
        context_switch(current_process, next_process);
    }
}

ProcessControlBlock* ProcessManager::select_next_process() {
    // Simple round-robin scheduler with priority
    ProcessControlBlock* best_process = nullptr;
    int highest_priority = -1;
    
    for (auto& pcb : process_table) {
        if (pcb->state == PROCESS_READY && pcb->priority > highest_priority) {
            best_process = pcb.get();
            highest_priority = pcb->priority;
        }
    }
    
    return best_process;
}

void ProcessManager::context_switch(ProcessControlBlock* old_proc, ProcessControlBlock* new_proc) {
    if (old_proc) {
        old_proc->state = PROCESS_READY;
    }
    
    if (new_proc) {
        current_process = new_proc;
        new_proc->state = PROCESS_RUNNING;
        // std::cout << "[SCHEDULER] Context switch to process " << new_proc->pid << std::endl;
    }
}

void ProcessManager::set_process_priority(int pid, int priority) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    auto it = pid_map.find(pid);
    if (it != pid_map.end()) {
        it->second->priority = priority;
        std::cout << "[PROCESS] Set priority " << priority << " for process " << pid << std::endl;
    }
}

ProcessControlBlock* ProcessManager::get_process(int pid) {
    auto it = pid_map.find(pid);
    return (it != pid_map.end()) ? it->second : nullptr;
}

std::vector<ProcessControlBlock*> ProcessManager::get_all_processes() {
    std::vector<ProcessControlBlock*> processes;
    for (auto& pcb : process_table) {
        processes.push_back(pcb.get());
    }
    return processes;
}

ProcessControlBlock* ProcessManager::get_current_process() {
    return current_process;
}

bool ProcessManager::send_signal(int pid, int signal) {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    auto it = pid_map.find(pid);
    if (it != pid_map.end()) {
        std::cout << "[PROCESS] Sending signal " << signal << " to process " << pid << std::endl;
        
        if (signal == 9) { // SIGKILL
            return terminate_process(pid);
        } else if (signal == 19) { // SIGSTOP
            return suspend_process(pid);
        } else if (signal == 18) { // SIGCONT
            return resume_process(pid);
        }
        
        return true;
    }
    return false;
}

bool ProcessManager::wait_for_process(int pid) {
    auto it = pid_map.find(pid);
    if (it != pid_map.end()) {
        ProcessControlBlock* pcb = it->second;
        if (pcb->process_thread && pcb->process_thread->joinable()) {
            pcb->process_thread->join();
            return true;
        }
    }
    return false;
}

void ProcessManager::print_process_table() {
    std::lock_guard<std::mutex> lock(process_mutex);
    
    std::cout << "[PROCESS] Process Table:" << std::endl;
    std::cout << "PID\tParent\tState\t\tCPU Time\tExecutable" << std::endl;
    
    for (auto& pcb : process_table) {
        std::string state_str;
        switch (pcb->state) {
            case PROCESS_READY: state_str = "READY"; break;
            case PROCESS_RUNNING: state_str = "RUNNING"; break;
            case PROCESS_BLOCKED: state_str = "BLOCKED"; break;
            case PROCESS_TERMINATED: state_str = "TERMINATED"; break;
        }
        
        std::cout << pcb->pid << "\t" << pcb->parent_pid << "\t" 
                  << state_str << "\t\t" << pcb->cpu_time << "ms\t\t" 
                  << pcb->executable_path << std::endl;
    }
}

size_t ProcessManager::get_process_count() {
    return process_table.size();
}
