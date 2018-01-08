pub mod bindings;

use sensors::observation_id::ObservationID;
use util::{apply_netmask, normalize_address};

use libc::c_void;

use std::collections::HashMap;
use std::net::IpAddr;

pub struct Sensor {
    network: IpAddr,
    netmask: IpAddr,
    str_network: String,
    worker: Option<*mut c_void>,
    default_observation_id: Option<ObservationID>,
    observation_id: HashMap<u32, ObservationID>,
}

impl Sensor {
    pub fn new(address: IpAddr, netmask: IpAddr) -> Self {
        let address = normalize_address(address);
        let netmask = normalize_address(netmask);
        if address.is_ipv4() && netmask.is_ipv6() || address.is_ipv6() && netmask.is_ipv4() {
            panic!("IP versions does not match");
        }

        let network = apply_netmask(&address, &netmask);

        Sensor {
            network: network,
            netmask: netmask,
            str_network: format!("{}", IpAddr::from(network)),
            worker: None,
            default_observation_id: None,
            observation_id: HashMap::new(),
        }
    }

    pub fn get_network(&self) -> IpAddr {
        self.network
    }

    pub fn get_netmask(&self) -> IpAddr {
        self.netmask
    }

    pub fn get_network_string(&self) -> &str {
        &self.str_network
    }

    pub fn get_worker(&self) -> Option<*mut c_void> {
        self.worker
    }

    pub fn list_observation_ids(&self) -> Vec<u32> {
        let mut observation_id_list = Vec::new();
        for observation_id in self.observation_id.keys() {
            observation_id_list.push(*observation_id);
        }

        match self.default_observation_id {
            Some(ref default_observation_id) => {
                observation_id_list.push(default_observation_id.get_id())
            }
            None => {}
        }

        observation_id_list
    }

    pub fn get_observation_id(&mut self, id: u32) -> Option<&mut ObservationID> {
        match self.observation_id.get_mut(&id) {
            Some(observation_id) => Some(observation_id),
            None => self.default_observation_id.as_mut(),
        }
    }

    pub fn set_worker(&mut self, worker: *mut c_void) {
        self.worker = Some(worker);
    }

    pub fn add_observation_id(&mut self, observation_id: ObservationID) {
        self.observation_id
            .insert(observation_id.get_id(), observation_id);
    }

    pub fn add_default_observation_id(&mut self, observation_id: ObservationID) {
        self.default_observation_id = Some(observation_id);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::{Ipv4Addr, Ipv6Addr};
    use std::str::FromStr;

    #[test]
    fn create_sensor_with_ipv4() {
        let ip = Ipv4Addr::from(3232235901);
        let netmask = Ipv4Addr::from(0xFFFFFF00);

        let sensor = Sensor::new(IpAddr::from(ip), IpAddr::from(netmask));
        let network = sensor.get_network();

        assert!(network.is_ipv4());
        assert_eq!("192.168.1.0", format!("{}", network));
    }

    #[test]
    fn create_sensor_with_ipv6() {
        let ip = Ipv6Addr::from_str("2001:db8:a0b:12f0::1").expect("Error parsing IP");
        let netmask = Ipv6Addr::from_str("FFFF:FFFF::").expect("Error parsing netmask");

        let sensor = Sensor::new(IpAddr::from(ip), IpAddr::from(netmask));
        let network = sensor.get_network();

        assert!(network.is_ipv6());
        assert_eq!("2001:db8::", format!("{}", network));
    }

    // #[test]
    // fn add_observation_ids() {
    //     let mut sensor = Sensor::new(
    //         IpAddr::from(Ipv4Addr::from(3232235901)),
    //         IpAddr::from(Ipv4Addr::from(0xFFFFFF00)),
    //     );
    //
    //     let observation_id_1 = ObservationID::new(0);
    //     let observation_id_2 = ObservationID::new(123);
    //
    //     sensor.add_default_observation_id(observation_id_1);
    //     sensor.add_observation_id(observation_id_2);
    //
    //     assert_eq!(sensor.get_observation_id(0).unwrap().get_id(), 0);
    //     assert_eq!(sensor.get_observation_id(123).unwrap().get_id(), 123);
    //     assert_eq!(sensor.get_observation_id(456).unwrap().get_id(), 0);
    // }

    // #[test]
    // fn add_observation_id_cb() {
    //     let times = 15;
    //     let counter = Rc::new(Cell::new(0));
    //
    //     let mut sensor = Sensor::new(
    //         IpAddr::from(Ipv4Addr::from(3232235901)),
    //         IpAddr::from(Ipv4Addr::from(0xFFFFFF00)),
    //     );
    //
    //     let counter_ = Rc::clone(&counter);
    //     sensor.set_callback(Some(Box::new(move |_observation_id| {
    //         counter_.set(counter_.get() + 1);
    //     })));
    //
    //     for _ in 0..times {
    //         let observation_id_1 = ObservationID::new(0);
    //         let observation_id_2 = ObservationID::new(123);
    //         sensor.add_observation_id(observation_id_2);
    //         sensor.add_default_observation_id(observation_id_1);
    //     }
    //
    //     assert_eq!(counter.get(), times * 2);
    // }
}
