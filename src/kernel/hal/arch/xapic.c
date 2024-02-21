/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>
#include <stdbool.h>
#include <stdint.h>

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

#define APIC_INTER_HALT_VECTOR 248
#define APIC_INTER_CORE_VECTOR 249
#define APIC_TIMER_INTERRUPT_VECTOR 250
#define APIC_THERMAL_INTERRUPT_VECTOR 251
#define APIC_PERFORMANCE_INTERRUPT_VECTOR 252
#define APIC_ERROR_INTERRUPT_VECTOR 253
#define APIC_SPURIOUS_INTERRUPT_VECTOR 254

#define QUANTUM 10 // clock frequency is 100 MHz

// Processor-defined:
#define T_DIVIDE 0 // divide error
#define T_DEBUG 1  // debug exception
#define T_NMI 2    // non-maskable interrupt
#define T_BRKPT 3  // breakpoint
#define T_OFLOW 4  // overflow
#define T_BOUND 5  // bounds check
#define T_ILLOP 6  // illegal opcode
#define T_DEVICE 7 // device not available
#define T_DBLFLT 8 // double fault
// #define T_COPROC      9      // reserved (not used since 486)
#define T_TSS 10   // invalid task switch segment
#define T_SEGNP 11 // segment not present
#define T_STACK 12 // stack exception
#define T_GPFLT 13 // general protection fault
#define T_PGFLT 14 // page fault
// #define T_RES        15      // reserved
#define T_FPERR 16   // floating point error
#define T_ALIGN 17   // aligment check
#define T_MCHK 18    // machine check
#define T_SIMDERR 19 // SIMD floating point error

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_TLBFLUSH 65 // flush TLB
#define T_SAMPCONF 66 // configure event counters
#define T_IPICALL 67  // Queued IPI call
#define T_DEFAULT 500 // catchall

#define T_IRQ0 32 // IRQ 0 corresponds to int T_IRQ

#define IRQ_TIMER 0
#define IRQ_KBD 1
#define IRQ_COM2 3
#define IRQ_COM1 4
#define IRQ_IDE 14
#define IRQ_ERROR 19
#define IRQ_SPURIOUS 31

// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID (0x0020 / 4)   // ID
#define VER (0x0030 / 4)  // Version
#define TPR (0x0080 / 4)  // Task Priority
#define PPR (0x00A0 / 4)  // Processor Priority
#define EOI (0x00B0 / 4)  // EOI
#define LDR (0x00D0 / 4)  // Logical Destination
#define SVR (0x00F0 / 4)  // Spurious Interrupt Vector
#define ENABLE 0x00000100 // Unit Enable
#define ISR (0x0100 / 4)  // In-service register
#define ISR_NR 0x8
#define TMR (0x0180 / 4)   // Trigger mode register
#define IRR (0x0200 / 4)   // Interrupt request register
#define ESR (0x0280 / 4)   // Error Status
#define CMCI (0x02f0 / 4)  // CMCI LVT
#define ICRLO (0x0300 / 4) // Interrupt Command
#define INIT 0x00000500    // INIT/RESET
#define STARTUP 0x00000600 // Startup IPI
#define DELIVS 0x00001000  // Delivery status
#define ASSERT 0x00004000  // Assert interrupt (vs deassert)
#define DEASSERT 0x00000000
#define LEVEL 0x00008000 // Level triggered
#define BCAST 0x00080000 // Send to all APICs, including self.
#define FIXED 0x00000000
#define ICRHI (0x0310 / 4)  // Interrupt Command [63:32]
#define TIMER (0x0320 / 4)  // Local Vector Table 0 (TIMER)
#define X1 0x0000000B       // divide counts by 1
#define PERIODIC 0x00020000 // Periodic
#define THERM (0x0330 / 4)  // Thermal sensor LVT
#define PCINT (0x0340 / 4)  // Performance Counter LVT
#define LINT0 (0x0350 / 4)  // Local Vector Table 1 (LINT0)
#define LINT1 (0x0360 / 4)  // Local Vector Table 2 (LINT1)
#define ERROR (0x0370 / 4)  // Local Vector Table 3 (ERROR)
#define MASKED 0x00010000   // Interrupt masked
#define MT_NMI 0x00000400   // NMI message type
#define MT_FIX 0x00000000   // Fixed message type
#define TICR (0x0380 / 4)   // Timer Initial Count
#define TCCR (0x0390 / 4)   // Timer Current Count
#define TDCR (0x03E0 / 4)   // Timer Divide Configuration

#define CMOS_ADDR 0x70

#define APIC_VECTOR_MASK 0xff
#define APIC_SPIV_APIC_ENABLED (1 << 8)

static volatile uint32_t *xapic = NULL;
static uint64_t xapichz;

/*============================================================================*
 * Private Functions                                                          *
 *============================================================================*/

static void microdelay(uint64_t delay)
{

    uint64_t tscdelay = (get_cpu_freq() * delay) / 1000000;
    uint64_t s = rdtsc();
    while (rdtsc() - s < tscdelay) {
        pause();
    }
}

static void xapic_write(uint32_t index, uint32_t value)
{
    xapic[index] = value;
    xapic[ID]; // wait for write to finish, by reading
}

static uint32_t xapic_read(uint32_t off)
{
    return xapic[off];
}

