# Kafka Netflow

* [Overview](#overview)
* [Differences with f2k](#differences-with-f2k)
* [Status](#status)
* [Installing](#installing)
  * [Requirements](#requirements)
  * [Compiling](#compiling)
* [Usage](#usage)
* [Basic usage](#basic-usage)
* [Sensors database file](#sensors-database-file)
* [Others configuration parameters](#others-configuration-parameters)
  * [Template cache](#template-cache)
  * [Multi-thread](#multi-thread)
  * [librdkafka options](#librdkafka-options)
  * [Long flow separation](#long-flow-separation)
  * [Geo information](#geo-information)
  * [Names resolution](#names-resolution)
    * [Mac vendor information (mac_vendor)](#mac-vendor-information-mac_vendor)
    * [Applications/engine ID (applications, <code>engines</code>)](#applicationsengine-id-applications-engines)
    * [Hosts, domains, vlan (hosts, <code>http_domains</code>,<code>vlans</code>)](#hosts-domains-vlan-hosts-http_domains-vlans)
    * [Netflow probe nets](#netflow-probe-nets)
    * [DNS](#dns)

## Overview

Simple service to dump collected Netflow V5/V9/IPFIX traffic to a Kafka topic.

`kafka-netflow` is based on [redBorder/f2k](https://github.com/redBorder/f2k),
but aims to provide a more flexible way to manage sensors.

## Differences with f2k

While `f2k` uses static JSON file to store sensors configuration,
`kafka-netflow` will use an external service to dynamically store this
information. This approach makes possible to add, remove, or update sensors
using a REST API instead of modify the configuration file and restarting the
service.

Zookeeper support is removed since sensors information (along with the
Netflow templates) will be stored on the external service.

## Status

Currently, `kafka-netflow` uses a new sensor database module which has been
rewritted in Rust. This new module allows to use an external service to store
sensors information, but it has not been implemented yet. **Sensor database
are still a static JSON file, but it will change in the future**.

This project is currently WIP. The first release will be compatible with `f2k`
but without support for Zookeeper.

## Installing

### Requirements

- Rust nightly (expected to work with stable Rust in the future)
- gcc
- Makefile
- udns
- jansson
- librdkafka
- GeoIP
- libpcap

### Compiling

```bash
./configure
make
make install
```

## Usage

## Basic usage

The most important configuration parameters are:

- Input/output parameters:
  - `--kafka=127.0.0.1@netflow`, broker@topic to send netflow
  - `--collector-port=2055`, Collector port to listen netflow

- Configuration
  - `--rb-config=/etc/sensors_db.json`, file with sensors config. File format is
detailed on the next section.

## Sensors database file

You need to specify each sensor you want to read netflow from in a JSON file:

```json
{
  "sensors_networks": {
    "4.3.2.1": {
      "observations_id": {
        "1": {
          "enrichment": {
            "sensor_ip": "4.3.2.1",
            "sensor_name": "flow_test",
            "observation_id": 1
          }
        }
      }
    }
  }
}
```

With this file, you will be listening for netflow coming from `4.3.2.1` (this
could be a network too, `4.3.2.0/24`), and the JSON output will be sent with
that `sensor_ip`, `sensor_name` and `observation_id` keys.

## Others configuration parameters

### Template cache

You can specify a folder to save/load templates using
`--template-cache=/var/kafka-netflow/templates`.

### Multi-thread

`--num-threads=N` can be used to specify the number of netflow processing
threads.


### librdkafka options

All [librdkafka options](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md).
can be used using `-X` parameter. The argument will be passed directly to
librdkafka config, so you can use whatever config you need.

Recommended options are:

- `-X=socket.max.fails=3`,
- `-X=delivery.report.only.error=true`,

### Long flow separation

Use `--separate-long-flows` if you want to divide flow with duration>60s into
minutes. For example, if the flow duration is 1m30s, `kafka-netflow` will send
1 message containing 2/3 of bytes and pkts for the minute, and 1/3 of bytes and
pkts to the last 30 seconds, like if we had received 2 different flows.

(see [Test 0017](tests/0017-separateLongTimeFlows.c) for more information about
how flow are divided)

### Geo information

`kafka-netflow` can add geographic information if you specify
[Maxmind GeoLite Databases](https://dev.maxmind.com/geoip/legacy/geolite/)
location using:

  - `--as-list=/var/GeoIP/asn.dat`,
  - `--country-list=/var/GeoIP/country.dat`,

### Names resolution

You can include more flow information, like many object names, with the option
`--hosts-path=/opt/rb/etc/objects/`. This folder needs to have files with the
provided names in order to `kafka-netflow` read them.

#### Mac vendor information (`mac_vendor`)

With `--mac-vendor-list=mac_vendors` `kafka-netflow` can translate flow source
and destination macs, and they will be sending in JSON output as
`in_src_mac_name`, `out_src_mac_name`, and so on.

The file `mac_vendors` should be like:

```
FCF152|Sony Corporation
FCF1CD|OPTEX-FA CO.,LTD.
FCF528|ZyXEL Communications Corporation
```

And you can generate it using `make manuf`, that will obtain it automatically
from [IANA Registration Authority](http://standards.ieee.org/develop/regauth/).

#### Applications/engine ID (`applications`, `engines`)

`kafka-netflow` can translate applications and engine ID if you specify a list
with them, like:

- &lt;hosts-path&gt;/engines
    ```
    None            0
    IANA-L3         1
    PANA-L3         2
    IANA-L4         3
    PANA-L4         4
    ...
    ```

- &lt;hosts-path&gt;/applications
    ```
    3com-amp3                 50332277
    3com-tsmux                50331754
    3pc                       16777250
    914c/g                    50331859
    ...
    ```

#### Hosts, domains, vlan (`hosts`, `http_domains`, `vlans`)

You can include more information about the flow source and destination (
`src_name` and `dst_name`) using a hosts list, using the same format as
`/etc/hosts`. The same can be used with files `vlan`, `domains`, `macs`.

#### Netflow probe nets

You can specify per netflow probe home nets, so they will be taking into account
when solving client/target IP.

You could specify them using `home_nets`:

```json
"sensors_networks": {
  "4.3.2.0/24": {
    "2055": {
      "sensor_name": "test1",
      "sensor_ip": "",
      "home_nets": [{
          "network": "10.13.30.0/16",
          "network_name": "users"
        },
        {
          "network": "2001:0428:ce00:0000:0000:0000:0000:0000/48",
          "network_name": "users6"
        }
      ],
    }
  }
}
```

#### DNS

`f2k` can make reverse DNS in order to obtain some hosts names. To enable them,
you must use:

- `enable-ptr-dns`, general enable
- `dns-cache-size-mb`, DNS cache to not repeat PTR queries
- `dns-cache-timeout-s`, Entry cache timeout
