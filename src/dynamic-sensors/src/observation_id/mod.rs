pub mod bindings;

use application::Application;
use selector::Selector;
use network::Network;
use interface::Interface;
use util::apply_netmask;

use libc::c_void;
use std::collections::HashMap;
use std::net::IpAddr;

pub struct ObservationID {
    id: u32,
    enrichment: Option<Vec<u8>>,
    fallback_first_switch: Option<i64>,
    applications: HashMap<u64, Application>,
    networks: HashMap<IpAddr, Network>,
    selectors: HashMap<u64, Selector>,
    interfaces: HashMap<u64, Interface>,
    templates: HashMap<u16, *mut c_void>,
    want_client_dns: bool,
    want_target_dns: bool,
    ptr_dns_target: bool,
    ptr_dns_client: bool,
    exporter_in_wan_side: bool,
    span_port: bool,
}

impl ObservationID {
    pub fn new(id: u32) -> ObservationID {
        ObservationID {
            id: id,
            enrichment: None,
            fallback_first_switch: None,
            applications: HashMap::new(),
            networks: HashMap::new(),
            selectors: HashMap::new(),
            interfaces: HashMap::new(),
            templates: HashMap::new(),
            want_client_dns: false,
            want_target_dns: false,
            ptr_dns_target: false,
            ptr_dns_client: false,
            exporter_in_wan_side: false,
            span_port: false,
        }
    }

    pub fn get_id(&self) -> u32 {
        self.id
    }

    pub fn get_network(&self, ip: IpAddr) -> Option<&Network> {
        let ip = if let IpAddr::V6(ipv6) = ip {
            match ipv6.to_ipv4() {
                Some(ipv4) => IpAddr::from(ipv4),
                None => IpAddr::from(ipv6),
            }
        } else {
            ip
        };

        for home_net in self.networks.values() {

            if ip.is_ipv4() && home_net.get_netmask().is_ipv4() ||
               ip.is_ipv6() && home_net.get_netmask().is_ipv6() {
                let network = apply_netmask(&ip, &home_net.get_netmask());
                if let Some(network) = self.networks.get(&network) {
                    return Some(network);
                };
            }
        }

        None
    }

    pub fn list_templates(&self) -> Vec<u16> {
        let mut templates = Vec::new();
        for template in self.templates.keys() {
            templates.push(*template);
        }

        templates
    }

    pub fn get_template(&self, id: u16) -> Option<&*mut c_void> {
        match self.templates.get(&id) {
            Some(template) => Some(template),
            None => None,
        }
    }

    pub fn get_enrichment(&self) -> Option<&[u8]> {
        match self.enrichment {
            Some(ref enrichment) => Some(enrichment),
            None => None,
        }
    }

    pub fn get_selector(&self, id: u64) -> Option<&Selector> {
        self.selectors.get(&id)
    }

    pub fn get_interface(&self, id: u64) -> Option<&Interface> {
        self.interfaces.get(&id)
    }

    pub fn get_application(&self, id: u64) -> Option<&Application> {
        self.applications.get(&id)
    }

    pub fn get_fallback_first_switch(&self) -> Option<i64> {
        self.fallback_first_switch
    }

    pub fn want_client_dns(&self) -> bool {
        self.want_client_dns
    }

    pub fn want_target_dns(&self) -> bool {
        self.want_target_dns
    }

    pub fn is_exporter_in_wan_side(&self) -> bool {
        self.exporter_in_wan_side
    }

    pub fn is_span_port(&self) -> bool {
        self.span_port
    }

    pub fn add_selector(&mut self, selector: Selector) {
        self.selectors.insert(selector.get_id(), selector);
    }

    pub fn add_application(&mut self, application: Application) {
        self.applications.insert(application.get_id(), application);
    }

    pub fn add_interface(&mut self, interface: Interface) {
        self.interfaces.insert(interface.get_id(), interface);
    }

    pub fn add_network(&mut self, network: Network) {
        self.networks.insert(*network.get_ip(), network);
    }

    pub fn add_template(&mut self, id: u16, template: *mut c_void) {
        self.templates.insert(id, template);
    }

    pub fn set_enrichment(&mut self, enrichment: &[u8]) {
        self.enrichment = Some(Vec::from(enrichment));
    }

    pub fn set_fallback_first_switch(&mut self, fallback_first_switch: i64) {
        self.fallback_first_switch = Some(fallback_first_switch);
    }

    pub fn set_exporter_in_wan_side(&mut self) {
        self.exporter_in_wan_side = true;
    }

    pub fn set_span_mode(&mut self) {
        self.span_port = true;
    }

    pub fn enable_ptr_dns_target(&mut self) {
        self.ptr_dns_target = true;
    }

    pub fn enable_ptr_dns_client(&mut self) {
        self.ptr_dns_client = true;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::str::FromStr;

    #[repr(C)]
    #[derive(Debug, PartialEq)]
    struct Template {
        example_data_1: [u8; 16],
        example_data_2: [u32; 4],
        example_data_3: String,
    }

    #[test]
    fn test_add_template() {
        let template = Box::new(Template {
            example_data_1: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
            example_data_2: [10, 20, 30, 50],
            example_data_3: String::from("Hello world"),
        });

        let mut observation_id = ObservationID::new(1234);
        let raw_data = Box::into_raw(template) as *mut c_void;

        observation_id.add_template(42, raw_data);
        let template_ref = observation_id.get_template(42);

        let tmpl = *template_ref.unwrap() as *mut Template;

        unsafe {
            assert_eq!((*tmpl).example_data_1,
                       [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]);
            assert_eq!((*tmpl).example_data_2, [10, 20, 30, 50]);
            assert_eq!((*tmpl).example_data_3, "Hello world");
        }
    }

    #[test]
    fn test_networks() {
        let mut observation_id = ObservationID::new(1234);

        let network = Network::new(IpAddr::from_str("10.13.30.0").unwrap(),
                                   IpAddr::from_str("255.255.0.0").unwrap(),
                                   "test-net");

        observation_id.add_network(network);

        let should_exists = observation_id.get_network(IpAddr::from_str("10.13.30.44").unwrap());
        assert!(should_exists.is_some());
    }
}
