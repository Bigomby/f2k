use super::FileLoader;
use sensors::sensor::Sensor;

use std::ffi::CStr;

use libc::{c_char, c_void};

#[no_mangle]
pub extern "C" fn db_loader_new_file_loader() -> *mut FileLoader {
    Box::into_raw(Box::new(FileLoader::new()))
}

#[no_mangle]
pub extern "C" fn db_loader_set_new_sensor_event(
    file_loader_ptr: *mut FileLoader,
    event_cb: extern "C" fn(*mut Sensor, *mut c_void),
    ctx: *mut c_void,
) {
    assert!(!file_loader_ptr.is_null());
    let file_loader = unsafe { &mut *file_loader_ptr };

    file_loader.set_new_sensor_event(
        Box::new(move |sensor, ctx| {
            event_cb(Box::into_raw(Box::new(sensor)), ctx);
        }),
        ctx,
    );
}

#[no_mangle]
pub extern "C" fn db_loader_load_file(file_loader_ptr: *mut FileLoader, filename: *const c_char) {
    assert!(!file_loader_ptr.is_null());
    assert!(!filename.is_null());

    let file_loader = unsafe { &mut *file_loader_ptr };
    let file = unsafe { CStr::from_ptr(filename) };

    file_loader.load(file.to_str().unwrap());
}

#[no_mangle]
pub extern "C" fn db_loader_file_loader_destroy(file_loader_ptr: *mut FileLoader) {
    if file_loader_ptr.is_null() {
        return;
    }

    unsafe { Box::from_raw(file_loader_ptr) };
}
