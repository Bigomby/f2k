use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
use libc::c_void;

#[no_mangle]
pub extern "C" fn dsensors_free(list_ptr: *mut c_void) {
    if list_ptr.is_null() {
        return;
    };

    unsafe { Box::from_raw(list_ptr) };
}

fn ipv4_to_u32(ip: &[u8; 4]) -> u32 {
    let mut ip_int = 0;

    for (i, octet) in ip.iter().enumerate() {
        ip_int += (*octet as u32) << (24 - 8 * i);
    }

    ip_int
}

fn ipv6_to_u128(ip: &[u8; 16]) -> u128 {
    let mut ip_int = 0;

    for (i, octet) in ip.iter().enumerate() {
        ip_int += (*octet as u128) << (120 - 8 * i);
    }

    ip_int
}

pub fn v6_to_v4(netmask: &IpAddr) -> IpAddr {
    match netmask {
        &IpAddr::V4(netmask) => IpAddr::from(netmask),
        &IpAddr::V6(netmask) => {
            let octets = &netmask.octets();
            IpAddr::from(Ipv4Addr::new(
                octets[12],
                octets[13],
                octets[14],
                octets[15],
            ))
        }
    }
}

pub fn normalize_address(address: IpAddr) -> IpAddr {
    match address {
        IpAddr::V4(_) => address,
        IpAddr::V6(ipv6) => match ipv6.to_ipv4() {
            Some(ipv4) => IpAddr::from(ipv4),
            None => address,
        },
    }
}

pub fn apply_netmask(ip: &IpAddr, netmask: &IpAddr) -> IpAddr {
    match (ip, netmask) {
        (&IpAddr::V4(ip), &IpAddr::V4(netmask)) => {
            let ref ip_octets = ip.octets();
            let ref netmask_octets = netmask.octets();
            IpAddr::from(Ipv4Addr::from(
                ipv4_to_u32(ip_octets) & ipv4_to_u32(netmask_octets),
            ))
        }
        (&IpAddr::V6(ip), &IpAddr::V6(netmask)) => {
            let ref ip_octets = ip.octets();
            let ref netmask_octets = netmask.octets();
            IpAddr::from(Ipv6Addr::from(
                ipv6_to_u128(ip_octets) & ipv6_to_u128(netmask_octets),
            ))
        }
        _ => panic!("IP address and netmask version does not match"),
    }
}

pub fn get_netmask_prefix_ipv4(netmask: IpAddr) -> u32 {
    match netmask {
        IpAddr::V4(netmask) => {
            let int_netmask = ipv4_to_u32(&netmask.octets()) as u128;
            int_netmask.count_ones()
        }
        IpAddr::V6(netmask) => match netmask.to_ipv4() {
            Some(netmask) => {
                let int_netmask = ipv4_to_u32(&netmask.octets()) as u128;
                int_netmask.count_ones()
            }
            None => {
                let int_netmask = ipv6_to_u128(&netmask.octets());
                32 - int_netmask.count_zeros()
            }
        },
    }
}

pub fn get_netmask_prefix_ipv6(netmask: IpAddr) -> u32 {
    match netmask {
        IpAddr::V4(_) => {
            panic!("Can't get ipv6 netmask for a ipv4 adddress");
        }
        IpAddr::V6(netmask) => {
            let int_netmask = ipv6_to_u128(&netmask.octets());
            int_netmask.count_ones()
        }
    }
}
