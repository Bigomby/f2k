pub mod bindings;

pub struct Application {
    id: u64,
    name: Vec<u8>,
}

impl Application {
    pub fn new(id: u64, name: Vec<u8>) -> Self {
        Application {
            id: id,
            name: Vec::from(name),
        }
    }

    pub fn get_id(&self) -> u64 {
        self.id
    }

    pub fn get_name(&self) -> &[u8] {
        &self.name
    }
}
