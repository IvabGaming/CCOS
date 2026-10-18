#ifndef KERNEL_H
#define KERNEL_H

#include "disk.h"
#include "iostream.h"

extern "C" {
    void* memset(void* dest, int val, size_t count) {
        unsigned char* ptr = static_cast<unsigned char*>(dest);
        while (count--) {
            *ptr++ = static_cast<unsigned char>(val);
        }
        return dest;
    }

    void* memcpy(void* dest, const void* src, size_t count) {
        unsigned char* d = static_cast<unsigned char*>(dest);
        const unsigned char* s = static_cast<const unsigned char*>(src);
        while (count--) {
            *d++ = *s++;
        }
        return dest;
    }
}

// VGA Constants
inline uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;
constexpr size_t VGA_WIDTH = 80;
constexpr size_t VGA_HEIGHT = 25;

static size_t vga_row = 0;
static size_t vga_col = 0;
static uint8_t vga_color = 0x07; // Light gray on black

// Path & Filesystem Globals
constexpr char CCFS_PATH_SEP = ';';
static uint8_t current_dir_id = 0;
static char current_path[64] = ";";

// Basic C-string helpers
inline size_t kstrlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

inline int kstrcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

inline void kstrncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
}

inline void kstrcat(char* dest, const char* src) {
    size_t dlen = kstrlen(dest);
    size_t i = 0;
    while (src[i] != '\0') {
        dest[dlen + i] = src[i];
        i++;
    }
    dest[dlen + i] = '\0';
}

inline bool kstr_startswith(const char* str, const char* prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) return false;
    }
    return true;
}

// VGA Text Driver Implementation
inline void vga_putc(char c) {
    if (c == '\n') {
        vga_col = 0;
        if (++vga_row == VGA_HEIGHT) {
            for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
                for (size_t x = 0; x < VGA_WIDTH; x++) {
                    VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
                }
            }
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (vga_color << 8) | ' ';
            }
            vga_row = VGA_HEIGHT - 1;
        }
        return;
    }
    VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = (vga_color << 8) | c;
    if (++vga_col == VGA_WIDTH) {
        vga_col = 0;
        vga_putc('\n');
    }
}

inline void vga_puts(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) vga_putc(str[i]);
}

inline void vga_clear() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = (vga_color << 8) | ' ';
        }
    }
    vga_row = 0;
    vga_col = 0;
}

// Simple Heap Allocation Manager (kmalloc/kfree)
static uint8_t heap_memory[1024 * 1024]; // 1 MB heap space
static size_t heap_offset = 0;

inline void* kmalloc(size_t size) {
    size = (size + 7) & ~7; // 8-byte align
    if (heap_offset + size > sizeof(heap_memory)) return nullptr;
    void* ptr = &heap_memory[heap_offset];
    heap_offset += size;
    return ptr;
}

inline void kfree(void*) {
    // Basic bump allocator - space recycled on clear
}

// Global dynamic allocation overloads
void* operator new(size_t size) { return kmalloc(size); }
void* operator new[](size_t size) { return kmalloc(size); }
void operator delete(void* p) noexcept { kfree(p); }
void operator delete[](void* p) noexcept { kfree(p); }
void operator delete(void* p, size_t) noexcept { kfree(p); }
void operator delete[](void* p, size_t) noexcept { kfree(p); }

// Native x86 Dynamic JIT Execution Engine
typedef int (*CCOS_Executable_Fn)();

inline void execute_native_code(const uint8_t* code, size_t size) {
    uint8_t* exec_buf = static_cast<uint8_t*>(kmalloc(size));
    for (size_t i = 0; i < size; i++) exec_buf[i] = code[i];

    CCOS_Executable_Fn entry_point = reinterpret_cast<CCOS_Executable_Fn>(exec_buf);
    std::kcout << "[CCCPP] Executing dynamically compiled code...\n";
    int result = entry_point();
    std::kcout << "[CCCPP] Program exited with code: " << static_cast<int32_t>(result) << std::endl;
}

