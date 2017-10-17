use application::Application;

use libc::{size_t, c_char};

#[no_mangle]
pub extern "C" fn application_new(id: u64,
                                  name_ptr: *mut u8,
                                  name_len: size_t)
                                  -> *mut Application {
    assert!(!name_ptr.is_null());

    let mut name = Vec::with_capacity(name_len + 1);
    unsafe {
        for i in 0..name_len {
            name.push(*name_ptr.offset(i as isize));
        }
        name.push('\0' as u8);
    }

    Box::into_raw(Box::new(Application::new(id, name)))
}

#[no_mangle]
pub extern "C" fn application_get_name(application_ptr: *mut Application) -> *const c_char {
    assert!(!application_ptr.is_null());
    let application = unsafe { &*application_ptr };

    application.get_name().as_ptr() as *const c_char
}
