# Development Environment Setup

This document instructs you on how to setup your development environment.

> ⚠️ Follow these instructions on a fresh Ubuntu 22.04 system.

> ⚠️ These instructions assume that you have superuser privileges on the machine.

## 1. Clone this Repository

```bash
export WORKSPACE=$HOME/nanvix                        # Change this if you want.
mkdir -p $WORKDIR                                    # Create workspace.
cd $WORKDIR                                          # Switch to workspace.
git clone https://github.com/nanvix/microkernel.git  # Clone repository.
cd microkernel                                       # Switch to source tree.
```

## 2. Install Dependencies

```bash
cat scripts/setup/dependencies.sh    # Inspect what is going to be installed.
sudo -E ./scripts/setup/qemu.sh x86  # Install dependencies.
```

## 3. Build Toolchain

> ⚠️ This step may take some time to complete.

```bash
export TARGET=x86                     # Select x86 as your target architecture.
./scripts/setup/toolchain.sh $TARGET  # Build GCC, Binutils, and GDB.
```

## 4. Build QEMU

> ⚠️ This step may take some time to complete.

```bash
export TARGET=x86                # Select x86 as your target architecture.
./scripts/setup/qemu.sh $TARGET  # Build QEMU.
```