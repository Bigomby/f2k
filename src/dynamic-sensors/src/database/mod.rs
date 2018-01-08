pub mod bindings;

use sensor::Sensor;
use util::apply_netmask;

use std::collections::HashMap;
use std::net::IpAddr;

type SensorCallback = Option<Box<Fn(&Sensor)>>;
type SensorMap = HashMap<IpAddr, Sensor>;

pub struct SensorsDB {
    database: SensorMap,
    cb: SensorCallback,
}

impl SensorsDB {
    pub fn new() -> Self {
        SensorsDB {
            database: SensorMap::new(),
            cb: None,
        }
    }

    pub fn set_callback(&mut self, cb: SensorCallback) {
        self.cb = cb;
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

        if let Some(ref mut cb) = self.cb {
            cb(&sensor);
        }

        self.database.insert(network, sensor);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::Ipv4Addr;
    use std::rc::Rc;
    use std::cell::Cell;

    #[test]
    fn test_add_sensors() {
        let ip_1 = IpAddr::from(Ipv4Addr::from(3232235901)); // Added to DB
        let ip_2 = IpAddr::from(Ipv4Addr::from(3232236132)); // Added to DB
        let ip_3 = IpAddr::from(Ipv4Addr::from(3232236133)); // Not added to DB
        let ip_4 = IpAddr::from(Ipv4Addr::from(3232236389)); // Not added to DB

        let nm = IpAddr::from(Ipv4Addr::from(0xFFFFFF00));

        let sensor_1 = Sensor::new(ip_1, nm);
        let sensor_2 = Sensor::new(ip_2, nm);

        let mut database = SensorsDB::new();

        database.add_sensor(sensor_1);
        database.add_sensor(sensor_2);

        // Sensors with ip_1 and ip_2 should exists on DB
        assert!(database.get_sensor(ip_1).is_some());
        assert!(database.get_sensor(ip_2).is_some());

        // Sensors with ip_3 is in he same network as ip_2
        assert!(database.get_sensor(ip_3).is_some());

        // Sensor with ip_4 is not known
        assert!(database.get_sensor(ip_4).is_none());
    }

    #[test]
    fn test_sensor_callback() {
        let times = 15;
        let counter = Rc::new(Cell::new(0));

        let counter_ = Rc::clone(&counter);
        let mut database = SensorsDB::new();

        database.set_callback(Some(Box::new(move |ref _sensor| {
            counter_.set(counter_.get() + 1);
        })));

        for _ in 0..times {
            database.add_sensor(Sensor::new(IpAddr::from(Ipv4Addr::from(3232235901)),
                                            IpAddr::from(Ipv4Addr::from(0xFFFFFF00))));
        }

        assert_eq!(counter.get(), times);
    }
}
