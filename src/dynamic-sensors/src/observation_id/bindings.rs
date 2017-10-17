use observation_id::ObservationID;
use selector::Selector;
use application::Application;
use network::Network;
use interface::Interface;

use libc::{c_char, c_void, size_t};
use std::ffi::CStr;
use std::ptr;
use std::net::IpAddr;

#[no_mangle]
pub extern "C" fn observation_id_new(id: u32) -> *mut ObservationID {
    Box::into_raw(Box::new(ObservationID::new(id)))
}

#[no_mangle]
pub extern "C" fn observation_id_get_id(observation_id_ptr: *const ObservationID) -> u32 {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    observation_id.get_id()
}

#[no_mangle]
pub extern "C" fn observation_id_get_selector(observation_id_ptr: *const ObservationID,
                                              selector_id: u64)
                                              -> *const Selector {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    match observation_id.get_selector(selector_id) {
        Some(selector) => selector as *const Selector,
        None => ptr::null(),
    }
}

#[no_mangle]
pub extern "C" fn observation_id_get_fallback_first_switch(
    observation_id_ptr: *const ObservationID,
) -> i64 {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    match observation_id.get_fallback_first_switch() {
        Some(fallback_first_switch) => fallback_first_switch,
        None => 0,
    }
}

#[no_mangle]
pub extern "C" fn observation_id_list_templates(observation_id_ptr: *const ObservationID,
                                                len: *mut size_t)
                                                -> *mut u16 {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    let mut template_list = observation_id.list_templates().into_boxed_slice();
    unsafe {
        if template_list.len() > 0 {
            *len = template_list.len() as size_t;
        } else {
            *len = 0;
            return ptr::null_mut();
        }
    };

    let sensor_list_raw = template_list.as_mut_ptr();
    Box::into_raw(template_list);
    sensor_list_raw
}

