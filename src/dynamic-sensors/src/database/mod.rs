pub mod bindings;

use sensor::Sensor;
use util::apply_netmask;

use std::collections::HashMap;
use std::net::IpAddr;

#[derive(Default)]
pub struct SensorsDB {
    database: HashMap<IpAddr, Sensor>,
}

impl SensorsDB {
    pub fn new() -> Self {
        SensorsDB::default()
    }

    pub fn get_sensor(&self, ip: IpAddr) -> Option<&Sensor> {
        let ip = if let IpAddr::V6(ipv6) = ip {
            match ipv6.to_ipv4() {
                Some(ipv4) => IpAddr::from(ipv4),
                None => IpAddr::from(ipv6),
            }
        } else {
            ip
        };

        for sensor in self.database.values() {
            let network = apply_netmask(&ip, &sensor.get_netmask());
            if let Some(sensor) = self.database.get(&network) {
                return Some(sensor);
            }
        }

        None
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
    use std::net::Ipv4Addr;

    #[test]
    fn test_add_sensors() {
        let mut database = SensorsDB::new();

        let sensor_1 = Sensor::new(IpAddr::from(Ipv4Addr::from(3232235901)),
                                   IpAddr::from(Ipv4Addr::from(0xFFFFFF00)));
        let sensor_2 = Sensor::new(IpAddr::from(Ipv4Addr::from(3232236232)),
                                   IpAddr::from(Ipv4Addr::from(0xFFFFFF00)));

        database.add_sensor(sensor_1);
        database.add_sensor(sensor_2);

        let sensor_in_db_1 = database.get_sensor(IpAddr::from(Ipv4Addr::from(3232235901)));
        let sensor_in_db_2 = database.get_sensor(IpAddr::from(Ipv4Addr::from(3232236132)));
        let sensor_in_db_3 = database.get_sensor(IpAddr::from(Ipv4Addr::from(3232236133)));
        let sensor_in_db_4 = database.get_sensor(IpAddr::from(Ipv4Addr::from(3232236389)));

        assert!(sensor_in_db_1.is_some());
        assert!(sensor_in_db_2.is_some());
        assert_eq!(sensor_in_db_2.unwrap().get_network(),
                   sensor_in_db_3.unwrap().get_network());
        assert!(sensor_in_db_4.is_none());
    }
}
