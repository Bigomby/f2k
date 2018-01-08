pub mod bindings;

mod parser;
use sensors::sensor::Sensor;

use serde_json;
use libc::c_void;

use std::fs::File;
use std::io::Read;
use std::ptr;

pub struct FileLoader {
    new_sensor_event: Option<Box<FnMut(Sensor, *mut c_void)>>,
    ctx: *mut c_void,
}

impl FileLoader {
    pub fn new() -> FileLoader {
        FileLoader {
            new_sensor_event: None,
            ctx: ptr::null_mut(),
        }
    }

    pub fn load(&mut self, filename: &str) {
        let mut f = File::open(filename).expect("File not found!");
        let mut contents = String::new();

        f.read_to_string(&mut contents)
            .expect("Something went wrong reading the file!");
        let database: serde_json::Value =
            serde_json::from_str(&contents).expect("Error parsing JSON!");

        let sensors_networks = database
            .get("sensors_networks")
            .expect("No \"sensors_networks\" object found!");

        let sensors = parser::parse_sensors(sensors_networks);
        for sensor in sensors {
            if let Some(ref mut event) = self.new_sensor_event {
                event(sensor, self.ctx);
            }
        }
    }

    pub fn set_new_sensor_event(&mut self, cb: Box<FnMut(Sensor, *mut c_void)>, ctx: *mut c_void) {
        self.ctx = ctx;
        self.new_sensor_event = Some(cb);
    }
}