// Native Micro C++ Compiler Implementation (ccc)
enum TokenType { TOK_INT, TOK_IDENTIFIER, TOK_NUMBER, TOK_ASSIGN, TOK_PLUS, TOK_MINUS, TOK_RETURN, TOK_SEMICOLON, TOK_EOF };

struct Token {
    TokenType type;
    char text[32];
    int value;
};

class Lexer {
private:
    const char* src;
    size_t pos;
public:
    explicit Lexer(const char* source) : src(source), pos(0) {}

    Token next_token() {
        while (src[pos] == ' ' || src[pos] == '\n' || src[pos] == '\r') pos++;
        if (src[pos] == '\0') return { TOK_EOF, "", 0 };

        if (src[pos] >= '0' && src[pos] <= '9') {
            Token t{ TOK_NUMBER, "", 0 };
            while (src[pos] >= '0' && src[pos] <= '9') {
                t.value = t.value * 10 + (src[pos] - '0');
                pos++;
            }
            return t;
        }

        if ((src[pos] >= 'a' && src[pos] <= 'z') || (src[pos] >= 'A' && src[pos] <= 'Z')) {
            Token t{ TOK_IDENTIFIER, "", 0 };
            int idx = 0;
            while ((src[pos] >= 'a' && src[pos] <= 'z') || (src[pos] >= '0' && src[pos] <= '9') || src[pos] == '_') {
                if (idx < 31) t.text[idx++] = src[pos];
                pos++;
            }
            t.text[idx] = '\0';
            if (kstrcmp(t.text, "return") == 0) t.type = TOK_RETURN;
            if (kstrcmp(t.text, "int") == 0) t.type = TOK_INT;
            return t;
        }

        if (src[pos] == '=') { pos++; return { TOK_ASSIGN, "=", 0 }; }
        if (src[pos] == '+') { pos++; return { TOK_PLUS, "+", 0 }; }
        if (src[pos] == '-') { pos++; return { TOK_MINUS, "-", 0 }; }
        if (src[pos] == ';') { pos++; return { TOK_SEMICOLON, ";", 0 }; }

        pos++;
        return next_token();
    }
};

class X86Emitter {
public:
    uint8_t buffer[256];
    size_t size;

    X86Emitter() : size(0) {}

    void emit_byte(uint8_t b) { buffer[size++] = b; }
    void emit_int32(uint32_t val) {
        emit_byte(val & 0xFF);
        emit_byte((val >> 8) & 0xFF);
        emit_byte((val >> 16) & 0xFF);
        emit_byte((val >> 24) & 0xFF);
    }

    void emit_prologue() {
        emit_byte(0x55);                    // push ebp
        emit_byte(0x89); emit_byte(0xE5);   // mov ebp, esp
    }

    void emit_mov_eax_imm(uint32_t val) {
        emit_byte(0xB8);
        emit_int32(val);
    }

    void emit_add_eax_imm(uint32_t val) {
        emit_byte(0x05);
        emit_int32(val);
    }

    void emit_ret() {
        emit_byte(0x5D); // pop ebp
        emit_byte(0xC3); // ret
    }
};

inline void ccfs_delete(const char* target) {
    uint8_t sector_buf[512];
    ata_read_sector(0, sector_buf);
    auto* dir = reinterpret_cast<DiskDirectorySector*>(sector_buf);

    bool found = false;
    for (int i = 0; i < 16; i++) {
        if (dir->files[i].used == 1 &&
            dir->files[i].parent_id == current_dir_id &&
            kstrcmp(dir->files[i].name, target) == 0) {
            
            // Clear the directory entry slot
            dir->files[i].used = 0;
            dir->files[i].byte_size = 0;
            dir->files[i].start_sector = 0;
            dir->files[i].name[0] = '\0';

            ata_write_sector(0, sector_buf);
            std::kcout << "Deleted: " << target << std::endl;
            found = true;
            break;
        }
    }

    if (!found) {
        std::kcout << "delete: file or directory not found: " << target << std::endl;
    }
}

