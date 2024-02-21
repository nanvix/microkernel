/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/* Must come first. */
#define _ASM_FILE_

/*============================================================================*
 * Imported Symbols                                                           *
 *============================================================================*/

#include <grub/mboot2.h>
#include <arch/x86.h>
#include <asm/x86.S>

.extern kmain

/*============================================================================*
 * Exported Symbols                                                           *
 *============================================================================*/

.globl _do_start
.globl kstack

/*============================================================================*
 * Bootstrap Section                                                          *
 *============================================================================*/

.section .bootstrap,"ax",@progbits

/*----------------------------------------------------------------------------*
 * mboot_header                                                           *
 *----------------------------------------------------------------------------*/

/*
 *  Multiboot2 header.
 */
.align 8
mboot_header:
        .long   MULTIBOOT2_HEADER_MAGIC /*  Magic number. */
        .long  MBOOT_ARCHITECTURE_I386 /* Architecture */
        .long   mboot_header_end - mboot_header /* Header Length */
        /*  checksum */
        .long   -(MULTIBOOT2_HEADER_MAGIC + MBOOT_ARCHITECTURE_I386 + (mboot_header_end - mboot_header))
        framebuffer_tag_end:
        .short MBOOT_HEADER_TAG_END
        .short 0
        .long 8
mboot_header_end:

/*----------------------------------------------------------------------------*
 * _do_start()                                                                *
 *----------------------------------------------------------------------------*/

/*
 * Entry point.
 */
.align 4
_do_start:

    /* Reset stack. */
    reset_stack

    /* Save multiboot parameters on the stack. */
    push %ebx /* Address of multiboot2 structure. */
    push %eax /* Magic value.                     */

    /*
     * Clear all general purpose registers,
     * so we have a deterministic startup.
     */
    clear_gprs

    /* Call kernel main function. */
    push %esp
    call kmain
    addl $4, %esp

    /* Cleanup multiboot parameters. */
    addl $8, %esp

    /* Halt execution. */
    halt

.align 4
.globl _mp_start
/* SMP boot entry */
.code16
_mp_start:
    cli
# 	cli
# 	wbinvd
# 	mov $0, %ax
# 	mov %ax, %ds
# 	cld
#
# 	lgdt MP_PADDR(mp_boot_gdtdesc)
# 	movl %cr0, %eax
# 	orl $CR0_PE, %eax
# 	movl %eax, %cr0
# 	ljmp $PROTECTED_CSEG, $MP_PADDR(_mp_protected)

