/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#include <stdint.h>

/**
 * @brief Performs a 64-bit unsigned integer division.
 *
 * @details This is a simple implementation of the __udivdi3 function, which is
 * used by the GCC compiler to perform 64-bit unsigned integer division.
 * This implementation is based on the algorithm described in the
 * "Hacker's Delight" book by Henry S. Warren, Jr.
 *
 * @note This implementation is not efficient and should be replaced by
 * a more efficient one in the future.
 *
 * @param n Dividend.
 * @param d Divisor.
 *
 * @return The quotient of the division.
 */
uint64_t __udivdi3(uint64_t n, uint64_t d)
{
    uint64_t q = 0;
    uint64_t r = 0;

    for (int i = 0; i < 64; i++) {
        r = (r << 1) | ((n >> 63) & 1);
        n <<= 1;
        q <<= 1;

        if (r >= d) {
            r -= d;
            q |= 1;
        }
    }

    return (q);
}