inline void ccfs_mkdir(const char* path) {
    uint8_t sector_buf[512];
    ata_read_sector(0, sector_buf);
    auto* dir = reinterpret_cast<DiskDirectorySector*>(sector_buf);

    // Make a mutable copy of the path to tokenize
    char path_copy[64];
    kstrncpy(path_copy, path, 63);

    uint8_t target_parent = current_dir_id;
    char* token = path_copy;
    char* rest = path_copy;

    // Simple tokenizer for CCFS_PATH_SEP (';')
    while (*rest != '\0') {
        char* next_sep = rest;
        while (*next_sep != '\0' && *next_sep != CCFS_PATH_SEP) {
            next_sep++;
        }
        
        if (*next_sep == CCFS_PATH_SEP) {
            *next_sep = '\0';
            token = rest;
            rest = next_sep + 1;

            // We have an intermediate directory in the path, find or traverse it
            bool found_dir = false;
            for (int i = 0; i < 16; i++) {
                if (dir->files[i].used == 1 && 
                    (dir->files[i].flags & CCFS_ATTR_DIR) &&
                    dir->files[i].parent_id == target_parent &&
                    kstrcmp(dir->files[i].name, token) == 0) {
                    target_parent = i;
                    found_dir = true;
                    break;
                }
            }
            if (!found_dir) {
                std::kcout << "mkdir: parent directory not found: " << token << "\n";
                return;
            }
        } else {
            // Final component of the path to create
            token = rest;
            break;
        }
    }

    // Check if directory already exists in the target parent
    for (int i = 0; i < 16; i++) {
        if (dir->files[i].used == 1 && 
            dir->files[i].parent_id == target_parent &&
            kstrcmp(dir->files[i].name, token) == 0) {
            std::kcout << "mkdir: directory already exists: " << token << "\n";
            return;
        }
    }

    // Find an empty slot to create the new directory
    for (int i = 1; i < 16; i++) {
        if (dir->files[i].used == 0) {
            kstrncpy(dir->files[i].name, token, 23);
            dir->files[i].start_sector = 0;
            dir->files[i].byte_size = 0;
            dir->files[i].flags = CCFS_ATTR_DIR;
            dir->files[i].parent_id = target_parent;
            dir->files[i].used = 1;

            ata_write_sector(0, sector_buf);
            std::kcout << "Created directory: " << token << std::endl;
            return;
        }
    }
    std::kcout << "Error: Directory index table full.\n";
}

inline void ccfs_cd(const char* target) {
    if (kstrcmp(target, "..") == 0) {
        current_dir_id = 0;
        current_path[0] = CCFS_PATH_SEP;
        current_path[1] = '\0';
        return;
    }

    uint8_t sector_buf[512];
    ata_read_sector(0, sector_buf);
    auto* dir = reinterpret_cast<DiskDirectorySector*>(sector_buf);

    for (int i = 0; i < 16; i++) {
        if (dir->files[i].used == 1 && 
            (dir->files[i].flags & CCFS_ATTR_DIR) &&
            dir->files[i].parent_id == current_dir_id &&
            kstrcmp(dir->files[i].name, target) == 0) {
            
            current_dir_id = i;
            if (kstrcmp(current_path, ";") != 0) {
                size_t len = kstrlen(current_path);
                current_path[len] = CCFS_PATH_SEP;
                current_path[len + 1] = '\0';
            }
            kstrcat(current_path, target);
            return;
        }
    }
    std::kcout << "cd: no such directory: " << target << std::endl;
}

inline void ccfs_ls() {
    uint8_t sector_buf[512];
    ata_read_sector(0, sector_buf);
    auto* dir = reinterpret_cast<DiskDirectorySector*>(sector_buf);

    std::kcout << "Directory contents of " << current_path << ":\n";
    for (int i = 0; i < 16; i++) {
        if (dir->files[i].used == 1 && dir->files[i].parent_id == current_dir_id) {
            if (dir->files[i].flags & CCFS_ATTR_DIR) {
                std::kcout << "  [DIR]  " << dir->files[i].name << "\n";
            } else {
                std::kcout << "  [FILE] " << dir->files[i].name 
                          << " (" << dir->files[i].byte_size << " bytes)\n";
            }
        }
    }
}

