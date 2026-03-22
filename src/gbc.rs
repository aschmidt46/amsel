use std::{cell::RefCell, rc::Rc};

use crate::gbc::{bus::Bus};

pub mod sm83;
pub mod ppu;
pub mod bus;
pub mod cartridge;

pub struct CGB{
    pub bus: Rc<RefCell<Bus>>,
    pub has_frame: bool,
}

impl CGB{
    pub fn new(path: &str) -> Self{
        let bus: Rc<RefCell<Bus>> = Rc::new(RefCell::new(Bus::new_init(path)));
        bus.borrow_mut().create_components(bus.clone());
        CGB { bus, has_frame: false }
    }

    pub fn clock(&mut self){
        self.bus.borrow_mut().clock();
        if self.bus.borrow().has_frame{
            self.has_frame = true;
            self.bus.borrow_mut().has_frame = false;
        }
    }
}

unsafe impl Send for CGB {}
