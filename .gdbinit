# Copyright(c) 2011-2024 The Maintainers of Nanvix.
# Licensed under the MIT License.

layout split
target remote tcp::1234
file bin/nanvix.elf
symbol-file bin/nanvix.elf
add-symbol-file bin/init.elf
add-symbol-file bin/test.elf
handle SIGSEGV nostop noprint nopass
set confirm off
set scheduler-locking step
focus cmd
set detach-on-fork
b kmain

define hook-stop
	if $_isvoid ($_exitcode) != 1
		quit
	end

	focus cmd
end
