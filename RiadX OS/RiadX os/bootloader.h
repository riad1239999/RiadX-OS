#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <string>
#include <vector>
#include <cstdint>

// Boot sector constants
#define BOOT_SECTOR_SIZE 512
#define BOOT_SIGNATURE 0xAA55
#define KERNEL_LOAD_ADDRESS 0x100000  // 1MB
#define STACK_ADDRESS 0x90000         // 576KB

// System information structure
struct SystemInfo {
    uint32_t memory_size;
    uint32_t memory_map_entries;
    uint16_t video_mode;
    uint32_t kernel_size;
    uint32_t initrd_size;
    char bootloader_name[32];
    char kernel_cmdline[256];
};

// Memory map entry
struct MemoryMapEntry {
    uint64_t base_address;
    uint64_t length;
    uint32_t type;          // 1=Available, 2=Reserved, 3=ACPI Reclaimable, 4=ACPI NVS
    uint32_t attributes;
};

// Boot stages
enum BootStage {
    STAGE_INIT,
    STAGE_MEMORY_DETECT,
    STAGE_LOAD_KERNEL,
    STAGE_SETUP_GDT,
    STAGE_ENABLE_A20,
    STAGE_ENTER_PROTECTED_MODE,
    STAGE_JUMP_TO_KERNEL,
    STAGE_COMPLETE
};

class Bootloader {
private:
    SystemInfo system_info;
    std::vector<MemoryMapEntry> memory_map;
    BootStage current_stage;
    bool verbose_output;
    
    // Boot process functions
    bool initialize_hardware();
    bool detect_memory();
    bool setup_gdt();
    bool enable_a20_line();
    bool load_kernel_image();
    bool setup_protected_mode();
    bool validate_kernel();
    
    // Memory management
    bool create_memory_map();
    uint32_t get_total_memory();
    uint32_t find_free_memory_block(uint32_t size);
    
    // Hardware detection
    bool detect_cpu_features();
    bool setup_video_mode();
    bool initialize_disk_subsystem();
    
    // Kernel loading
    bool read_kernel_from_disk();
    bool decompress_kernel();
    bool relocate_kernel();
    bool setup_kernel_parameters();
    
    // Error handling
    void panic(const std::string& message);
    void print_error(const std::string& error);
    void print_status(const std::string& status);
    
    // Utility functions
    void delay(uint32_t milliseconds);
    bool verify_checksum(void* data, size_t size);
    void setup_interrupt_handlers();

public:
    Bootloader();
    ~Bootloader();
    
    // Main boot sequence
    bool boot();
    
    // Configuration
    void set_verbose(bool verbose) { verbose_output = verbose; }
    void set_kernel_cmdline(const std::string& cmdline);
    
    // System information
    const SystemInfo& get_system_info() const { return system_info; }
    const std::vector<MemoryMapEntry>& get_memory_map() const { return memory_map; }
    BootStage get_current_stage() const { return current_stage; }
    
    // Debugging
    void dump_system_info();
    void dump_memory_map();
    void print_boot_progress();
    
    // Low-level hardware interface (implemented in assembly)
    static void enable_interrupts();
    static void disable_interrupts();
    static uint8_t inb(uint16_t port);
    static void outb(uint16_t port, uint8_t value);
    static uint16_t inw(uint16_t port);
    static void outw(uint16_t port, uint16_t value);
    static uint32_t ind(uint16_t port);
    static void outd(uint16_t port, uint32_t value);
    
    // Memory operations
    static void* memcpy_boot(void* dest, const void* src, size_t count);
    static void* memset_boot(void* dest, int value, size_t count);
    static int memcmp_boot(const void* buf1, const void* buf2, size_t count);
    
    // String operations
    static size_t strlen_boot(const char* str);
    static char* strcpy_boot(char* dest, const char* src);
    static int strcmp_boot(const char* str1, const char* str2);
};

// Global Descriptor Table structures
struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct GDTPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Interrupt Descriptor Table structures
struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Boot sector structure
struct BootSector {
    uint8_t jump_instruction[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_serial;
    char volume_label[11];
    char file_system[8];
    uint8_t boot_code[448];
    uint16_t boot_sector_signature;
} __attribute__((packed));

// Multiboot header for kernel compatibility
struct MultibootHeader {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
} __attribute__((packed));

// Function prototypes for assembly routines
extern "C" {
    void boot_main();
    void switch_to_protected_mode();
    void jump_to_kernel(uint32_t kernel_entry);
    void setup_gdt_asm();
    void load_idt(IDTPointer* idt_ptr);
    void enable_a20();
    uint32_t detect_memory_e820();
    void halt_system();
}

#endif
