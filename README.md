# CCOS (C++ Custom Operating System)

CCOS is a bare-metal, 32-bit x86 interactive operating system kernel developed in C++17 and assembly. Built from scratch in a freestanding environment without stdlib or runtime overhead, it features custom dynamic memory management, interactive VGA text rendering, raw ATA hard disk persistence, and a built-in shell interface.

---

## Key Features

- **Freestanding C++17 Runtime:** Custom runtime without standard library support (`-ffreestanding`, `-fno-exceptions`, `-fno-rtti`).
- **Memory Allocation Engine:** Custom heap manager (`kmalloc` / `kfree`) with dynamic 8-byte block alignment and coalescing, plus overloaded global `new` and `delete` operators.
- **Hardware Abstraction Drivers:**
  - **VGA Text Driver:** Directly manipulates `0xB8000` text memory with dynamic color attribute customization (`col [color]`).
  - **PS/2 Keyboard Driver:** Scancode polling via I/O port `0x60` and `0x64`.
  - **ATA IDE Primary Bus Controller:** Raw sector read/write access (`0x1F0`–`0x1F7`) supporting a 512 MB hard disk (`disk.img`).
  - **Power Management:** ACPI-based QEMU system shutdown.
- **Persistent Hard Disk Filesystem:** Custom block-structured index directory stored on disk Sector 0 for persistent file operations (`dls`, `dwrite`, `dcat`).
- **Interactive Shell CLI:** Full command parsing loop featuring Unix-style prompt (`ccos -$ `), system utilities, memory test routines, and visual effects (Matrix rain).

---

## Project Structure

```text
├── boot.s          # x86 Multiboot assembly header & kernel entry point
├── kernel.h        # Freestanding types, I/O wrappers, heap, VGA, & shell loop
├── disk.h          # ATA IDE drive controller & 512 MB raw filesystem drivers
├── linker.ld       # Executable linking layout starting at 1 MB
└── Makefile        # Build pipeline & QEMU launcher automation
```

---

## Prerequisites (macOS Setup)

Ensure you have installed the cross-compilation toolchain and emulator:

```bash
brew install llvm binutils qemu
```

*(Note: Linker commands target `x86_64-elf-ld` provided via Homebrew binutils).*

---

## Build & Execution

The included `Makefile` handles cross-compiling objects, linking `ccos.bin`, generating a zeroed 512 MB `disk.img`, and launching QEMU.

### 1. Build Kernel & Disk Image
```bash
make
```

### 2. Run in Standard Windowed Mode
```bash
make run
```

### 3. Run in Terminal Mode (Curses)
```bash
make tty
```

### 4. Cleaning Build Artifacts
```bash
# Clean binary and object files (preserves disk.img data)
make clean

# Full reset (erases binaries AND clears disk.img)
make clean-all
```

---

## Shell Command Reference

| Command | Usage / Syntax | Description |
| :--- | :--- | :--- |
| **`help`** | `help` | Lists all available shell commands. |
| **`version`** | `version` | Displays CCOS kernel release and shell build info. |
| **`whoami`** | `whoami` | Displays identity statement. |
| **`clear`** | `clear` | Clears VGA text memory (`0xB8000`) and resets cursor. |
| **`col`** | `col <color>` | Sets text foreground color (*options: red, green, blue, cyan, yellow, magenta, white, gray*). |
| **`alloc_test`** | `alloc_test` | Allocates and frees dynamic `KString` objects via `kmalloc` / `kfree` to test heap sanity. |
| **`dls`** | `dls` | Lists files stored on the 512 MB ATA hard disk along with starting sector offsets. |
| **`dwrite`** | `dwrite <filename> <text>` | Saves a file directly to sectors on `disk.img`. |
| **`dcat`** | `dcat <filename>` | Reads and displays content stored on the target hard disk sector. |
| **`matrix`** | `matrix` | Runs digital rain visual animation (*press any key to exit*). |
| **`shutdown`** | `shutdown` | Sends ACPI power-off signal to shutdown QEMU emulator. |

---

## License

This project is open-source under the MIT License.