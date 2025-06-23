#include "memory.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdlib>

MemoryManager::MemoryManager() 
    : memory_pool(nullptr), pool_size(MEMORY_POOL_SIZE), 
      next_free_offset(0), next_virtual_address(0x1000000) {
    std::cout << "[MEMORY] Memory manager initializing..." << std::endl;
}

MemoryManager::~MemoryManager() {
    shutdown();
}

bool MemoryManager::initialize() {
    std::lock_guard<std::mutex> lock(memory_mutex);
    
    try {
        // Allocate memory pool
        memory_pool = static_cast<uint8_t*>(std::malloc(pool_size));
        if (!memory_pool) {
            std::cerr << "[MEMORY] Failed to allocate memory pool" << std::endl;
            return false;
        }
        
        // Initialize memory pool
        std::memset(memory_pool, 0, pool_size);
        
        // Create initial free block
        MemoryBlock initial_block;
        initial_block.address = memory_pool;
        initial_block.size = pool_size;
        initial_block.is_free = true;
        initial_block.process_id = -1;
        memory_blocks.push_back(initial_block);
        
        // Initialize page table
        page_table.resize(1024); // 1024 pages
        for (auto& entry : page_table) {
            entry.present = 0;
            entry.writable = 1;
            entry.user_accessible = 1;
        }
        
        std::cout << "[MEMORY] Memory manager initialized with " 
                  << (pool_size / 1024) << "KB memory pool" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[MEMORY] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void MemoryManager::shutdown() {
    std::lock_guard<std::mutex> lock(memory_mutex);
    
    if (memory_pool) {
        std::free(memory_pool);
        memory_pool = nullptr;
    }
    
    memory_blocks.clear();
    allocated_blocks.clear();
    page_table.clear();
    
    std::cout << "[MEMORY] Memory manager shutdown complete" << std::endl;
}

void* MemoryManager::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(memory_mutex);
    
    if (size == 0) return nullptr;
    
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    
    void* ptr = first_fit_allocate(size);
    if (ptr) {
        allocated_blocks[ptr] = size;
        std::cout << "[MEMORY] Allocated " << size << " bytes at " << ptr << std::endl;
    } else {
        std::cerr << "[MEMORY] Failed to allocate " << size << " bytes" << std::endl;
    }
    
    return ptr;
}

void* MemoryManager::allocate_aligned(size_t size, size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return nullptr; // Alignment must be power of 2
    }
    
    size_t aligned_size = size + alignment - 1;
    void* ptr = allocate(aligned_size);
    
    if (ptr) {
        // Calculate aligned address
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<void*>(aligned_addr);
    }
    
    return nullptr;
}

void MemoryManager::deallocate(void* ptr) {
    std::lock_guard<std::mutex> lock(memory_mutex);
    
    if (!ptr) return;
    
    auto it = allocated_blocks.find(ptr);
    if (it == allocated_blocks.end()) {
        std::cerr << "[MEMORY] Attempting to free unallocated pointer: " << ptr << std::endl;
        return;
    }
    
    size_t size = it->second;
    allocated_blocks.erase(it);
    
    // Find the memory block and mark as free
    for (auto& block : memory_blocks) {
        if (block.address == ptr) {
            block.is_free = true;
            block.process_id = -1;
            std::cout << "[MEMORY] Freed " << size << " bytes at " << ptr << std::endl;
            break;
        }
    }
    
    // Coalesce adjacent free blocks
    coalesce_free_blocks();
}

void* MemoryManager::first_fit_allocate(size_t size) {
    for (auto& block : memory_blocks) {
        if (block.is_free && block.size >= size) {
            if (block.size > size) {
                // Split the block
                MemoryBlock new_block;
                new_block.address = static_cast<uint8_t*>(block.address) + size;
                new_block.size = block.size - size;
                new_block.is_free = true;
                new_block.process_id = -1;
                memory_blocks.push_back(new_block);
            }
            
            block.size = size;
            block.is_free = false;
            return block.address;
        }
    }
    return nullptr;
}

void* MemoryManager::best_fit_allocate(size_t size) {
    MemoryBlock* best_block = nullptr;
    size_t best_size = SIZE_MAX;
    
    for (auto& block : memory_blocks) {
        if (block.is_free && block.size >= size && block.size < best_size) {
            best_block = &block;
            best_size = block.size;
        }
    }
    
    if (best_block) {
        if (best_block->size > size) {
            // Split the block
            MemoryBlock new_block;
            new_block.address = static_cast<uint8_t*>(best_block->address) + size;
            new_block.size = best_block->size - size;
            new_block.is_free = true;
            new_block.process_id = -1;
            memory_blocks.push_back(new_block);
        }
        
        best_block->size = size;
        best_block->is_free = false;
        return best_block->address;
    }
    
    return nullptr;
}

