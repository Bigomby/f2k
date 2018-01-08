pub mod bindings;

use sensors::sensor::Sensor;
use util::apply_netmask;

use std::collections::HashMap;
use std::net::IpAddr;

type SensorMap = HashMap<IpAddr, Sensor>;

pub struct SensorsDB {
    database: SensorMap,
}

impl SensorsDB {
    pub fn new() -> Self {
        SensorsDB {
            database: SensorMap::new(),
        }
    }

    pub fn get_sensor(&mut self, sensor_ip: IpAddr) -> Option<&mut Sensor> {
        println!("{:?}", sensor_ip);

        let networks: Vec<IpAddr> = self.database
            .values()
            .into_iter()
            .filter(|sensor| {
                sensor.get_network().is_ipv4() && sensor_ip.is_ipv4()
                    || sensor.get_network().is_ipv6() && sensor_ip.is_ipv6()
            })
            .map(|sensor| sensor.get_netmask())
            .map(|netmask| apply_netmask(&sensor_ip, &netmask))
            .collect();

        let network = networks.first().unwrap();
        self.database.get_mut(network)
    }

    pub fn list_sensors(&self) -> Vec<&Sensor> {
        let mut sensors = Vec::new();
        for sensor in self.database.values() {
            sensors.push(sensor);
        }

        sensors
    }

    pub fn add_sensor(&mut self, sensor: Sensor) {
        let network = apply_netmask(&sensor.get_network(), &sensor.get_netmask());
        self.database.insert(network, sensor);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::{Ipv4Addr, Ipv6Addr};
    use std::str::FromStr;

    #[test]
    fn test_add_sensors() {
        let ip_1 = Ipv4Addr::from_str("192.168.1.135").expect("Error parsing IP");
        let ip_2 = Ipv6Addr::from_str("2001:db8:a0b:12f0::1").expect("Error parsing IP");
        let ip_3 = Ipv4Addr::from_str("192.168.1.200").expect("Error parsing IP");
        let ip_4 = Ipv6Addr::from_str("2001:db8:a0b:12f0::15").expect("Error parsing IP");
        let ip_5 = Ipv4Addr::from_str("192.168.15.1").expect("Error parsing IP");
        let ip_6 = Ipv6Addr::from_str("2001:ff8:a0b:12f0::15").expect("Error parsing IP");

        let netmask_1 = Ipv4Addr::from_str("255.255.255.0").expect("Error parsing netmask");
        let netmask_2 = Ipv6Addr::from_str("FFFF:FFFF::").expect("Error parsing netmask");

        let sensor_1 = Sensor::new(IpAddr::from(ip_1), IpAddr::from(netmask_1));
        let sensor_2 = Sensor::new(IpAddr::from(ip_2), IpAddr::from(netmask_2));

        let mut database = SensorsDB::new();

        database.add_sensor(sensor_1);
        database.add_sensor(sensor_2);

        // ip_1 and ip_3 belongs to the same network and ip_v5 does not
        assert!(database.get_sensor(IpAddr::from(ip_1)).is_some());
        assert!(database.get_sensor(IpAddr::from(ip_3)).is_some());
        assert!(database.get_sensor(IpAddr::from(ip_5)).is_none());

        // ip_2 and ip_4 belongs to the same network and ip_v6 does not
        assert!(database.get_sensor(IpAddr::from(ip_2)).is_some());
        assert!(database.get_sensor(IpAddr::from(ip_4)).is_some());
        assert!(database.get_sensor(IpAddr::from(ip_6)).is_none());
    }
}
