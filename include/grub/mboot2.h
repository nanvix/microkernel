/*
 * Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 * DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MBOOT_HEADER
#define MBOOT_HEADER 1

/*  How many bytes from the start of the file we search for the header. */
#define MBOOT_SEARCH 32768
#define MBOOT_HEADER_ALIGN 8

/*  The magic field should contain this. */
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6

/*  This should be in %eax. */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

/*  Alignment of multiboot modules. */
#define MBOOT_MOD_ALIGN 0x00001000

/*  Alignment of the multiboot info structure. */
#define MBOOT_INFO_ALIGN 0x00000008

/*  Flags set in the ’flags’ member of the multiboot header. */

#define MBOOT_TAG_ALIGN 8
#define MBOOT_TAG_TYPE_END 0
#define MBOOT_TAG_TYPE_CMDLINE 1
#define MBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MBOOT_TAG_TYPE_MODULE 3
#define MBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MBOOT_TAG_TYPE_BOOTDEV 5
#define MBOOT_TAG_TYPE_MMAP 6
#define MBOOT_TAG_TYPE_VBE 7
#define MBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MBOOT_TAG_TYPE_APM 10
#define MBOOT_TAG_TYPE_EFI32 11
#define MBOOT_TAG_TYPE_EFI64 12
#define MBOOT_TAG_TYPE_SMBIOS 13
#define MBOOT_TAG_TYPE_ACPI_OLD 14
#define MBOOT_TAG_TYPE_ACPI_NEW 15
#define MBOOT_TAG_TYPE_NETWORK 16
#define MBOOT_TAG_TYPE_EFI_MMAP 17
#define MBOOT_TAG_TYPE_EFI_BS 18
#define MBOOT_TAG_TYPE_EFI32_IH 19
#define MBOOT_TAG_TYPE_EFI64_IH 20
#define MBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

#define MBOOT_HEADER_TAG_END 0
#define MBOOT_HEADER_TAG_INFORMATION_REQUEST 1
#define MBOOT_HEADER_TAG_ADDRESS 2
#define MBOOT_HEADER_TAG_ENTRY_ADDRESS 3
#define MBOOT_HEADER_TAG_CONSOLE_FLAGS 4
#define MBOOT_HEADER_TAG_FRAMEBUFFER 5
#define MBOOT_HEADER_TAG_MODULE_ALIGN 6
#define MBOOT_HEADER_TAG_EFI_BS 7
#define MBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI32 8
#define MBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI64 9
#define MBOOT_HEADER_TAG_RELOCATABLE 10

#define MBOOT_ARCHITECTURE_I386 0
#define MBOOT_ARCHITECTURE_MIPS32 4
#define MBOOT_HEADER_TAG_OPTIONAL 1

#define MBOOT_LOAD_PREFERENCE_NONE 0
#define MBOOT_LOAD_PREFERENCE_LOW 1
#define MBOOT_LOAD_PREFERENCE_HIGH 2

#define MBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED 1
#define MBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED 2

#ifndef _ASM_FILE_

typedef unsigned char mboot_uint8_t;
typedef unsigned short mboot_uint16_t;
typedef unsigned int mboot_uint32_t;
typedef unsigned long long mboot_uint64_t;

struct mboot_header {
    /*  Must be MBOOT_MAGIC - see above. */
    mboot_uint32_t magic;

    /*  ISA */
    mboot_uint32_t architecture;

    /*  Total header length. */
    mboot_uint32_t header_length;

    /*  The above fields plus this one must equal 0 mod 2^32. */
    mboot_uint32_t checksum;
};

struct mboot_header_tag {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
};

struct mboot_header_tag_information_request {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
    mboot_uint32_t requests[];
};

struct mboot_header_tag_address {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
    mboot_uint32_t header_addr;
    mboot_uint32_t load_addr;
    mboot_uint32_t load_end_addr;
    mboot_uint32_t bss_end_addr;
};

struct mboot_header_tag_entry_address {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
    mboot_uint32_t entry_addr;
};

struct mboot_header_tag_console_flags {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
    mboot_uint32_t console_flags;
};

struct mboot_header_tag_framebuffer {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
    mboot_uint32_t width;
    mboot_uint32_t height;
    mboot_uint32_t depth;
};

struct mboot_header_tag_module_align {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
};

struct mboot_header_tag_relocatable {
    mboot_uint16_t type;
    mboot_uint16_t flags;
    mboot_uint32_t size;
    mboot_uint32_t min_addr;
    mboot_uint32_t max_addr;
    mboot_uint32_t align;
    mboot_uint32_t preference;
};

