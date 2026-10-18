/* boot.s - Multiboot header and kernel initialization */
.set ALIGN,    1<<0             /* Align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* Provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* Multiboot flags */
.set MAGIC,    0x1BADB002       /* Magic number for bootloader */
.set CHECKSUM, -(MAGIC + FLAGS) /* Checksum required */

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16 KiB Kernel Stack */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp
	call kernel_main
	cli
1:	hlt
	jmp 1b