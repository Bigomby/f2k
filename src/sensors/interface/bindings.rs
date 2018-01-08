use sensors::interface::Interface;

use libc::{size_t, c_char};

#[no_mangle]
pub extern "C" fn interface_new(id: u64,
                                name_ptr: *mut u8,
                                name_len: size_t,
                                description_ptr: *mut u8,
                                description_len: size_t)
                                -> *mut Interface {
    assert!(!name_ptr.is_null());
    assert!(!description_ptr.is_null());

    let mut name = Vec::with_capacity(name_len + 1);
    unsafe {
        for i in 0..name_len {
            name.push(*name_ptr.offset(i as isize));
        }
        name.push('\0' as u8);
    }

    let mut description = Vec::with_capacity(description_len + 1);
    unsafe {
        for i in 0..description_len {
            description.push(*description_ptr.offset(i as isize));
        }
        description.push('\0' as u8);
    }

    Box::into_raw(Box::new(Interface::new(id, name, description)))
}

#[no_mangle]
pub extern "C" fn interface_get_name(interface_ptr: *mut Interface) -> *const c_char {
    assert!(!interface_ptr.is_null());
    let interface = unsafe { &*interface_ptr };

    interface.get_name().as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn interface_get_description(interface_ptr: *mut Interface) -> *const c_char {
    assert!(!interface_ptr.is_null());
    let interface = unsafe { &*interface_ptr };

    interface.get_description().as_ptr() as *const c_char
}
