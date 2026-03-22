use std::{cell::RefCell, rc::Rc};

use crate::gbc::{cartridge::RomObject, ppu::PPU, sm83::{Opcode, OperandType, SM83}};


pub struct Bus{
    work_ram: [u8; 0x8000], // 32KiB, 8KiB auf DMG
    high_ram: [u8; 0x80],
    external_ram_bank: usize,
    external_ram: [[u8; 0x2000]; 16], //max 16 Bänke
    cpu: Option<SM83>,
    ppu: Option<PPU>,
    cart: Option<RomObject>,
    div: u8, //FF04
    tima: u8, //FF05
    tma: u8, //FF06
    tac: u8, //FF07
    oam_dma: u8, // FF46
    div_counter: usize,
    tima_counter: usize,
    joypad: [bool; 4],
    buttons: [bool; 4],
    read_joypad: bool,
    read_buttons: bool,
    pub has_frame: bool,
    dma_next_clock: i32,
}

const TAC_TABLE: [u16; 4] = [256 * 4, 4 * 4, 16 * 4, 64 * 4];

impl Bus{
    pub fn new() -> Self{
        Bus { work_ram: [0; 0x8000], high_ram: [0; 0x80], cart: None, cpu: None, ppu: None, has_frame: false, external_ram_bank: 0, external_ram: [[0; 0x2000]; 16],
            div: 0, tima: 0,  tma: 0, tac: 0, div_counter: 0, tima_counter: 0, joypad: [true; 4], buttons: [true; 4], read_buttons: false, read_joypad: false, oam_dma: 0, dma_next_clock: -1}
    }
    pub fn new_init(path: &str) -> Self{
        let mut bus = Bus::new();
        bus.cart = RomObject::new(path).ok();
        bus
    }
    pub fn create_components(&mut self, this: Rc<RefCell<Bus>>){
        let mut cpu = SM83::new_init(Rc::downgrade(&this));
        cpu.set_initial_state_dmg();
        let ppu = PPU::new(Rc::downgrade(&this));
        self.cpu = Some(cpu);
        self.ppu = Some(ppu);
    }
    pub fn read_memory(&mut self, addr: u16) -> u8{
        match self.get_path(addr){
            Some(place) => *place,
            None => self.read_side_effects(addr),
        }
    }
    pub fn write_memory(&mut self, addr: u16, val: u8){
        match self.get_path(addr){
            Some(place) => *place = val,
            None => self.write_side_effects(addr, val),
        };
    }
    pub fn read_memory_16(&mut self, addr: u16) -> u16{
        let low = self.read_memory(addr);
        let high = self.read_memory(addr.wrapping_add(1));
        (low as u16) | ((high as u16) << 8)
    }

    fn read_side_effects(&mut self, addr: u16) -> u8{
        let cart = self.cart.as_mut().unwrap();
        let ppu = self.ppu.as_mut().unwrap();
        let cpu = self.cpu.as_mut().unwrap();
        match addr{
            0x0000..0x4000 => cart.raw_data[addr as usize],
            0x4000..0x8000 => cart.raw_data[addr as usize],
            0xFF00 => {
                let mut res = 0xFF;
                if !self.read_joypad && self.read_buttons{
                    for i in 0..4{
                        res &= !((!self.joypad[i] as u8) << i);
                    }
                }
                else if self.read_joypad && !self.read_buttons{
                    for i in 0..4{
                        res &= !((!self.buttons[i] as u8) << i);
                    }
                }
                res &= !(1 << 4);
                res &= !(1 << 5);
                res |= (self.read_joypad as u8) << 4;
                res |= (self.read_buttons as u8) << 5;
                res
            },
            0xFF04 => self.div,
            0xFF05 => self.tima,
            0xFF06 => self.tma,
            0xFF07 => self.tac,
            0xFF0F => cpu.if_reg,
            0xFF40 => ppu.lcdc,
            0xFF41 => ppu.stat,
            0xFF42 => ppu.scy,
            0xFF43 => ppu.scx,
            0xFF46 => self.oam_dma,
            0xFF4A => ppu.wy,
            0xFF4B => ppu.wx,
            0xFF44 => ppu.scanline,
            0xFF45 => ppu.lyc,
            0xFFFF => cpu.ie_reg,
            _ => 0xFF, // IO Register geben das zurück, wenn keine Tasten gedrückt
        }
    }
    fn write_side_effects(&mut self, addr: u16, val: u8){
        let cart = self.cart.as_mut().unwrap();
        let ppu = self.ppu.as_mut().unwrap();
        let cpu = self.cpu.as_mut().unwrap();
        match addr{
            0xFF00 => {
                self.read_buttons = (val & 0b00100000) > 0;
                self.read_joypad = (val &  0b00010000) > 0;
            },
            0xFF04 => self.div = 0,
            0xFF05 => self.tima = val,
            0xFF06 => self.tma = val,
            0xFF07 => self.tac = val,
            0xFF0F => cpu.if_reg = val,
            0xFF40 => ppu.lcdc = val,
            0xFF41 => ppu.write_stat(val),
            0xFF42 => ppu.scy = val,
            0xFF43 => ppu.scx = val,
            0xFF46 => {
                self.oam_dma = val;
                cpu.remaining_cycles += 640;
                let source = (val as u16) * 0x100;
                self.dma_next_clock = source as i32;
            }
            0xFF4A => ppu.wy = val,
            0xFF4B => ppu.wx = val,
            0xFF44 => (),
            0xFF45 => ppu.lyc = val,
            0xFFFF => cpu.ie_reg = val,
            _ => (),//panic!("Konnte nicht schreiben!"),
        }
    }

