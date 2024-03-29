/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/cc.h>
#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/libcore.h>

/**
 * @name Exception Hooks
 *
 * @note These are defined in assembly code.
 */
/**@{*/
extern void _do_excp0(void);  /** Division-by-Zero Error      */
extern void _do_excp1(void);  /** Debug Exception             */
extern void _do_excp2(void);  /** Non-Maskable Interrupt      */
extern void _do_excp3(void);  /** Breakpoint Exception        */
extern void _do_excp4(void);  /** Overflow Exception          */
extern void _do_excp5(void);  /** Bounds Check Exception      */
extern void _do_excp6(void);  /** Invalid Opcode Exception    */
extern void _do_excp7(void);  /** Coprocessor Not Available   */
extern void _do_excp8(void);  /** Double Fault                */
extern void _do_excp9(void);  /** Coprocessor Segment Overrun */
extern void _do_excp10(void); /** Invalid TSS                 */
extern void _do_excp11(void); /** Segment Not Present         */
extern void _do_excp12(void); /** Stack Segment Fault         */
extern void _do_excp13(void); /** General Protection Fault    */
extern void _do_excp14(void); /** Page Fault                  */
extern void _do_excp15(void); /** Reserved                    */
extern void _do_excp16(void); /** Floating Point Exception    */
extern void _do_excp17(void); /** Alignment Check Exception   */
extern void _do_excp18(void); /** Machine Check Exception     */
extern void _do_excp19(void); /** SMID Unit Exception         */
extern void _do_excp20(void); /** Virtual Exception           */
extern void _do_excp30(void); /** Security Exception.         */
/**@}*/

/**
 * @name Hardware Interrupt Hooks
 *
 * @note These are defined in assembly code.
 */
/**@{*/
extern void _do_hwint0(void);
extern void _do_hwint1(void);
extern void _do_hwint2(void);
extern void _do_hwint3(void);
extern void _do_hwint4(void);
extern void _do_hwint5(void);
extern void _do_hwint6(void);
extern void _do_hwint7(void);
extern void _do_hwint8(void);
extern void _do_hwint9(void);
extern void _do_hwint10(void);
extern void _do_hwint11(void);
extern void _do_hwint12(void);
extern void _do_hwint13(void);
extern void _do_hwint14(void);
extern void _do_hwint15(void);
/**@}*/

/**
 * @name Trap Interrupt Hook
 *
 * @note This is defined in assembly code.
 */
/**@{*/
extern void _do_kcall(void); /** Kernel Call */
/**@}*/

/*============================================================================*
 * Private Variables                                                          *
 *============================================================================*/

/**
 * @brief Interrupt Descriptor Table (IDT).
 */
static struct idte idt[IDT_LENGTH];

/**
 * @brief Pointer to Interrupt Descriptor Table (IDTPTR)
 */
static struct idtptr idtptr;

/*============================================================================*
 * Private Functions                                                          *
 *============================================================================*/

/**
 * @brief Flushes the IDT.
 *
 * @details Flushes the Interrupt Descriptor Table (IDT).
 *
 * @param idtptr Interrupt descriptor table pointer.
 */
static void idt_load(const struct idtptr *_idtptr)
{
    kprintf("[hal][cpu] loading idt...");

    asm volatile("mov %0, %%eax;\
			lidt (%%eax);"
                 :
                 : "r"(_idtptr)
                 : "memory", "eax");
}

/**
 * @brief Sets an IDT entry.
 *
 * @param n        Target entry.
 * @param handler  Handler.
 * @param selector Segment selector.
 * @param flags    Handler flags.
 * @param type     Interrupt type.
 */
static void set_idte(int n, unsigned handler, unsigned selector, unsigned flags,
                     unsigned type)
{
    // Set handler.
    idt[n].handler_low = (handler & 0xffff);
    idt[n].handler_high = (handler >> 16) & 0xffff;

    // Set GDT selector.
    idt[n].selector = selector;

    // Set type and flags.
    idt[n].type = type;
    idt[n].flags = flags;
}

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details Initializes the Interrupt Descriptor Table (IDT). Returns the
 * vector offset for hardware interrupts.
 */
unsigned idt_init(unsigned cs_sel)
{
    kprintf("[hal][cpu] initializing idt...");

    // Ensure that size of structures match what we expect.
    KASSERT_SIZE(sizeof(struct idte), IDTE_SIZE);
    KASSERT_SIZE(sizeof(struct idtptr), IDTPTR_SIZE);

    // Blank IDT and IDT pointer.
    __memset(idt, 0, sizeof(idt));
    __memset(&idtptr, 0, IDTPTR_SIZE);

    // Set exception hooks.
    const unsigned excp_off = 0;
    set_idte(excp_off + 0, (unsigned)_do_excp0, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 1, (unsigned)_do_excp1, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 2, (unsigned)_do_excp2, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 3, (unsigned)_do_excp3, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 4, (unsigned)_do_excp4, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 5, (unsigned)_do_excp5, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 6, (unsigned)_do_excp6, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 7, (unsigned)_do_excp7, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 8, (unsigned)_do_excp8, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 9, (unsigned)_do_excp9, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 10, (unsigned)_do_excp10, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 11, (unsigned)_do_excp11, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 12, (unsigned)_do_excp12, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 13, (unsigned)_do_excp13, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 14, (unsigned)_do_excp14, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 15, (unsigned)_do_excp15, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 16, (unsigned)_do_excp16, cs_sel, 0x8, IDT_INT32);
    for (int i = 21; i < 30; i++) {
        set_idte(excp_off + i, (unsigned)_do_excp15, cs_sel, 0x8, IDT_INT32);
    }
    set_idte(excp_off + 30, (unsigned)_do_excp30, cs_sel, 0x8, IDT_INT32);
    set_idte(excp_off + 31, (unsigned)_do_excp15, cs_sel, 0x8, IDT_INT32);

    // Set hardware interrupts.
    const unsigned hwint_off = 32;
    set_idte(hwint_off + 0, (unsigned)_do_hwint0, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 1, (unsigned)_do_hwint1, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 2, (unsigned)_do_hwint2, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 3, (unsigned)_do_hwint3, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 4, (unsigned)_do_hwint4, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 5, (unsigned)_do_hwint5, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 6, (unsigned)_do_hwint6, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 7, (unsigned)_do_hwint7, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 8, (unsigned)_do_hwint8, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 9, (unsigned)_do_hwint9, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 10, (unsigned)_do_hwint10, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 11, (unsigned)_do_hwint11, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 12, (unsigned)_do_hwint12, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 13, (unsigned)_do_hwint13, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 14, (unsigned)_do_hwint14, cs_sel, 0x8, IDT_INT32);
    set_idte(hwint_off + 15, (unsigned)_do_hwint15, cs_sel, 0x8, IDT_INT32);

    // Set kernel call interrupt.
    set_idte(TRAP_GATE, (unsigned)_do_kcall, cs_sel, 0xe, IDT_INT32);

    // Set IDT pointer.
    idtptr.size = sizeof(idt) - 1;
    idtptr.ptr = (unsigned)&idt;

    // Load IDT.
    idt_load(&idtptr);

    return (hwint_off);
}
