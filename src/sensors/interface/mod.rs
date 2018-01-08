pub mod bindings;

pub struct Interface {
    id: u64,
    name: Vec<u8>,
    description: Vec<u8>,
}

impl Interface {
    pub fn new(id: u64, name: Vec<u8>, description: Vec<u8>) -> Self {
        Interface {
            id: id,
            name: Vec::from(name),
            description: Vec::from(description),
        }
    }

    pub fn get_id(&self) -> u64 {
        self.id
    }

    pub fn get_name(&self) -> &[u8] {
        &self.name
    }

    pub fn get_description(&self) -> &[u8] {
        &self.description
    }
}
