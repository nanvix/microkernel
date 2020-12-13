#
# MIT License
#
# Copyright(c) 2011-2020 The Maintainers of Nanvix
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

#===============================================================================
# Toolchain Configuration
#===============================================================================

# Compiler Options
ifneq ($(LIBLWIP),)
CFLAGS += -I $(INCDIR)/posix
endif

ifeq ($(ENABLE_STRESS_TESTS),yes)
CFLAGS += -D__ENABLE_STRESS_TESTS
endif

#===============================================================================
# Binaries Soucers and Objects
#===============================================================================

# Binary
ARCHIVE = $(LIBKERNEL)

# C Source Files
SRC = $(wildcard dev/*.c)       \
      $(wildcard dev/net/net.c) \
      $(wildcard init/*.c)      \
      $(wildcard mm/*.c)        \
      $(wildcard noc/*.c)       \
      $(wildcard pm/*.c)        \
      $(wildcard pm/thread/*.c) \
      $(wildcard pm/task/*.c)   \
      $(wildcard sys/*.c)       \
      $(wildcard test/*.c)

# Sources (Target Specific)
ifeq ($(TARGET),qemu-x86)
SRC += $(wildcard dev/net/rtl8139.c) \
       $(wildcard dev/bus/pci.c)
endif

ifeq ($(TARGET),qemu-openrisc)
SRC += $(wildcard dev/net/ethoc.c)
endif

# Object Files
OBJ = $(SRC:.c=.$(OBJ_SUFFIX).o)

#===============================================================================

include $(BUILDDIR)/makefile.archive
