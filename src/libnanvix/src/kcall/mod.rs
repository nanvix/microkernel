/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

//==============================================================================
// Modules
//==============================================================================

mod void;

//==============================================================================
// Exports
//==============================================================================

pub use self::void::*;

//==============================================================================
// Imports
//==============================================================================

use core::arch;

//==============================================================================
// Enumerations
//==============================================================================

#[repr(C)]
pub enum KcallNumbers {
    Void0 = 0,
    Void1 = 1,
    Void2 = 2,
    Void3 = 3,
    Void4 = 4,
    Shutdown = 6,
    Write = 7,
    FrameAlloc = 8,
    FrameFree = 9,
    VmemCreate = 10,
    VmemRemove = 11,
    VmemMap = 12,
    VmemUnmap = 13,
    VmemControl = 14,
    VmemInfo = 15,
    KmodGet = 16,
    Spawn = 17,
    Semget = 18,
    Semop = 19,
    Semctl = 20,
    Boxtag = 25,
    MbIsAssigned = 26,
    MbOwner = 27,
    MbDefault = 28,
    MbAssign = 29,
    MbLink = 30,
    MbUnlink = 31,
    MbPush = 32,
    MbPop = 33,
}

//==============================================================================
// Private Standalone Functions
//==============================================================================

///
/// **Description**
///
/// Issues a kernel call with no arguments.
///
/// **Parameters**
/// - `kcall_nr` - Kernel call number.
///
/// **Return**
///
/// This function returns the value returned by the kernel call.
///
#[inline(never)]
pub unsafe fn kcall0(kcall_nr: u32) -> u32 {
    let ret: u32;
    arch::asm!("int 0x80",
        inout("eax") kcall_nr => ret,
        options(nostack, preserves_flags)
    );
    ret
}

///
/// **Description**
///
/// Issues a kernel call with one argument.
///
/// **Parameters**
/// - `kcall_nr` - Kernel call number.
/// - `arg0` - First argument for the kernel call.
///
/// **Return**
///
/// This function returns the value returned by the kernel call.
///
#[inline(never)]
pub unsafe fn kcall1(kcall_nr: u32, arg0: u32) -> u32 {
    let ret: u32;
    arch::asm!("int 0x80",
        inout("eax") kcall_nr => ret,
        in("ebx") arg0,
        options(nostack, preserves_flags)
    );
    ret
}

///
/// **Description**
///
/// Issues a kernel call with two arguments.
///
/// **Parameters**
/// - `kcall_nr` - Kernel call number.
/// - `arg0` - First argument for the kernel call.
/// - `arg1` - Second argument for the kernel call.
///
/// **Return**
///
/// This function returns the value returned by the kernel call.
///
#[inline(never)]
pub unsafe fn kcall2(kcall_nr: u32, arg0: u32, arg1: u32) -> u32 {
    let ret: u32;
    arch::asm!("int 0x80",
        inout("eax") kcall_nr => ret,
        in("ebx") arg0,
        in("ecx") arg1,
        options(nostack, preserves_flags)
    );
    ret
}

///
/// **Description**
///
/// Issues a kernel call with three arguments.
///
/// **Parameters**
/// - `kcall_nr` - Kernel call number.
/// - `arg0` - First argument for the kernel call.
/// - `arg1` - Second argument for the kernel call.
/// - `arg2` - Third argument for the kernel call.
///
/// **Return**
///
/// This function returns the value returned by the kernel call.
///
#[inline(never)]
pub unsafe fn kcall3(kcall_nr: u32, arg0: u32, arg1: u32, arg2: u32) -> u32 {
    let ret: u32;
    arch::asm!("int 0x80",
        inout("eax") kcall_nr => ret,
        in("ebx") arg0,
        in("ecx") arg1,
        in("edx") arg2,
        options(nostack, preserves_flags)
    );
    ret
}

///
/// **Description**
///
/// Issues a kernel call with four arguments.
///
/// **Parameters**
/// - `kcall_nr` - Kernel call number.
/// - `arg0` - First argument for the kernel call.
/// - `arg1` - Second argument for the kernel call.
/// - `arg2` - Third argument for the kernel call.
/// - `arg3` - Fourth argument for the kernel call.
///
/// **Return**
///
/// This function returns the value returned by the kernel call.
///
#[inline(never)]
pub unsafe fn kcall4(
    kcall_nr: u32,
    arg0: u32,
    arg1: u32,
    arg2: u32,
    arg3: u32,
) -> u32 {
    let ret: u32;
    arch::asm!("int 0x80",
        inout("eax") kcall_nr => ret,
        in("ebx") arg0,
        in("ecx") arg1,
        in("edx") arg2,
        in("edi") arg3,
        options(nostack, preserves_flags)
    );
    ret
}
