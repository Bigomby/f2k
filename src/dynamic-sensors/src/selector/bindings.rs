use selector::Selector;

use libc::{size_t, c_char};

#[no_mangle]
pub extern "C" fn selector_new(id: u64, name_ptr: *mut u8, name_len: size_t) -> *mut Selector {
    assert!(!name_ptr.is_null());

    let mut name = Vec::with_capacity(name_len + 1);
    unsafe {
        for i in 0..name_len {
            name.push(*name_ptr.offset(i as isize));
        }
        name.push('\0' as u8);
    }

    Box::into_raw(Box::new(Selector::new(id, name)))
}

#[no_mangle]
pub extern "C" fn selector_get_name(selector_ptr: *const Selector) -> *const c_char {
    assert!(!selector_ptr.is_null());
    let selector = unsafe { &*selector_ptr };

    selector.get_name().as_ptr() as *const c_char
}
