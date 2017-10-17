#![feature(i128_type)]

extern crate libc;

pub mod application;
pub mod database;
pub mod sensor;
pub mod observation_id;
pub mod selector;
pub mod network;
pub mod interface;
pub mod util;

#[repr(C)]
#[derive(Clone, Copy, PartialEq, Debug, Default)]
pub struct NetAddress {
    network: [u8; 16],
    netmask: [u8; 16],
    broadcast: [u8; 16],
}
