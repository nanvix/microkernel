# Copyright(c) 2011-2023 The Maintainers of Nanvix.
# Licensed under the MIT License.

#===============================================================================
# Artifacts
#===============================================================================

# C Source Files
SRC_C = $(wildcard hal/arch/*.c) \
		$(wildcard hal/*.c)      \
		$(wildcard lib/*.c)      \
		$(wildcard pm/*.c)       \
		$(wildcard mm/*.c)       \
		$(wildcard *.c)

# Asembly Source Files
SRC_ASM = $(wildcard hal/arch/*.S)

# Object Files
OBJ = $(SRC_ASM:.S=.$(TARGET).o) \
      $(SRC_C:.c=.$(TARGET).o)

# Libraries
export LIBS += $(LIBRARIES_DIR)/$(LIBDEV)
export LIBS += $(LIBRARIES_DIR)/$(LIBCORE)

#===============================================================================
# Build Rules
#===============================================================================

# Builds binary file.
all: $(OBJ)
ifeq ($(VERBOSE), no)
	@echo [CC] $(BINARIES_DIR)/$(KERNEL)
	@$(CC) $(LDFLAGS) -o $(BINARIES_DIR)/$(KERNEL) $(OBJ) $(LIBS) $(THEIR_LIBS)
else
	$(CC) $(LDFLAGS) -o $(BINARIES_DIR)/$(KERNEL) $(OBJ) $(LIBS) $(THEIR_LIBS)
endif

# Cleans build objects.
clean:
ifeq ($(VERBOSE), no)
	@echo [CLEAN] $(OBJ)
	@rm -rf $(OBJ)
	@echo [CLEAN] $(BINARIES_DIR)/$(KERNEL)
	@rm -rf $(BINARIES_DIR)/$(KERNEL)
else
	rm -rf $(OBJ)
	rm -rf $(BINARIES_DIR)/$(KERNEL)
endif

# Builds a C source file.
%.$(TARGET).o: %.c
ifeq ($(VERBOSE), no)
	@echo [CC] $@
	@$(CC) $(CFLAGS) $< -c -o $@
else
	$(CC) $(CFLAGS) $< -c -o $@
endif

# Builds an assembly source file.
%.$(TARGET).o: %.S
ifeq ($(VERBOSE), no)
	@echo [CC] $@
	@$(CC) $(CFLAGS) $< -c -o $@
else
	$(CC) $(CFLAGS) $< -c -o $@
endif