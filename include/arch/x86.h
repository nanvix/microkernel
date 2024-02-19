/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#ifndef ARCH_X86_H_
#define ARCH_X86_H_

#include <arch/x86/cpu.h>
#include <arch/x86/mmu.h>
#include <dev/uart.h>
#include <nanvix/cc.h>
#ifndef _ASM_FILE_
#include <stdbool.h>
#include <stdint.h>
#endif /* !_ASM_FILE_ */

/**
 * @name Capabilities
 */
/**@{*/
#define __TARGET_HAS_PMIO 1   /** Has Port-Mapped I/O?      */
#define __TARGET_HAS_TLB_HW 1 /** Has Hardware-Managed TLB? */
/**@}*/

/**
 * @name I/O Ports for Serial Device
 */
/**@{*/
#define UART_PORT_0 0x3f8 /** Port 0 */
#define UART_PORT_1 0x2f8 /** Port 1 */
#define UART_PORT_2 0x3e8 /** Port 2 */
#define UART_PORT_3 0x2e8 /** Port 3 */
#define UART_PORT_4 0x5f8 /** Port 4 */
#define UART_PORT_5 0x4f8 /** Port 5 */
#define UART_PORT_6 0x5e8 /** Port 6 */
#define UART_PORT_7 0x4e8 /** Port 7 */
/**@}*/

#ifndef _ASM_FILE_

/**
 * @brief Writes 8 bits to an I/O port.
 *
 * @param port Number of the target port.
 * @param bits Bits to write.
 */
static inline void output8(uint16_t port, uint8_t bits)
{
    asm volatile("outb %0, %1" : : "a"(bits), "Nd"(port));
}

/**
 * @brief Writes 16 bits to an I/O port.
 *
 * @param port Number of the target port.
 * @param bits Bits to write.
 */
static inline void output16(uint16_t port, uint16_t bits)
{
    asm volatile("outw %0, %1" : : "a"(bits), "Nd"(port));
}

/**
 * @brief Writes 32 bits to an I/O port.
 *
 * @param port Number of the target port.
 * @param bits Bits to write.
 */
static inline void output32(uint16_t port, uint32_t bits)
{
    asm volatile("outl %0, %1" : : "a"(bits), "Nd"(port));
}

/**
 * @brief Waits for an operation in an I/O port to complete.
 */
static inline void iowait(void)
{
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

/**
 * @brief Read 8 bits from an I/O port.
 *
 * @param port Number of the target port.
 */
static inline uint8_t input8(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return (ret);
}

/**
 * @brief Read 16 bits from an I/O port.
 *
 * @param port Number of the target port.
 */
static inline uint16_t input16(uint16_t port)
{
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return (ret);
}

/**
 * @brief Read 32 bits from an I/O port.
 *
 * @param port Number of the target port.
 */
static inline uint32_t input32(uint16_t port)
{
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return (ret);
}

/**
 * @brief Reads the value of the Time Stamp Counter (TSC).
 *
 * @return The value of the Time Stamp Counter (TSC).
 */
static inline uint64_t rdtsc(void)
{
    uint32_t hi, lo;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

/**
 * @brief Pauses the CPU.
 */
static inline void pause(void)
{
    asm volatile("pause" : :);
}

/**
 * @brief Checks if the CPU has the CPUID instruction.
 *
 * @return True if the CPU has the CPUID instruction, and false otherwise.
 */
extern bool __has_cpuid(void);

#endif /* !_ASM_FILE_ */

#endif /* ARCH_X86_H */
