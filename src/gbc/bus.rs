use std::{cell::RefCell, rc::Rc};

use crate::gbc::{cartridge::RomObject, ppu::PPU, sm83::SM83};


pub struct Bus{
    memory: [u8; 0x10000],
    cpu: Option<SM83>,
    ppu: Option<PPU>,
    cart: Option<RomObject>,
}

impl Bus{
    pub fn new() -> Self{
        Bus { memory: [0; 0x10000], cart: None, cpu: None, ppu: None}
    }
    pub fn new_init(path: &str) -> Self{
        let mut bus = Bus::new();
        bus.cart = RomObject::new(path).ok();
        bus
    }
    pub fn create_components(&mut self, this: Rc<RefCell<Bus>>){
        let mut cpu = SM83::new_init(this.clone());
        cpu.set_initial_state_dmg();
        let ppu = PPU::new(this.clone());
        self.cpu = Some(cpu);
        self.ppu = Some(ppu);
    }
    pub fn read_memory(&mut self, addr: u16) -> u8{
        *self.get_path(addr)
    }
    pub fn write_memory(&mut self, addr: u16, val: u8){
        *self.get_path(addr) = val;
    }
    pub fn read_memory_16(&mut self, addr: u16) -> u16{
        let low = self.read_memory(addr);
        let high = self.read_memory(addr.wrapping_add(1));
        (low as u16) | ((high as u16) << 8)
    }

    fn get_path(&mut self, addr: u16) -> &mut u8{
        let cart = self.cart.as_mut().unwrap();
        let ppu = self.ppu.as_mut().unwrap();
        let cpu = self.cpu.as_mut().unwrap();
        match addr{
            0x0000..0x4000 => &mut cart.raw_data[addr as usize],
            0x4000..0x8000 => &mut cart.raw_data[addr as usize],
            0x8000..0xA000 => &mut ppu.vram[ppu.bank_select][addr as usize],
            
            _ => panic!("Ungültige Speicheradresse!: {}", addr),
        }
    }
    pub fn clock(&mut self){
        self.ppu.as_mut().unwrap().clock();
        self.cpu.as_mut().unwrap().clock();
    }
}

#[cfg(test)]
mod test{
    #[test]
    fn read_write_test(){
        let mut b = super::Bus::new();
        assert_eq!(b.read_memory(0xFF30), 0);
        b.write_memory(0xFF30, 0xAA);
        assert_eq!(b.read_memory(0xFF30), 0xAA);
    }
}