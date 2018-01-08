use sensors::sensor::Sensor;
use sensors::observation_id::ObservationID;

use libc::{c_char, c_void, size_t};

use std::ptr;

#[no_mangle]
pub extern "C" fn sensor_get_network_string(sensor_ptr: *const Sensor) -> *const c_char {
    let sensor = unsafe {
        assert!(!sensor_ptr.is_null());
        &*sensor_ptr
    };

    sensor.get_network_string().as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn sensor_get_observation_id(
    sensor_ptr: *mut Sensor,
    id: u32,
) -> *mut ObservationID {
    let sensor = unsafe {
        assert!(!sensor_ptr.is_null());
        &mut *sensor_ptr
    };

    match sensor.get_observation_id(id) {
        Some(observation_id) => observation_id as *mut ObservationID,
        None => ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn sensor_get_observation_id_list(
    sensor_ptr: *const Sensor,
    len: *mut size_t,
) -> *mut u32 {
    assert!(!sensor_ptr.is_null());
    let sensor = unsafe { &*sensor_ptr };

    let mut observation_id_list = sensor.list_observation_ids().into_boxed_slice();
    unsafe { *len = observation_id_list.len() as size_t };
    let sensor_list_raw = observation_id_list.as_mut_ptr();

    Box::into_raw(observation_id_list);
    sensor_list_raw
}

#[no_mangle]
pub extern "C" fn sensor_get_worker(sensor_ptr: *const Sensor) -> *mut c_void {
    let sensor = unsafe {
        assert!(!sensor_ptr.is_null());
        &*sensor_ptr
    };

    sensor
        .get_worker()
        .expect("No worker associated to the sensor")
}

#[no_mangle]
pub extern "C" fn sensor_set_worker(sensor_ptr: *mut Sensor, worker: *mut c_void) {
    let sensor = unsafe {
        assert!(!sensor_ptr.is_null());
        &mut *sensor_ptr
    };

    sensor.set_worker(worker);
}

#[no_mangle]
pub extern "C" fn sensor_add_observation_id(
    sensor_ptr: *mut Sensor,
    observation_id_ptr: *mut ObservationID,
) {
    let sensor = unsafe {
        assert!(!sensor_ptr.is_null());
        &mut *sensor_ptr
    };

    let observation_id = unsafe {
        assert!(!observation_id_ptr.is_null());
        Box::from_raw(observation_id_ptr)
    };

    sensor.add_observation_id(*observation_id);
}

#[no_mangle]
pub extern "C" fn sensor_add_default_observation_id(
    sensor_ptr: *mut Sensor,
    observation_id_ptr: *mut ObservationID,
) {
    let sensor = unsafe {
        assert!(!sensor_ptr.is_null());
        &mut *sensor_ptr
    };

    let observation_id = unsafe {
        assert!(!observation_id_ptr.is_null());
        Box::from_raw(observation_id_ptr)
    };

    sensor.add_default_observation_id(*observation_id);
}
