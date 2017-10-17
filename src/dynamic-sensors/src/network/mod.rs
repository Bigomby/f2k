pub mod bindings;

use util::{get_netmask_prefix_ipv4, get_netmask_prefix_ipv6, v6_to_v4, apply_netmask};

use std::net::IpAddr;
use std::ffi::CString;

pub struct Network {
    network: IpAddr,
    netmask: IpAddr,
    name: CString,
    addres_as_str: CString,
}

impl Network {
    pub fn new(network: IpAddr, netmask: IpAddr, name: &str) -> Self {
        let (network, netmask) = if let IpAddr::V6(ipv6) = network {
            match ipv6.to_ipv4() {
                Some(ipv4) => (IpAddr::from(ipv4), v6_to_v4(&netmask)),
                None => (IpAddr::from(ipv6), netmask),
            }
        } else {
            (network, v6_to_v4(&netmask))
        };

        let network_str = match network {
            IpAddr::V4(ipv4) => format!("{}/{}", ipv4, get_netmask_prefix_ipv4(netmask)),
            IpAddr::V6(ipv6) => format!("{}/{}", ipv6, get_netmask_prefix_ipv6(netmask)),
        };

        Network {
            network: apply_netmask(&network, &netmask),
            netmask: netmask,
            name: CString::new(name).expect("Invalid network name"),
            addres_as_str: CString::new(network_str).expect("Invalid address"),
        }
    }

    pub fn get_ip(&self) -> &IpAddr {
        &self.network
    }

    pub fn get_netmask(&self) -> &IpAddr {
        &self.netmask
    }

    pub fn get_ip_str(&self) -> &CString {
        &self.addres_as_str
    }

    pub fn get_name(&self) -> &CString {
        &self.name
    }
}