static void mask_pc(bool mask)
{
    xapic_write(PCINT, mask ? MASKED : MT_NMI);
}

static void __cpu_init()
{
    uint64_t count;

    kprintf("xapic: Initializing LAPIC");

    // Enable local APIC, do not suppress EOI broadcast, set spurious
    // interrupt vector.
    xapic_write(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

    if (xapichz == 0) {
        // Measure the TICR frequency
        xapic_write(TDCR, X1);
        xapic_write(TICR, 0xffffffff);
        uint64_t ccr0 = xapic_read(TCCR);
        microdelay(10 * 1000); // 1/100th of a second
        uint64_t ccr1 = xapic_read(TCCR);
        xapichz = 100 * (ccr0 - ccr1);
    }

    count = (QUANTUM * xapichz) / 1000;
    if (count > 0xffffffff) {
        kpanic("initxapic: QUANTUM too large");
    }

    // The timer repeatedly counts down at bus frequency
    // from xapic[TICR] and then issues an interrupt.
    xapic_write(TDCR, X1);
    xapic_write(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
    xapic_write(TICR, count);

    // Disable logical interrupt lines.
    xapic_write(LINT0, MASKED);
    xapic_write(LINT1, MASKED);

    // Disable performance counter overflow interrupts
    // on machines that provide that interrupt entry.
    if (((xapic[VER] >> 16) & 0xFF) >= 4) {
        mask_pc(false);
    }

    // Map error interrupt to IRQ_ERROR.
    xapic_write(ERROR, T_IRQ0 + IRQ_ERROR);

    // Clear error status register (requires back-to-back writes).
    xapic_write(ESR, 0);
    xapic_write(ESR, 0);

    // Ack any outstanding interrupts.
    xapic_write(EOI, 0);

    // Send an Init Level De-Assert to synchronise arbitration ID's.
    xapic_write(ICRHI, 0);
    xapic_write(ICRLO, BCAST | INIT | LEVEL);
    while (xapic[ICRLO] & DELIVS) {
        noop();
    }

    // Enable interrupts on the APIC (but not on the processor).
    xapic_write(TPR, 0);
}

static int xapic_wait(void)
{
    int i = 100000;
    while ((xapic_read(ICRLO) & DELIVS) != 0) {
        pause();
        i--;
        if (i == 0) {
            kprintf("xapic_wait: wedged?\n");
            return -1;
        }
    }
    return 0;
}

#define CMOS_REGISTER_SHUTDOWN_STATUS 0xf

static void cmos_write(uint8_t reg, uint8_t value)
{
    output8(CMOS_ADDR, reg); // offset 0xF is shutdown code
    output8(CMOS_ADDR + 1, value);
}

static void warm_reset_write(uint32_t addr)
{
    static volatile uint16_t *wrv =
        (uint16_t *)(0x40 << 4 | 0x67); // Warm reset vector
    wrv[0] = 0;
    wrv[1] = addr >> 4;
}

static void start_ap(uint32_t coreid, uint32_t addr)
{
    // "The BSP must initialize CMOS shutdown code to 0AH
    // and the warm reset vector (DWORD based at 40:67) to point at
    // the AP startup code prior to the [universal startup algorithm]."
    cmos_write(CMOS_REGISTER_SHUTDOWN_STATUS, 0x0a);
    warm_reset_write(addr);

    // "Universal startup algorithm."
    // Send INIT (level-triggered) interrupt to reset other CPU.

    xapic_write(ICRHI, coreid << 24);
    xapic_write(ICRLO, INIT | LEVEL | ASSERT);
    xapic_wait();
    microdelay(10000);
    xapic_write(ICRLO, INIT | LEVEL);
    xapic_wait();
    microdelay(10000);

    // Send startup IPI (twice!) to enter bootstrap code.
    // Regular hardware is supposed to only accept a STARTUP
    // when it is in the halted state due to an INIT.  So the second
    // should be ignored, but it is part of the official Intel algorithm.
    // Bochs complains about the second one.  Too bad for Bochs.
    for (int i = 0; i < 2; i++) {
        uint32_t tmp;
        tmp = xapic_read(ICRHI);
        tmp = (tmp & 0xffffffff) | (coreid << 24);
        xapic_write(ICRHI, tmp);
        tmp = xapic_read(ICRLO);
        tmp = (tmp & 0xfff00000) | (0x4000 | STARTUP | (addr >> 12));
        xapic_write(ICRLO, tmp);
        microdelay(200);
    }
}

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

void xapic_init(void)
{
    uint32_t bootaddr = 0x7000;
    uint64_t apic_base = rdmsr(MSR_IA32_APIC_BASE);

    // Sanity check: xAPIC must be enabled.
    if (!(apic_base & MSR_IA32_APIC_BASE_ENABLE)) {
        kpanic("xAPIC is not enabled");
    }

    xapic = (uint32_t *)(uint32_t)(apic_base & MSR_IA32_APIC_BASE_BASE);

    // Print address.
    kprintf("xAPIC base address: %x", xapic);

    extern char _mp_start;
    extern char _mp_start_end;

    __memcpy((void *)bootaddr,
             (void *)(&_mp_start),
             (uint32_t)(&_mp_start_end) - (uint32_t)(&_mp_start));

    __cpu_init();
    start_ap(1, bootaddr);
}
