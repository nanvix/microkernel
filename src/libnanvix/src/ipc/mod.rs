/*
 * Copyright(c) 2011-2024 The Maintainers of Nanvix.
 * Licensed under the MIT License.
 */

//==============================================================================
// Imports
//==============================================================================

use crate::kcall::{
    self,
    KcallNumbers,
};

/// **Description**
///
/// Get mailbox tag.
///
/// **Parameters**
///
/// - `image` - Image of the target process.
///
/// **Return**
///
/// Upon successful completion, the mailbox tag is returned.
/// Upon failure, a negative error code is returned instead.
pub fn mailbox_tag(mbxid: i32) -> i32 {
    unsafe { kcall::kcall1(KcallNumbers::Boxtag as u32, mbxid as u32) as i32 }
}

/// Comments to be defined
pub fn mailbox_is_assigned(mbxid: i32) -> i32 {
    unsafe {kcall::kcall1(KcallNumbers::MbIsAssigned as u32, mbxid as u32) as i32}
}

/// Comments to be defined
pub fn mailbox_owner(mbxid: i32) -> i32 {
    unsafe{kcall::kcall1(KcallNumbers::MbOwner as u32, mbxid as u32) as i32}
}

/// Comments to be defined
pub fn mailbox_default(mbxid: i32) -> i32 {
    unsafe{kcall::kcall1(KcallNumbers::MbDefault as u32, mbxid as u32) as i32}
}

//Comments to be defined
pub fn mailbox_assign(mbxid: i32, owner: i32, tag: i32) -> i32 {
    unsafe{kcall::kcall3(KcallNumbers::MbAssign as u32, mbxid as u32, owner as u32, tag as u32) as i32}
}

//Comments to be defined
pub fn mailbox_link(mbxid: i32) -> i32 {
    unsafe{kcall::kcall1(KcallNumbers::MbLink as u32, mbxid as u32) as i32}
}

//Comments to be defined
pub fn mailbox_unlink(mbxid: i32) -> i32 {
    unsafe{kcall::kcall1(KcallNumbers::MbUnlink as u32, mbxid as u32) as i32}
}

//Comments to be defined
pub fn mailbox_push(mbxid: i32, msg: &i32, sz: i32) -> i32 {
    unsafe{kcall::kcall3(KcallNumbers::MbAssign as u32, mbxid as u32, *msg as u32, sz as u32) as i32}
}

//Comments to be defined
pub fn mailbox_pop(mbxid: i32, msg: &i32, sz: i32) -> i32 {
    unsafe{kcall::kcall3(KcallNumbers::MbAssign as u32, mbxid as u32, *msg as u32, sz as u32) as i32}
}
