#
# MIT License
#
# Copyright(c) 2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
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

#
# Default make rule.
#
.DEFAULT_GOAL := all

#===============================================================================
# Build Options
#===============================================================================

# Verbose Build?
export VERBOSE ?= no

# Release Version?
export RELEASE ?= no

# Installation Prefix
export PREFIX ?= $(HOME)

# Use Docker?
export DOCKER ?= no

# Stall regression tests?
export SUPPRESS_TESTS ?= no

# Extra
export ADDONS ?=

#===============================================================================
# Directories
#===============================================================================

export ROOTDIR    := $(CURDIR)
export BINDIR     := $(ROOTDIR)/bin
export BUILDDIR   := $(ROOTDIR)/build
export CONTRIBDIR := $(ROOTDIR)/contrib
export DOCDIR     := $(ROOTDIR)/doc
export IMGDIR     := $(ROOTDIR)/img
export INCDIR     := $(ROOTDIR)/include
export LIBDIR     := $(ROOTDIR)/lib
export LINKERDIR  := $(BUILDDIR)/$(TARGET)/linker
export MAKEDIR    := $(BUILDDIR)/$(TARGET)/make
export SRCDIR     := $(ROOTDIR)/src
export TOOLSDIR   := $(ROOTDIR)/utils

#===============================================================================
# Libraries and Binaries
#===============================================================================

# Libraries
export BARELIB   := barelib-$(TARGET).a
export LIBHAL    := libhal-$(TARGET).a
export LIBKERNEL := libkernel-$(TARGET).a

#===============================================================================
# Target-Specific Make Rules
#===============================================================================

ifeq ($(DOCKER),no)
include $(MAKEDIR)/makefile.microkernel
endif

#===============================================================================
# Toolchain Configuration
#===============================================================================

# Compiler Options
export CFLAGS += -std=c99 -fno-builtin
export CFLAGS += -Wall -Wextra -Werror -Wa,--warn
export CFLAGS += -Winit-self -Wswitch-default -Wfloat-equal
export CFLAGS += -Wundef -Wshadow -Wuninitialized -Wlogical-op
export CFLAGS += -Wvla # -Wredundant-decls
export CFLAGS += -Wno-missing-profile
export CFLAGS += -fno-stack-protector
export CFLAGS += -Wno-unused-function
export CFLAGS += -I $(INCDIR)
export CFLAGS += -I $(ROOTDIR)/src/lwip/src/include
export CFLAGS += -D__NANVIX_MICROKERNEL
export CFLAGS += $(ADDONS)

# Enable sync and portal implementation that uses mailboxes
export CFLAGS += -D__NANVIX_IKC_USES_ONLY_MAILBOX=0

# Additional C Flags
include $(BUILDDIR)/makefile.cflags

# Archiver Options
export ARFLAGS = rc

#===============================================================================

# Image Source
export IMGSRC = $(IMGDIR)/$(TARGET).img

# Image Name
export IMAGE = microkernel-debug.img

# Builds everything.
all: | make-dirs image

# Make Directories
make-dirs:
	@mkdir -p $(BINDIR)
	@mkdir -p $(LIBDIR)

ifeq ($(DOCKER),no)
# Builds image.
image: all-target
	@bash $(TOOLSDIR)/nanvix-build-image.sh $(IMAGE) $(BINDIR) $(IMGSRC)

# Cleans build.
clean: clean-target

# Cleans everything.
distclean: distclean-target
	@rm -rf $(IMAGE) $(BINDIR)/$(EXECBIN) $(LIBDIR)/$(LIBKERNEL)

# Run Docker containers.
else
image-tests: all-targets
	cd $(ROOTDIR)/docker && docker-compose -f image-tests.yml up $(TARGET)

# Cleans builds.
clean:
	cd $(ROOTDIR)/docker && docker-compose -f clean.yml up $(TARGET)

# Cleans everything.
distclean:
	cd $(ROOTDIR)/docker && docker-compose -f distclean.yml up $(TARGET)
endif

#===============================================================================
# Contrib Install and Uninstall Rules
#===============================================================================

include $(BUILDDIR)/makefile.microkernel

#===============================================================================
# Install and Uninstall Rules
#===============================================================================

include $(BUILDDIR)/makefile.install

#===============================================================================
# Debug and Run Rules
#===============================================================================

include $(BUILDDIR)/makefile.run