inline void ccfs_write(const char* filename, const char* content) {
    uint8_t sector_buf[512];
    ata_read_sector(0, sector_buf);
    auto* dir = reinterpret_cast<DiskDirectorySector*>(sector_buf);

    int target_slot = -1;
    uint32_t next_sector = 1;

    for (int i = 1; i < 16; i++) {
        if (dir->files[i].used == 1) {
            uint32_t end_sec = dir->files[i].start_sector + ((dir->files[i].byte_size + 511) / 512);
            if (end_sec > next_sector) next_sector = end_sec;
        } else if (target_slot == -1) {
            target_slot = i;
        }
    }

    if (target_slot == -1) {
        std::kcout << "dwrite: directory full.\n";
        return;
    }

    size_t size = kstrlen(content);
    uint8_t data_buf[512] = {0};
    for (size_t i = 0; i < size && i < 512; i++) data_buf[i] = content[i];

    ata_write_sector(next_sector, data_buf);

    kstrncpy(dir->files[target_slot].name, filename, 23);
    dir->files[target_slot].start_sector = next_sector;
    dir->files[target_slot].byte_size = size;
    dir->files[target_slot].flags = CCFS_ATTR_FILE;
    dir->files[target_slot].parent_id = current_dir_id;
    dir->files[target_slot].used = 1;

    ata_write_sector(0, sector_buf);
    std::kcout << "Saved " << filename << " to CCFS sector " << next_sector << std::endl;
}

inline void ccfs_cat(const char* filename) {
    uint8_t sector_buf[512];
    ata_read_sector(0, sector_buf);
    auto* dir = reinterpret_cast<DiskDirectorySector*>(sector_buf);

    for (int i = 0; i < 16; i++) {
        if (dir->files[i].used == 1 &&
            (dir->files[i].flags == CCFS_ATTR_FILE) &&
            dir->files[i].parent_id == current_dir_id &&
            kstrcmp(dir->files[i].name, filename) == 0) {

            uint8_t data_buf[512] = {0};
            ata_read_sector(dir->files[i].start_sector, data_buf);
            data_buf[dir->files[i].byte_size < 511 ? dir->files[i].byte_size : 511] = '\0';
            std::kcout << reinterpret_cast<char*>(data_buf) << std::endl;
            return;
        }
    }
    std::kcout << "dcat: file not found.\n";
}

inline void echo(const char* text) {
    std::kcout << text << std::endl;
}

// PS/2 Keyboard Polling
inline char keyboard_poll() {
    static bool shift_pressed = false;

    static const char scancode_ascii[] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
      '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0,  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
       '*',   0, ' '
    };

    static const char scancode_shift_ascii[] = {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
      '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
        0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
       '*',   0, ' '
    };

    if (inb(0x64) & 1) {
        uint8_t scancode = inb(0x60);

        // Detect Left Shift (0x2A) or Right Shift (0x36) press
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = true;
            return 0;
        }
        // Detect Left Shift release (0xAA) or Right Shift release (0xB6)
        if (scancode == 0xAA || scancode == 0xB6) {
            shift_pressed = false;
            return 0;
        }

        // Ignore other break codes (when bit 7 is set)
        if (!(scancode & 0x80) && scancode < sizeof(scancode_ascii)) {
            return shift_pressed ? scancode_shift_ascii[scancode] : scancode_ascii[scancode];
        }
    }
    return 0;
}

