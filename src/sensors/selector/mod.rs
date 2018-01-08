pub mod bindings;

pub struct Selector {
    id: u64,
    name: Vec<u8>,
}

impl Selector {
    pub fn new(id: u64, name: Vec<u8>) -> Self {
        Selector {
            id: id,
            name: name,
        }
    }

    pub fn get_id(&self) -> u64 {
        self.id
    }

    pub fn get_name(&self) -> &[u8] {
        &self.name
    }
}