#[no_mangle]
pub extern "C" fn observation_id_get_template(observation_id_ptr: *const ObservationID,
                                              id: u16)
                                              -> *mut c_void {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    match observation_id.get_template(id) {
        Some(template) => *template,
        None => ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn observation_id_get_application(observation_id_ptr: *const ObservationID,
                                                 application_id: u64)
                                                 -> *const Application {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    match observation_id.get_application(application_id) {
        Some(application) => application as *const Application,
        None => ptr::null(),
    }
}

#[no_mangle]
pub extern "C" fn observation_id_get_network(observation_id_ptr: *const ObservationID,
                                             ip: &[u8; 16])
                                             -> *const Network {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    let ip_address = IpAddr::from(*ip);
    match observation_id.get_network(ip_address) {
        Some(network) => network as *const Network,
        None => ptr::null(),
    }
}

#[no_mangle]
pub extern "C" fn observation_id_get_interface(observation_id_ptr: *const ObservationID,
                                               interface_id: u64)
                                               -> *const Interface {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    match observation_id.get_interface(interface_id) {
        Some(interface) => interface as *const Interface,
        None => ptr::null(),
    }
}

#[no_mangle]
pub extern "C" fn observation_id_get_enrichment(observation_id_ptr: *const ObservationID)
                                                -> *const c_char {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    match observation_id.get_enrichment() {
        Some(enrichment) => (*enrichment).as_ptr() as *const c_char,
        None => ptr::null(),
    }
}

#[no_mangle]
pub extern "C" fn observation_id_want_client_dns(observation_id_ptr: *const ObservationID) -> bool {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    observation_id.want_client_dns()
}

#[no_mangle]
pub extern "C" fn observation_id_want_target_dns(observation_id_ptr: *const ObservationID) -> bool {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    observation_id.want_target_dns()
}

#[no_mangle]
pub extern "C" fn observation_id_is_exporter_in_wan_side(observation_id_ptr: *const ObservationID)
                                                         -> bool {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    observation_id.is_exporter_in_wan_side()
}

#[no_mangle]
pub extern "C" fn observation_id_is_span_port(observation_id_ptr: *const ObservationID) -> bool {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &*observation_id_ptr };

    observation_id.is_span_port()
}

#[no_mangle]
pub extern "C" fn observation_id_add_template(observation_id_ptr: *mut ObservationID,
                                              id: u16,
                                              template_ptr: *mut c_void) {
    assert!(!observation_id_ptr.is_null());
    assert!(!template_ptr.is_null());
    let mut observation_id = unsafe { &mut *observation_id_ptr };
    let template = unsafe { &mut *template_ptr };

    observation_id.add_template(id, template);
}

#[no_mangle]
pub extern "C" fn observation_id_add_application(observation_id_ptr: *mut ObservationID,
                                                 application_ptr: *mut Application) {
    assert!(!observation_id_ptr.is_null());
    assert!(!application_ptr.is_null());
    let mut observation_id = unsafe { &mut *observation_id_ptr };
    let application = unsafe { Box::from_raw(application_ptr) };

    observation_id.add_application(*application);
}

#[no_mangle]
pub extern "C" fn observation_id_add_selector(observation_id_ptr: *mut ObservationID,
                                              selector_ptr: *mut Selector) {
    assert!(!observation_id_ptr.is_null());
    let mut observation_id = unsafe { &mut *observation_id_ptr };
    let selector = unsafe { Box::from_raw(selector_ptr) };

    observation_id.add_selector(*selector);
}

#[no_mangle]
pub extern "C" fn observation_id_add_interface(observation_id_ptr: *mut ObservationID,
                                               interface_ptr: *mut Interface) {
    assert!(!observation_id_ptr.is_null());
    assert!(!interface_ptr.is_null());
    let mut observation_id = unsafe { &mut *observation_id_ptr };
    let interface = unsafe { Box::from_raw(interface_ptr) };

    observation_id.add_interface(*interface);
}

#[no_mangle]
pub extern "C" fn observation_id_add_network(observation_id_ptr: *mut ObservationID,
                                             network_ptr: *mut Network) {
    assert!(!observation_id_ptr.is_null());
    assert!(!network_ptr.is_null());
    let mut observation_id = unsafe { &mut *observation_id_ptr };
    let network = unsafe { Box::from_raw(network_ptr) };

    observation_id.add_network(*network);
}

#[no_mangle]
pub extern "C" fn observation_id_set_enrichment(observation_id_ptr: *mut ObservationID,
                                                enrichment_ptr: *mut c_char) {
    assert!(!observation_id_ptr.is_null());
    let observation_id = unsafe { &mut *observation_id_ptr };
    let enrichment = unsafe { CStr::from_ptr(enrichment_ptr) };
    observation_id.set_enrichment(enrichment.to_bytes_with_nul());
}

#[no_mangle]
pub extern "C" fn observation_id_set_exporter_in_wan_side(observation_id_ptr: *mut ObservationID) {
    let observation_id = unsafe {
        assert!(!observation_id_ptr.is_null());
        &mut *observation_id_ptr
    };

    observation_id.set_exporter_in_wan_side();
}

#[no_mangle]
pub extern "C" fn observation_id_set_span_mode(observation_id_ptr: *mut ObservationID) {
    let observation_id = unsafe {
        assert!(!observation_id_ptr.is_null());
        &mut *observation_id_ptr
    };

    observation_id.set_span_mode();
}


#[no_mangle]
pub extern "C" fn observation_id_set_fallback_first_switch(observation_id_ptr: *mut ObservationID,
                                                           fallback_first_switch: i64) {
    let observation_id = unsafe {
        assert!(!observation_id_ptr.is_null());
        &mut *observation_id_ptr
    };

    observation_id.set_fallback_first_switch(fallback_first_switch);
}

#[no_mangle]
pub extern "C" fn observation_id_enable_ptr_dns_client(observation_id_ptr: *mut ObservationID) {
    let observation_id = unsafe {
        assert!(!observation_id_ptr.is_null());
        &mut *observation_id_ptr
    };

    observation_id.enable_ptr_dns_client();
}

#[no_mangle]
pub extern "C" fn observation_id_enable_ptr_dns_target(observation_id_ptr: *mut ObservationID) {
    let observation_id = unsafe {
        assert!(!observation_id_ptr.is_null());
        &mut *observation_id_ptr
    };

    observation_id.enable_ptr_dns_target();
}