// Shell Processing
inline void parse_and_execute(const char* cmd) {
    using std::kcout;
    using std::endl;

    if (kstrcmp(cmd, "help") == 0) {
        kcout << "CCOS Shell Commands:\n";
        kcout << "  help, version, clear, whoami, shutdown\n";
        kcout << "  mkdir [dir], cd [dir], dls, dwrite [file] [text], dcat [file], del [name],\n echo [text]\n";
        kcout << "  ccc [file]  - Compile and execute C++ file from CCFS\n";
    } else if (kstrcmp(cmd, "version") == 0) {
        kcout << "CCOS Kernel v1.2.0 (32-bit x86, CCFS hierarchical filesystem)\n";
    } else if (kstrcmp(cmd, "clear") == 0) {
        vga_clear();
    } else if (kstrcmp(cmd, "whoami") == 0) {
        kcout << "idk who u r, im just a dumb kernel boi\n";
    } else if (kstr_startswith(cmd, "mkdir ")) {
        ccfs_mkdir(cmd + 6);
    } else if (kstr_startswith(cmd, "cd ")) {
        ccfs_cd(cmd + 3);
    } else if (kstrcmp(cmd, "dls") == 0) {
        ccfs_ls();
    } else if (kstr_startswith(cmd, "dwrite ")) {
        const char* args = cmd + 7;
        char filename[24] = {0};
        int idx = 0;
        while (*args && *args != ' ' && idx < 23) filename[idx++] = *args++;
        if (*args == ' ') args++;
        ccfs_write(filename, args);
    } else if (kstr_startswith(cmd, "dcat ")) {
        ccfs_cat(cmd + 5);
    } else if (kstr_startswith(cmd, "ccc ")) {
        const char* filename = cmd + 4;
        uint8_t sector_buf[512];
        ata_read_sector(0, sector_buf);
        auto* dir = reinterpret_cast<DiskDirectorySector*>(sector_buf);

        bool found = false;
        for (int i = 0; i < 16; i++) {
            if (dir->files[i].used == 1 && kstrcmp(dir->files[i].name, filename) == 0) {
                uint8_t data_buf[512] = {0};
                ata_read_sector(dir->files[i].start_sector, data_buf);

                Lexer lexer(reinterpret_cast<char*>(data_buf));
                X86Emitter emitter;
                emitter.emit_prologue();

                Token tok = lexer.next_token();
                if (tok.type == TOK_INT) {
                    lexer.next_token(); // name
                    if (lexer.next_token().type == TOK_RETURN) {
                        Token num1 = lexer.next_token();
                        emitter.emit_mov_eax_imm(num1.value);
                        Token op = lexer.next_token();
                        if (op.type == TOK_PLUS) {
                            Token num2 = lexer.next_token();
                            emitter.emit_add_eax_imm(num2.value);
                        }
                    }
                }
                emitter.emit_ret();
                execute_native_code(emitter.buffer, emitter.size);
                found = true;
                break;
            }
        }
        if (!found) kcout << "ccc: file not found.\n";
    } else if (kstrcmp(cmd, "shutdown") == 0) {
        outw(0x604, 0x2000); // QEMU ACPI Shutdown
    } else if (kstr_startswith(cmd, "del ")) {
        ccfs_delete(cmd + 4);
    } else if (kstrcmp(cmd, ":-)") == 0) {
        std::kcout << "Happy! You found my easter egg!\nYay! :-D\n" << std::endl;
    } else if (kstr_startswith(cmd, "echo ")) {
        echo(cmd + 4);
    } else if (kstrlen(cmd) > 0) {
        kcout << "Unknown command: " << cmd << "\n";
    }
}

extern "C" void kernel_main() {
    vga_clear();
    std::kcout << "=======================================\n";
    std::kcout << "  CCOS Bare-Metal Environment (x86)\n";
    std::kcout << "=======================================\n\n";

    char input_buffer[128];
    size_t input_pos = 0;

    std::kcout << "ccos " << current_path << "-$ ";

    while (true) {
        char c = keyboard_poll();
        if (c != 0) {
            if (c == '\n') {
                vga_putc('\n');
                input_buffer[input_pos] = '\0';
                parse_and_execute(input_buffer);
                input_pos = 0;
                std::kcout << "ccos " << current_path << "-$ ";
            } else if (c == '\b') {
                if (input_pos > 0) {
                    input_pos--;
                    vga_col--;
                    vga_putc(' ');
                    vga_col--;
                }
            } else if (input_pos < sizeof(input_buffer) - 1) {
                input_buffer[input_pos++] = c;
                vga_putc(c);
            }
        }
    }
}

#endif