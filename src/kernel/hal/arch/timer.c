/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/lib.h>
#include <stdint.h>

/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

/**
 * @brief Module name.
 *
 */
#define MODULE_NAME "[hal][timer]"

/**
 * @brief Oscillator frequency (in Hz)
 */
#define PIT_FREQUENCY 1193182

/**
 * @name Registers
 */
/**@{*/
#define PIT_CTRL 0x43 /** Control */
#define PIT_DATA 0x40 /** Data    */
/**@}*/

/**
 * @name Select Channels
 */
/**@{*/
#define PIT_SEL0 0x00 /** Channel 0         */
#define PIT_SEL1 0x40 /** Channel 1         */
#define PIT_SEL2 0x80 /** Channel 2         */
#define PIT_RB 0xc0   /** Read-back command */
/**@}*/

/**
 * @name Read-back Commands
 */
/**@{*/
#define PIT_RB_CNTR0 0x02 /** Read-back channel 0      */
#define PIT_RB_CNTR1 0x04 /** Read-back channel 1      */
#define PIT_RB_CNTR2 0x08 /** Read-back channel 2      */
#define PIT_RB_STAT 0x10  /** Don't latch status flag. */
#define PIT_RB_COUNT 0x20 /** Don't latch count flag.  */
/**@}*/

/**
 * @name Status Byte
 */
/**@{*/
#define PIT_STAT_OUT 0x80  /** OUT pin status */
#define PIT_STAT_NULL 0x00 /** NULL status    */
/**@}*/

/**
 * @name Access Mode.
 */
/**@{*/
#define PIT_ACC_LATCH 0x00 /** Latch count value          */
#define PIT_ACC_LO 0x10    /** Access mode: lobyte only   */
#define PIT_ACC_HI 0x20    /** Access mode: hibyte only   */
#define PIT_ACC_LOHI 0x30  /** Access mode: lobyte/hibyte */
/**@}*/

/**
 * @name Operating Mode
 */
/**@{*/
#define PIT_MODE_TCOUNT 0x00   /** Mode 0: interrupt on terminal count      */
#define PIT_MODE_HWSHOT 0x02   /** Mode 1: hardware re-triggerable one-shot */
#define PIT_MODE_RATE 0x04     /** Mode 2: rate generator                   */
#define PIT_MODE_WAVE 0x06     /** Mode 3: square wave generator            */
#define PIT_MODE_SWSTROBE 0x08 /** Mode 4: software triggered strobe        */
#define PIT_MODE_HWSTROBE 0x0a /** Mode 5: hardware triggered strobe        */
/**@}*/

/**
 * @name BCD/Binary Mode
 */
/**@{*/
#define PIT_BINARY 0x00 /** Binary mode */
#define PIT_BCD 0x01    /** BCD mode    */
/**@}*/

/*============================================================================*
 * Private Functions                                                          *
 *============================================================================*/

/**
 * @details Gets the frequency of the CPU.
 */
uint64_t get_cpu_freq(void)
{
    static uint64_t cpuhz = 0;
    if (cpuhz == 0) {
        // Setup PIT for terminal count starting from 2^16 - 1
        uint64_t xticks = 0x000000000000FFFFull;
        output8(PIT_CTRL,
                PIT_SEL0 | PIT_ACC_LOHI | PIT_MODE_TCOUNT | PIT_BINARY);
        output8(PIT_DATA, xticks % 256);
        output8(PIT_DATA, xticks / 256);

        // Wait until OUT bit of status byte is set
        uint64_t s = rdtsc();
        do {
            output8(PIT_CTRL, PIT_RB | PIT_RB_COUNT | PIT_RB_CNTR0);
            if (rdtsc() - s > 1ULL << 32) {
                kprintf("PIT stuck, assuming 2GHz");
                cpuhz = 2 * 1000 * 1000 * 1000;
                return cpuhz;
            }
        } while (!(input8(PIT_DATA) & PIT_STAT_OUT));
        uint64_t e = rdtsc();

        cpuhz = ((e - s) * 10000000) / ((xticks * 10000000) / PIT_FREQUENCY);
    }

    // kprintf("cpu freq: %d Hz", cpuhz);
    return (cpuhz);
}

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details This function initializes the timer device. The frequency of the
 * device is set to @p freq Hz.
 */
void timer_init(unsigned freq)
{
    const uint16_t freq_divisor = PIT_FREQUENCY / freq;

    kprintf(MODULE_NAME " initializing timer...");

    // Send control byte: adjust frequency divisor.
    kprintf(MODULE_NAME " setting frequency to %d Hz", freq);
    output8(PIT_CTRL, PIT_SEL0 | PIT_ACC_LOHI | PIT_MODE_WAVE | PIT_BINARY);

    // Send data byte: divisor_low and divisor_high.
    output8(PIT_DATA, (uint8_t)(freq_divisor & 0xff));
    output8(PIT_DATA, (uint8_t)((freq_divisor >> 8)));
}