# .align 4
# mp_boot_gdt:
# 	.quad 0 # null seg
# 	SEGDESC(0, 0xfffff,
# 			SEG_R | SEG_CODE | SEG_S | SEG_DPL(0) | SEG_P | SEG_D | SEG_G)
# 	SEGDESC(0, 0xfffff,
# 			SEG_W | SEG_S | SEG_DPL(0) | SEG_P | SEG_D | SEG_G)
#
# .align 16
# mp_boot_gdtdesc:
# 	.word	0x17 # sizeof(mp_boot_gdt) - 1
# 	.long	MP_PADDR(mp_boot_gdt)  # address gdt
#
#
# .code32
# _mp_protected:
# 	movw $PROTECTED_DSEG, %ax
# 	movw %ax, %ds
# 	movw %ax, %es
# 	movw %ax, %fs
# 	movw %ax, %gs
# 	movw %ax, %ss
# 	movl $PADDR(cur_cpu_stack), %esp
#
# 	call loadgdt
# 	ljmp $KCSEG32, $PADDR(_mp_star32)
#
# .code32
# _mp_star32:
# 	call init32e
# 	movl $PADDR(_mp_star64), %eax
# 	ljmp $KCSEG64, $PADDR(_tramp64)
#
# .code64
# _mp_star64:
# 	movq cur_cpu_stack, %rsp
# 	movq $0, %rbp
# 	movl cur_cpu_id, %edi
# 	call secondary_start
# 	/* Un-reachable */
# 	jmp .
#
# .code64
# _tramp64:
# 	movq $KCODE, %r11
# 	/* rax stores the paddr of _start64 */
# 	addq %r11, %rax
# 	jmp *%rax
#
# /* initial stack */
# .comm stack, KSTACKSIZE
#
# /* Page table attribute bits */
# #define PRESENT  (1 << 0)
# #define WRITABLE (1 << 1)
# #define HUGE_1G  (1 << 7)
# #define GLOBAL   (1 << 8)
# #define NX	 (1ULL << 63)
#
# /* CHCORE page table: one PGD page */
# .align 4096
# .global CHCORE_PGD
# CHCORE_PGD:
# 	.quad PADDR(PUD_0)    + PRESENT + WRITABLE
# 	.space 4096 - 3*8
# 	.quad PADDR(PUD_DM)   + PRESENT + WRITABLE
# 	.quad PADDR(PUD_CODE) + PRESENT + WRITABLE
#
# /* CHCORE page table: three PUD pages */
# .align 4096
# .global CHCORE_PUD_Boot_Mapping
# CHCORE_PUD_Boot_Mapping:
# PUD_0:
# 	/*
# 	 * Bindly mapping 0~4G with rwx here.
# 	 * We will adjust the kernel page table mapping after detecting
# 	 * the useable physical memory. See 'refill_kernel_page_table'.
# 	 */
# 	.quad (0 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL
# 	.quad (1 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL
# 	.quad (2 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL
# 	.quad (3 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL
# 	.space 4096 - 4*8
#
# .align 4096
# /* PUD_Direct_Mapping */
# .global CHCORE_PUD_Direct_Mapping
# CHCORE_PUD_Direct_Mapping:
# PUD_DM:
# 	/* Bindly mapping 0~4G with rw */
# 	.quad (0 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL + NX
# 	.quad (1 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL + NX
# 	.quad (2 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL + NX
# 	.quad (3 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL + NX
# 	.space 4096 - 4*8
#
# .align 4096
# .global CHCORE_PUD_CODE_Mapping
# CHCORE_PUD_CODE_Mapping:
# PUD_CODE:
# 	.space 4096 - 8
# 	/* Simply mapping 0~1G with rwx */
# 	.quad (0 << 30) + PRESENT + WRITABLE + HUGE_1G + GLOBAL
#
# .align 4096
# .code32
# loadgdt:
# 	subl $8, %esp
# 	// TODO: bootgdt
# 	movl $PADDR(bootgdt), 4(%esp)
# 	movw $(8*GDT_ENTRIES-1), 2(%esp)
# 	lgdt 2(%esp)
# 	addl $8, %esp
#
# 	/* data segment selector */
# 	movl $KDSEG, %eax
# 	movw %ax, %ds
# 	movw %ax, %es
# 	movw %ax, %ss
#
# 	/* null segment selector */
# 	movl $0, %eax
# 	movw %ax, %fs
# 	movw %ax, %gs
#
# 	ret
#
#
# .code32
# /* initialize IA-32e mode */
# init32e:
# 	/* set cr4.PAE = cr4.PSE = cr4.OSFXSR = cr4.OSXMMEXCPT = 1 */
# 	movl %cr4, %eax
# 	// TODO: 0x630
# 	orl $0x630, %eax
# 	movl %eax, %cr4
#
# 	/* load cr3 with physical base address of level 4 page table */
# 	movl $PADDR(CHCORE_PGD), %eax
# 	movl %eax, %cr3
#
# 	/*
# 	 * enable IA-32e mode by setting IA32_EFER.LME = 1
# 	 * turn on IA32_EFER.SCE (syscall enable) and IA32_EFER.NXE (no-execute enable)
# 	 */
# 	movl $0xc0000080, %ecx
# 	rdmsr
# 	orl $((1 << 8) | (1 << 0) | (1 << 11)), %eax
# 	wrmsr
#
# 	wbinvd
# 	/* enable paging by setting cr0.PG = 1 */
# 	movl %cr0, %eax
# 	orl $0x80000000, %eax
# 	movl %eax, %cr0
#
# 	ret

.globl _mp_start_end
_mp_start_end:


/*============================================================================*
 * Data Section                                                               *
 *============================================================================*/

.section .data

/*
 * Kernel stack.
 */
kstack:
    .skip PAGE_SIZE