struct mboot_color {
    mboot_uint8_t red;
    mboot_uint8_t green;
    mboot_uint8_t blue;
};

struct mboot_mmap_entry {
    mboot_uint64_t addr;
    mboot_uint64_t len;
#define MBOOT_MEMORY_AVAILABLE 1
#define MBOOT_MEMORY_RESERVED 2
#define MBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MBOOT_MEMORY_NVS 4
#define MBOOT_MEMORY_BADRAM 5
    mboot_uint32_t type;
    mboot_uint32_t zero;
};
typedef struct mboot_mmap_entry mboot_memory_map_t;

struct mboot_tag {
    mboot_uint32_t type;
    mboot_uint32_t size;
};

struct mboot_tag_string {
    mboot_uint32_t type;
    mboot_uint32_t size;
    char string[];
};

struct mboot_tag_module {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t mod_start;
    mboot_uint32_t mod_end;
    char cmdline[];
};

struct mboot_tag_basic_meminfo {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t mem_lower;
    mboot_uint32_t mem_upper;
};

struct mboot_tag_bootdev {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t biosdev;
    mboot_uint32_t slice;
    mboot_uint32_t part;
};

struct mboot_tag_mmap {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t entry_size;
    mboot_uint32_t entry_version;
    struct mboot_mmap_entry entries[];
};

struct mboot_vbe_info_block {
    mboot_uint8_t external_specification[512];
};

struct mboot_vbe_mode_info_block {
    mboot_uint8_t external_specification[256];
};

struct mboot_tag_vbe {
    mboot_uint32_t type;
    mboot_uint32_t size;

    mboot_uint16_t vbe_mode;
    mboot_uint16_t vbe_interface_seg;
    mboot_uint16_t vbe_interface_off;
    mboot_uint16_t vbe_interface_len;

    struct mboot_vbe_info_block vbe_control_info;
    struct mboot_vbe_mode_info_block vbe_mode_info;
};

struct mboot_tag_framebuffer_common {
    mboot_uint32_t type;
    mboot_uint32_t size;

    mboot_uint64_t framebuffer_addr;
    mboot_uint32_t framebuffer_pitch;
    mboot_uint32_t framebuffer_width;
    mboot_uint32_t framebuffer_height;
    mboot_uint8_t framebuffer_bpp;
#define MBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MBOOT_FRAMEBUFFER_TYPE_RGB 1
#define MBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2
    mboot_uint8_t framebuffer_type;
    mboot_uint16_t reserved;
};

struct mboot_tag_framebuffer {
    struct mboot_tag_framebuffer_common common;

    union {
        struct {
            mboot_uint16_t framebuffer_palette_num_colors;
            struct mboot_color *framebuffer_palette;
        };
        struct {
            mboot_uint8_t framebuffer_red_field_position;
            mboot_uint8_t framebuffer_red_mask_size;
            mboot_uint8_t framebuffer_green_field_position;
            mboot_uint8_t framebuffer_green_mask_size;
            mboot_uint8_t framebuffer_blue_field_position;
            mboot_uint8_t framebuffer_blue_mask_size;
        };
    };
};

struct mboot_tag_elf_sections {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t num;
    mboot_uint32_t entsize;
    mboot_uint32_t shndx;
    char sections[];
};

struct mboot_tag_apm {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint16_t version;
    mboot_uint16_t cseg;
    mboot_uint32_t offset;
    mboot_uint16_t cseg_16;
    mboot_uint16_t dseg;
    mboot_uint16_t flags;
    mboot_uint16_t cseg_len;
    mboot_uint16_t cseg_16_len;
    mboot_uint16_t dseg_len;
};

struct mboot_tag_efi32 {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t pointer;
};

struct mboot_tag_efi64 {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint64_t pointer;
};

struct mboot_tag_smbios {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint8_t major;
    mboot_uint8_t minor;
    mboot_uint8_t reserved[6];
    mboot_uint8_t tables[];
};

struct mboot_tag_acpi {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint8_t rsdp[];
};

struct mboot_tag_network {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint8_t dhcpack[];
};

struct mboot_tag_efi_mmap {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t descr_size;
    mboot_uint32_t descr_vers;
    mboot_uint8_t efi_mmap[];
};

struct mboot_tag_efi32_ih {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t pointer;
};

struct mboot_tag_efi64_ih {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint64_t pointer;
};

struct mboot_tag_load_base_addr {
    mboot_uint32_t type;
    mboot_uint32_t size;
    mboot_uint32_t load_base_addr;
};

#endif /*  ! _ASM_FILE_ */

#endif /*  ! MBOOT_HEADER */
