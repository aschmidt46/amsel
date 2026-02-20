use std::{cell::RefCell, rc::Rc};

use crate::gbc::{bus::Bus};

pub mod sm83;
pub mod ppu;
pub mod bus;
pub mod cartridge;

struct CGB{
    bus: Rc<RefCell<Bus>>,
}

impl CGB{
    pub fn new(path: &str) -> Self{
        let bus: Rc<RefCell<Bus>> = Rc::new(RefCell::new(Bus::new_init(path)));
        CGB { bus }
    }

    pub fn clock(&mut self){
        self.bus.borrow_mut().clock();
    }
}
