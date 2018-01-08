use sensors::observation_id::ObservationID;
use sensors::network::Network;
use sensors::sensor::Sensor;

use serde_json::{to_string, Value};

use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
use std::str::FromStr;

pub fn parse_sensors(j_sensor_networks: &Value) -> Vec<Sensor> {
    let j_sensors = j_sensor_networks
        .as_object()
        .expect("\"sensors_networks\" should be an object");

    j_sensors
        .iter()
        .map(|(address, sensor)| parse_sensor(address, sensor))
        .collect()
}

pub fn parse_observation_id(id_str: &str, j_observation_id: &Value) -> ObservationID {
    let id = match id_str {
        "default" => 0,
        _ => id_str
            .parse()
            .expect("Observation ID key should be a number (or \"default\")"),
    };
    let j_observation_id = j_observation_id
        .as_object()
        .expect("Every observation_id should be an object");

    let mut observation_id = ObservationID::new(id);

    for (key, value) in j_observation_id {
        match key.as_ref() {
            "home_nets" => {
                let home_nets = value.as_array().expect("Home nets should be an array");
                for j_home_net in home_nets {
                    let (network, netmask, name) = parse_home_net(j_home_net);
                    observation_id.add_network(Network::new(network, netmask, &name));
                }
            }
            "enrichment" => {
                observation_id.set_enrichment(parse_enrichment(value));
            }
            "fallback_first_switch" => {
                observation_id.set_fallback_first_switch(parse_fallback_first_switch(value));
            }
            "span_port" => if parse_span_port(value) {
                observation_id.set_span_mode();
            },
            "exporter_in_wan_side" => if parse_exporter_in_wan_side(value) {
                observation_id.set_exporter_in_wan_side();
            },
            unknown_field => println!("Ignoring unknown field: \"{}\"", unknown_field),
        }
    }

    observation_id
}

fn parse_sensor(address: &str, j_sensor: &Value) -> Sensor {
    let j_observation_ids = j_sensor
        .as_object()
        .expect("Every sensor should be an object")
        .get("observations_id")
        .expect("Sensors should contain \"observations_id\" object")
        .as_object()
        .expect("\"observations_id\" should be an object");

    let (ip_address, netmask) = parse_address(address);
    let mut sensor = Sensor::new(ip_address, netmask);

    for (j_observation_id_key, j_observation_id_value) in j_observation_ids {
        let observation_id = parse_observation_id(j_observation_id_key, j_observation_id_value);

        match observation_id.get_id() {
            0 => sensor.add_default_observation_id(observation_id),
            _ => sensor.add_observation_id(observation_id),
        };
    }

    sensor
}

fn parse_exporter_in_wan_side(j_exporter_in_wan_side: &Value) -> bool {
    j_exporter_in_wan_side
        .as_bool()
        .expect("\"exporter_in_wan_side\" should be boolean")
}

fn parse_span_port(j_span_port: &Value) -> bool {
    j_span_port
        .as_bool()
        .expect("\"fallback_first_switch\" should be boolean")
}

fn parse_fallback_first_switch(j_fallback_first_switch: &Value) -> i64 {
    j_fallback_first_switch
        .as_i64()
        .expect("\"fallback_first_switch\" should be an integer")
}

fn parse_enrichment(j_enrichment: &Value) -> String {
    let enrichment = j_enrichment
        .as_object()
        .expect("Enrichment should be an object");
    let enrichment = to_string(&enrichment).expect("Error processing enrichment");

    enrichment[1..enrichment.len() - 1].to_owned()
}

fn parse_home_net(j_home_net: &Value) -> (IpAddr, IpAddr, String) {
    let network_str = j_home_net
        .get("network")
        .expect("Every \"home_net\" should contain a network")
        .as_str()
        .expect("\"home_net\" networks should be string");
    let network_name = j_home_net
        .get("network_name")
        .expect("Every \"home_net\" should contain a network name")
        .as_str()
        .expect("\"home_net\" network names should be strings");

    let (network, netmask) = parse_address(network_str);
    (network, netmask, network_name.to_owned())
}

fn parse_address(address_str: &str) -> (IpAddr, IpAddr) {
    let mut ip_netmask_str = address_str.split("/");

    let ip_str = ip_netmask_str.next().expect("No IP address found");
    let netmask_str = ip_netmask_str.next();

    match Ipv4Addr::from_str(ip_str) {
        Ok(ipv4_address) => {
            let parsed_ip = IpAddr::from(ipv4_address);
            let parsed_netmask = match netmask_str {
                Some(netmask) => parse_netmask_v4(netmask),
                None => parse_netmask_v4("32"),
            };

            (parsed_ip, parsed_netmask)
        }

        Err(_) => match Ipv6Addr::from_str(ip_str) {
            Ok(ipv6_address) => {
                let parsed_ip = IpAddr::from(ipv6_address);
                let parsed_netmask = match netmask_str {
                    Some(netmask) => parse_netmask_v6(netmask),
                    None => parse_netmask_v6("128"),
                };

                (parsed_ip, parsed_netmask)
            }
            Err(err) => panic!(err),
        },
    }
}

fn parse_netmask_v4(netmask_str: &str) -> IpAddr {
    let netmask_bits: u8 = netmask_str.parse().expect("Can't parse netmask");
    assert!(netmask_bits <= 32);

    let mut netmask: u32 = 0;

    for _ in 0..netmask_bits {
        netmask = (netmask << 1) | 1;
    }
    netmask <<= 32 - netmask_bits;

    IpAddr::from(Ipv4Addr::from(netmask))
}

fn parse_netmask_v6(netmask_str: &str) -> IpAddr {
    let netmask_bits: u8 = netmask_str.parse().expect("Can't parse netmask");
    assert!(netmask_bits <= 128);

    let mut netmask: u128 = 0;

    for _ in 0..netmask_bits {
        netmask = (netmask << 1) | 1;
    }
    netmask <<= 128 - netmask_bits;

    IpAddr::from(Ipv6Addr::from(netmask))
}
