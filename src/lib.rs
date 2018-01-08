#![feature(i128_type)]

extern crate libc;
extern crate serde;
extern crate serde_json;

pub mod sensors;
pub mod db_loader;
pub mod util;

#[repr(C)]
#[derive(Clone, Copy, PartialEq, Debug, Default)]
pub struct NetAddress {
    network: [u8; 16],
    netmask: [u8; 16],
    broadcast: [u8; 16],
}