    fn get_path(&mut self, addr: u16) -> Option<&mut u8>{
        let cart = self.cart.as_mut().unwrap();
        let ppu = self.ppu.as_mut().unwrap();
        let cpu = self.cpu.as_mut().unwrap();
        match addr{
            // Das hier ist ein Problem, weil Spiele in den ROM schreiben für Bank switching und in der Implementierung hier die ROM überschreiben
            0x0000..0x4000 => None, // Deswegen Delegat an Side Effects Funktion
            0x4000..0x8000 => None,
            0x8000..0xA000 => Some(&mut ppu.vram[ppu.bank_select][addr as usize - 0x8000]),
            0xA000..0xC000 => { // Ram auf Cartridge und tauschbare Bank, falls vorhanden
                Some(&mut self.external_ram[self.external_ram_bank][addr as usize - 0xA000])
            },
            0xC000..0xD000 => Some(&mut self.work_ram[addr as usize - 0xC000]),
            0xD000..0xE000 => Some(&mut self.work_ram[addr as usize - 0xC000]), // In CGB tauschbar
            0xE000..0xFE00 => None,//panic!("Illegaler Speicherbereich!"),
            0xFE00..0xFEA0 => Some(&mut ppu.oam[addr as usize - 0xFE00]),
            0xFEA0..0xFF00 => None,//panic!("Illegaler Speicherbereich!"),
            0xFF00..0xFF80 => None, //IO Register
            0xFF80..0xFFFF => Some(&mut self.high_ram[addr as usize - 0xFF80]),
            0xFFFF => Some(&mut cpu.ie_reg),
            
            // _ => panic!("Ungültige Speicheradresse!: {}", addr),
        }
    }
    pub fn request_vblank(&mut self){
        self.cpu.as_mut().unwrap().if_reg |= 1 << 0;
    }
    pub fn request_stat(&mut self){
        self.cpu.as_mut().unwrap().if_reg |= 1 << 1;
    }
    pub fn request_timer(&mut self){
        self.cpu.as_mut().unwrap().if_reg |= 1 << 2;
    }
    pub fn request_joypad(&mut self){
        self.cpu.as_mut().unwrap().if_reg |= 1 << 4;
    }
    pub fn clock(&mut self){
        if self.div_counter % 256 == 0{
            self.div = self.div.wrapping_add(1);
        }
        self.div_counter = self.div_counter.wrapping_add(1);

        if self.tima_counter % TAC_TABLE[(self.tac & 0b11) as usize] as usize == 0{
            if self.tac % 0b100 > 0{
                match self.tima.checked_add(1){
                    None => self.tima = self.tma,
                    Some(val) => { self.tima = val; self.request_timer(); },
                }
            }
        }
        self.tima_counter = self.tima_counter.wrapping_add(1);

        self.ppu.as_mut().unwrap().clock();
        self.cpu.as_mut().unwrap().clock();

        if self.dma_next_clock >= 0{
            let mut oams = self.ppu.as_mut().unwrap().oam.clone();
            for i in 0..160{
                let v = self.read_memory(i + (self.dma_next_clock as u16));
                oams[i as usize] = v;
            }
            self.ppu.as_mut().unwrap().oam = oams;

            self.dma_next_clock = -1;
        }
    }
    pub fn press_button(&mut self, b: usize){
        self.buttons[b] = false;
        if self.read_buttons { self.request_joypad(); }
    }
    pub fn release_button(&mut self, b: usize){
        self.buttons[b] = true;
    }
    pub fn press_joypad(&mut self, b: usize){
        self.joypad[b] = false;
        if self.read_joypad { self.request_joypad(); }
    }
    pub fn release_joypad(&mut self, b: usize){
        self.joypad[b] = true;
    }
    pub fn break_exec(&mut self){
        let br = self.cpu.as_mut().unwrap().break_execution;
        self.cpu.as_mut().unwrap().break_execution = !br;
    }
    pub fn step(&mut self){
        self.cpu.as_mut().unwrap().remaining_steps += 1;
    }
    pub fn get_instruction_offset(&mut self, offset: i32) -> (String, u16) {
        let pc = self.cpu.as_mut().unwrap().reg_pc.saturating_add_signed(offset as i16);
        (self.cpu.as_mut().unwrap().get_instruction_print_at(pc), pc)
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