/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

#include <grub/mboot2.h>
#include <nanvix/kernel/hal.h>
#include <nanvix/kernel/kmod.h>
#include <nanvix/kernel/lib.h>
#include <nanvix/kernel/log.h>

/*============================================================================*
 * Structures                                                                 *
 *============================================================================*/

/**
 * @brief Kernel arguments.
 */
struct kargs {
    unsigned long magic; /** Magic number.                         */
    unsigned long addr;  /** Address of the Multiboot information. */
};

/*============================================================================*
 * Private Functions                                                          *
 *============================================================================*/

/**
 * @brief Parses a multiboot module tag.
 *
 * @param tag Target module tag.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
static int mboot_parse_module(struct mboot_tag *tag)
{
    int ret = 0;
    struct mboot_tag_module *mod = (struct mboot_tag_module *)tag;
    ret |= kmod_register(mod->mod_start, mod->mod_end, mod->cmdline);
    return (ret);
}

/**
 * @brief Parses a multiboot memory map tag.
 *
 * @param tag Target memory map tag.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
static int mboot_parse_mmap(struct mboot_tag *tag)
{
    int ret = 0;

    for (mboot_memory_map_t *mmap = ((struct mboot_tag_mmap *)tag)->entries;
         (mboot_uint8_t *)mmap < (mboot_uint8_t *)tag + tag->size;
         mmap = (mboot_memory_map_t *)((unsigned long)mmap +
                                       ((struct mboot_tag_mmap *)tag)
                                           ->entry_size)) {
        switch (mmap->type) {
            case MBOOT_MEMORY_AVAILABLE:
                ret |=
                    mmap_register_available((unsigned long)mmap->addr,
                                            (size_t)(unsigned long)mmap->len);
                break;
            case MBOOT_MEMORY_BADRAM:
                ret |=
                    mmap_register_erroneous((unsigned long)mmap->addr,
                                            (size_t)(unsigned long)mmap->len);
                break;
            default:
                ret |= mmap_register_reserved((unsigned long)mmap->addr,
                                              (size_t)(unsigned long)mmap->len);
                break;
        }
    }

    return (ret);
}

/**
 * @brief Parses multiboot ACPI tag.
 *
 * @param tag Target ACPI tag.
 *
 * @return Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
static int mboot_parse_acpi(struct mboot_tag *tag)
{
    int ret = 0;
    extern int acpi_info_parse(const void *);

    struct mboot_tag_acpi *mboot_tag_acpi = (struct mboot_tag_acpi *)tag;

    log(INFO,
        "found acpi tag (size=%d, rsdp=%x)",
        mboot_tag_acpi->size,
        mboot_tag_acpi->rsdp);

    ret |= acpi_info_parse((void *)mboot_tag_acpi->rsdp);

    return (ret);
}

/**
 * @details Parses multiboot information.
 *
 * @param magic Magic number passed in by boot loader.
 * @param addr Physical address of multiboot structure.
 *
 * @returns Upon successful completion, zero is returned. Upon failure, a
 * negative number is returned instead.
 */
static int mboot_parse(unsigned magic, unsigned long addr)
{
    // Check if magic number matches what we expect.
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kprintf("Invalid magic number: %x", (unsigned)magic);
        return (-1);
    }

    // Check if multiboot structure is aligned in a 4 byte boundary.
    if (addr & 7) {
        kprintf("Unaligned mbi: %x", addr);
        return (-1);
    }

    int ret = 0;
    struct mboot_tag *tag = (struct mboot_tag *)(addr + 8);

    while (tag->type != MBOOT_TAG_TYPE_END) {
        switch (tag->type) {
            case MBOOT_TAG_TYPE_CMDLINE: {
                // TODO: Parse command line.
            } break;
            case MBOOT_TAG_TYPE_BOOT_LOADER_NAME: {
                // TODO: Parse boot loader name.
            } break;
            case MBOOT_TAG_TYPE_MODULE: {
                ret |= mboot_parse_module(tag);
            } break;
            case MBOOT_TAG_TYPE_BASIC_MEMINFO: {
                // TODO: Parse basic memory information.
            } break;
            case MBOOT_TAG_TYPE_BOOTDEV: {
                // TODO: Parse boot device.
            } break;
            case MBOOT_TAG_TYPE_MMAP: {
                ret |= mboot_parse_mmap(tag);
            } break;
            case MBOOT_TAG_TYPE_FRAMEBUFFER: {
                // TODO: Parse framebuffer.
            } break;
            case MBOOT_TAG_TYPE_ACPI_OLD: {
                ret |= mboot_parse_acpi(tag);
            } break;
            case MBOOT_TAG_TYPE_ACPI_NEW: {
                ret |= mboot_parse_acpi(tag);
            } break;
            default:
                break;
        }

        tag =
            (struct mboot_tag *)((mboot_uint8_t *)tag + TRUNCATE(tag->size, 8));
    }

    return (ret);
}

/*============================================================================*
 * Public Functions                                                           *
 *============================================================================*/

/**
 * @brief Parses kernel arguments.
 */
int kargs_parse(struct kargs *args)
{
    mboot_parse(args->magic, args->addr);

    return (0);
}
