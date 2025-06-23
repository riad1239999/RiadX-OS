#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <map>

// Memory management constants
#define PAGE_SIZE 4096
#define MEMORY_POOL_SIZE (1024 * 1024 * 16) // 16MB virtual memory pool

struct MemoryBlock {
    void* address;
    size_t size;
    bool is_free;
    int process_id;
};

struct PageTableEntry {
    uint64_t physical_address : 40;
    uint64_t present : 1;
    uint64_t writable : 1;
    uint64_t user_accessible : 1;
    uint64_t write_through : 1;
    uint64_t cache_disabled : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t page_size : 1;
    uint64_t global : 1;
    uint64_t available : 3;
    uint64_t reserved : 12;
};

class MemoryManager {
private:
    std::vector<MemoryBlock> memory_blocks;
    std::map<void*, size_t> allocated_blocks;
    std::mutex memory_mutex;
    
    uint8_t* memory_pool;
    size_t pool_size;
    size_t next_free_offset;
    
    // Virtual memory management
    std::vector<PageTableEntry> page_table;
    uint64_t next_virtual_address;
    
    // Memory allocation algorithms
    void* first_fit_allocate(size_t size);
    void* best_fit_allocate(size_t size);
    void coalesce_free_blocks();
    
    // Paging functions
    uint64_t virtual_to_physical(uint64_t virtual_addr);
    bool map_page(uint64_t virtual_addr, uint64_t physical_addr);
    void unmap_page(uint64_t virtual_addr);

public:
    MemoryManager();
    ~MemoryManager();
    
    bool initialize();
    void shutdown();
    
    // Memory allocation
    void* allocate(size_t size);
    void* allocate_aligned(size_t size, size_t alignment);
    void deallocate(void* ptr);
    
    // Memory information
    size_t get_total_memory();
    size_t get_free_memory();
    size_t get_used_memory();
    
    // Process memory management
    void* allocate_for_process(int process_id, size_t size);
    void deallocate_process_memory(int process_id);
    
    // Virtual memory management
    void* allocate_virtual_page();
    void free_virtual_page(void* page);
    bool protect_memory(void* ptr, size_t size, int protection);
    
    // Debug functions
    void print_memory_map();
    bool validate_pointer(void* ptr);
};

#endif
