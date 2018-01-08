use sensors::sensors_db::SensorsDB;
use sensors::sensor::Sensor;
use util::normalize_address;

use libc::size_t;

use std::net::IpAddr;
use std::ptr;

#[no_mangle]
pub extern "C" fn sensors_db_new() -> *mut SensorsDB {
    Box::into_raw(Box::new(SensorsDB::new()))
}

#[no_mangle]
pub extern "C" fn sensors_db_get(
    database_ptr: *mut SensorsDB,
    sensor_ip: &[u8; 16],
) -> *mut Sensor {
    assert!(!database_ptr.is_null());

    let database = unsafe { &mut *database_ptr };
    let address = normalize_address(IpAddr::from(*sensor_ip));

    match database.get_sensor(address) {
        None => ptr::null_mut(),
        Some(sensor) => sensor as *mut Sensor,
    }
}

#[no_mangle]
pub extern "C" fn sensors_db_add(database_ptr: *mut SensorsDB, sensor_ptr: *mut Sensor) {
    assert!(!database_ptr.is_null());
    let database = unsafe { &mut *database_ptr };
    if sensor_ptr.is_null() {
        return;
    }

    let sensor = unsafe { Box::from_raw(sensor_ptr) };
    database.add_sensor(*sensor);
}

#[no_mangle]
pub extern "C" fn sensors_db_list(
    database_ptr: *const SensorsDB,
    len: *mut size_t,
) -> *mut *const Sensor {
    assert!(!database_ptr.is_null());
    let database = unsafe { &*database_ptr };

    let sensor_list = database.list_sensors();
    let mut sensors_ptrs = Vec::new();
    for sensor in sensor_list {
        sensors_ptrs.push(sensor as *const Sensor);
    }

    unsafe { *len = sensors_ptrs.len() as size_t };
    let mut raw_slice = sensors_ptrs.into_boxed_slice();
    let raw_ptr = raw_slice.as_mut_ptr();
    Box::into_raw(raw_slice);

    raw_ptr
}

#[no_mangle]
pub extern "C" fn sensors_db_destroy(database_ptr: *mut SensorsDB) {
    if database_ptr.is_null() {
        return;
    }

    unsafe { Box::from_raw(database_ptr) };
}
