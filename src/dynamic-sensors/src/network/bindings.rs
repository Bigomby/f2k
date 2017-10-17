use network::Network;

use libc::c_char;
use std::ffi::CStr;
use std::net::IpAddr;

#[no_mangle]
pub extern "C" fn network_new(network: &[u8; 16],
                              netmask: &[u8; 16],
                              name_ptr: *const c_char)
                              -> *mut Network {
    assert!(!name_ptr.is_null());

    unsafe {
        Box::into_raw(Box::new(Network::new(IpAddr::from(*network),
                                            IpAddr::from(*netmask),
                                            CStr::from_ptr(name_ptr)
                                                .to_str()
                                                .expect("Invalid string"))))
    }
}

#[no_mangle]
pub extern "C" fn network_get_ip_str(network_ptr: *const Network) -> *const c_char {
    assert!(!network_ptr.is_null());
    let network = unsafe { &*network_ptr };

    network.get_ip_str().as_bytes_with_nul().as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn network_get_name(network_ptr: *const Network) -> *const c_char {
    assert!(!network_ptr.is_null());
    let network = unsafe { &*network_ptr };

    network.get_name().as_bytes_with_nul().as_ptr() as *const c_char
}
