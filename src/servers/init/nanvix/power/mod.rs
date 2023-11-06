/*
 * Copyright(c) 2011-2023 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

//==============================================================================
// Imports
//==============================================================================

use nanvix::{
    kcall,
    kcall::KcallNumbers,
};

//==============================================================================
// Public Standalone Functions
//==============================================================================

///
/// **Description**
///
/// Shutdowns the system.
///
pub fn shutdown() -> ! {
    unsafe {
        kcall::kcall0(KcallNumbers::Shutdown as u32);
    }
    // Never gets here.
    loop {}
}