void MemoryManager::coalesce_free_blocks() {
    // Sort blocks by address
    std::sort(memory_blocks.begin(), memory_blocks.end(), 
              [](const MemoryBlock& a, const MemoryBlock& b) {
                  return a.address < b.address;
              });
    
    // Merge adjacent free blocks
    for (size_t i = 0; i < memory_blocks.size() - 1; ) {
        if (memory_blocks[i].is_free && memory_blocks[i + 1].is_free) {
            uint8_t* end_of_current = static_cast<uint8_t*>(memory_blocks[i].address) + memory_blocks[i].size;
            if (end_of_current == memory_blocks[i + 1].address) {
                // Merge blocks
                memory_blocks[i].size += memory_blocks[i + 1].size;
                memory_blocks.erase(memory_blocks.begin() + i + 1);
                continue;
            }
        }
        i++;
    }
}

size_t MemoryManager::get_total_memory() {
    return pool_size;
}

size_t MemoryManager::get_free_memory() {
    std::lock_guard<std::mutex> lock(memory_mutex);
    
    size_t free_memory = 0;
    for (const auto& block : memory_blocks) {
        if (block.is_free) {
            free_memory += block.size;
        }
    }
    return free_memory;
}

size_t MemoryManager::get_used_memory() {
    return get_total_memory() - get_free_memory();
}

void* MemoryManager::allocate_for_process(int process_id, size_t size) {
    void* ptr = allocate(size);
    if (ptr) {
        // Mark memory as belonging to process
        for (auto& block : memory_blocks) {
            if (block.address == ptr) {
                block.process_id = process_id;
                break;
            }
        }
    }
    return ptr;
}

void MemoryManager::deallocate_process_memory(int process_id) {
    std::lock_guard<std::mutex> lock(memory_mutex);
    
    for (auto& block : memory_blocks) {
        if (block.process_id == process_id && !block.is_free) {
            block.is_free = true;
            block.process_id = -1;
        }
    }
    
    coalesce_free_blocks();
    std::cout << "[MEMORY] Deallocated all memory for process " << process_id << std::endl;
}

void* MemoryManager::allocate_virtual_page() {
    void* page = allocate_aligned(PAGE_SIZE, PAGE_SIZE);
    if (page) {
        uint64_t virtual_addr = next_virtual_address;
        next_virtual_address += PAGE_SIZE;
        
        // Map virtual to physical
        map_page(virtual_addr, reinterpret_cast<uint64_t>(page));
        return reinterpret_cast<void*>(virtual_addr);
    }
    return nullptr;
}

void MemoryManager::free_virtual_page(void* page) {
    uint64_t virtual_addr = reinterpret_cast<uint64_t>(page);
    uint64_t physical_addr = virtual_to_physical(virtual_addr);
    
    if (physical_addr) {
        unmap_page(virtual_addr);
        deallocate(reinterpret_cast<void*>(physical_addr));
    }
}

bool MemoryManager::map_page(uint64_t virtual_addr, uint64_t physical_addr) {
    size_t page_index = (virtual_addr - 0x1000000) / PAGE_SIZE;
    if (page_index < page_table.size()) {
        page_table[page_index].physical_address = physical_addr >> 12;
        page_table[page_index].present = 1;
        return true;
    }
    return false;
}

void MemoryManager::unmap_page(uint64_t virtual_addr) {
    size_t page_index = (virtual_addr - 0x1000000) / PAGE_SIZE;
    if (page_index < page_table.size()) {
        page_table[page_index].present = 0;
        page_table[page_index].physical_address = 0;
    }
}

uint64_t MemoryManager::virtual_to_physical(uint64_t virtual_addr) {
    size_t page_index = (virtual_addr - 0x1000000) / PAGE_SIZE;
    if (page_index < page_table.size() && page_table[page_index].present) {
        uint64_t page_offset = virtual_addr & (PAGE_SIZE - 1);
        return (page_table[page_index].physical_address << 12) | page_offset;
    }
    return 0;
}

bool MemoryManager::protect_memory(void* ptr, size_t size, int protection) {
    // Implement memory protection
    std::cout << "[MEMORY] Setting protection " << protection << " for " 
              << size << " bytes at " << ptr << std::endl;
    return true;
}

void MemoryManager::print_memory_map() {
    std::lock_guard<std::mutex> lock(memory_mutex);
    
    std::cout << "[MEMORY] Memory Map:" << std::endl;
    std::cout << "Total: " << (get_total_memory() / 1024) << "KB, "
              << "Used: " << (get_used_memory() / 1024) << "KB, "
              << "Free: " << (get_free_memory() / 1024) << "KB" << std::endl;
    
    for (const auto& block : memory_blocks) {
        std::cout << "  Block: " << block.address 
                  << " Size: " << block.size 
                  << " Free: " << (block.is_free ? "Yes" : "No")
                  << " PID: " << block.process_id << std::endl;
    }
}

bool MemoryManager::validate_pointer(void* ptr) {
    if (!ptr) return false;
    
    uint8_t* addr = static_cast<uint8_t*>(ptr);
    return (addr >= memory_pool && addr < memory_pool + pool_size);
}
