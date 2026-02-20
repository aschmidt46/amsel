use std::{cell::RefCell, rc::Rc};

use crate::gbc::bus::Bus;


pub struct PPU{
    pub (crate) vram: [[u8; 0x2000]; 2], // Beide Bänke zu je 8KiB
    pub (crate) bank_select: usize,
    bus: Rc<RefCell<Bus>>,
}

impl PPU{
    pub fn new(bus: Rc<RefCell<Bus>>) -> Self{
        PPU { vram: [[0; 0x2000]; 2], bus, bank_select: 0 }
    }
    pub fn clock(&mut self){

    }
}
