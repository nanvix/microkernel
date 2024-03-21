/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

 //==============================================================================
// Modules
//==============================================================================

mod constants;

//==============================================================================
// Imports
//==============================================================================

use crate::kcall::{
    self,
    KcallNumbers,
};

//==============================================================================
// Exports
//==============================================================================

pub use self::constants::*;

// Gets the owner of a mailbox
pub fn mailbox_owner(mbxid: i32) -> i32 {
    unsafe{kcall::kcall1(KcallNumbers::MbOwner as u32, mbxid as u32) as i32}
}

// Assign a mailbox
pub fn mailbox_assign(mbxid: i32, owner: i32, tag: i32) -> i32 {
    unsafe{kcall::kcall3(KcallNumbers::MbAssign as u32, mbxid as u32, owner as u32, tag as u32) as i32}
}

// Links a mailbox
pub fn mailbox_link(mbxid: i32) -> i32 {
    unsafe{kcall::kcall1(KcallNumbers::MbLink as u32, mbxid as u32) as i32}
}

// Unlinks a mailbox
pub fn mailbox_unlink(mbxid: i32) -> i32 {
    unsafe{kcall::kcall1(KcallNumbers::MbUnlink as u32, mbxid as u32) as i32}
}

// Add a message to a mailbox
pub fn mailbox_push(mbxid: i32, msg: &i32, sz: i32) -> i32 {
    unsafe{kcall::kcall3(KcallNumbers::MbAssign as u32, mbxid as u32, *msg as u32, sz as u32) as i32}
}

// Remove a message from a mailbox
pub fn mailbox_pop(mbxid: i32, msg: &i32, sz: i32) -> i32 {
    unsafe{kcall::kcall3(KcallNumbers::MbAssign as u32, mbxid as u32, *msg as u32, sz as u32) as i32}
}
