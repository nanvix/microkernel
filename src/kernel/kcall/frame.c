/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

/*============================================================================*
 * Imports                                                                    *
 *============================================================================*/

#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/mm.h>

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @details Attempts to allocate a page frame.
 */
frame_t kcall_fralloc(void)
{
    // TODO: https://github.com/nanvix/microkernel/issues/367

    KASSERT_SIZE_LE(sizeof(frame_t), sizeof(word_t));

    return (frame_alloc_any());
}

/**
 * @details Frees a page frame.
 */
int kcall_frfree(frame_t frame)
{
    // TODO: https://github.com/nanvix/microkernel/issues/367

    // Check if the target frame number is invalid.
    if (frame < (USER_BASE_PHYS >> PAGE_SHIFT)) {
        return (-1);
    }

    return (frame_free(frame));
}
