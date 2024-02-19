/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/* Must come first. */
#define _ASM_FILE_

/*============================================================================*
 * Exported Symbols                                                           *
 *============================================================================*/

.globl __has_cpuid

/*============================================================================*
 * Text Section                                                               *
 *============================================================================*/

.section .text,"ax", @progbits

/*----------------------------------------------------------------------------*
 * __has_cpuid()                                                              *
 *----------------------------------------------------------------------------*/

.align 4
__has_cpuid:
    pushfl                               # Save EFLAGS
    pushfl                               # Store EFLAGS
    xorl $0x00200000, (%esp)             # Invert the ID bit in stored EFLAGS
    popfl                                # Load stored EFLAGS (with ID bit inverted)
    pushfl                               # Store EFLAGS again (ID bit may or may not be inverted)
    popl %eax                            # eax = modified EFLAGS (ID bit may or may not be inverted)
    xorl (%esp), %eax                    # eax = whichever bits were changed
    popfl                                # Restore original EFLAGS
    andl $0x00200000, %eax               # eax = zero if ID bit can't be changed, else non-zero
    movl $0, %eax
    jz __has_cpuid.out
    movl $1, %eax
    __has_cpuid.out:
    ret
