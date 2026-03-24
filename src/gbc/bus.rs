use std::{cell::RefCell, cmp::min, rc::Rc};

use crate::gbc::{cartridge::RomObject, ppu::PPU, sm83::{Opcode, OperandType, SM83}};

#[derive(PartialEq)]
enum DmaMode{
    VramDma,
    Hblank,
    Finished
}

pub struct Bus{
    work_ram: [[u8; 0x1000]; 8], // 32KiB (8 Bänke a 4KiB), 8KiB auf DMG
    high_ram: [u8; 0x80],
    external_ram_bank: usize,
    rom_bank_lower: usize,
    rom_bank_upper: usize,
    external_ram: [[u8; 0x2000]; 16], //max 16 Bänke
    pub cpu: Option<SM83>,
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

    //cgb
    cgb_mode: bool,
    wram_bank: usize,
    hdma1: u8, hdma2: u8, hdma3: u8, hdma4: u8, hdma5: u8,
    vram_dma_active: DmaMode,
    vram_dma_bytes_remaining: u8,
    vram_dma_pointer: u16,
    palette_address: u8,
    palette_address_obj: u8,
    ff72: u8, ff73: u8, ff74: u8, ff75: u8,

    // mbc3
    enable_ram_rtc: bool,
    rtc_s: u8,
    rtc_m: u8,
    rtc_h: u8,
    rtc_dl: u8,
    rtc_dh: u8,

    //debug
    pub (crate) br: bool,
}

const TAC_TABLE: [u16; 4] = [256 * 4, 4 * 4, 16 * 4, 64 * 4];

impl Bus{
    pub fn new() -> Self{
        Bus { work_ram: [[0; 0x1000]; 8], high_ram: [0; 0x80], cart: None, cpu: None, ppu: None, has_frame: false, external_ram_bank: 0, external_ram: [[0; 0x2000]; 16],
            div: 0, tima: 0,  tma: 0, tac: 0, div_counter: 0, tima_counter: 0, joypad: [true; 4], buttons: [true; 4], read_buttons: false, read_joypad: false,
            oam_dma: 0, dma_next_clock: -1, rom_bank_lower: 0, rom_bank_upper: 1, rtc_s: 0, rtc_m: 0, rtc_h: 0, rtc_dl: 0, rtc_dh: 0, enable_ram_rtc: true,
            cgb_mode: false, wram_bank: 1, hdma1: 0, hdma2: 0, hdma3: 0, hdma4: 0, hdma5: 0, vram_dma_active: DmaMode::Finished, vram_dma_bytes_remaining: 0,
            vram_dma_pointer: 0, br: false, palette_address: 0, palette_address_obj: 0, ff72: 0, ff73: 0, ff74: 0, ff75: 0,
        }
    }
    pub fn new_init(path: &str) -> Self{
        let mut bus = Bus::new();
        bus.cart = RomObject::new(path).ok();
        if bus.cart.as_mut().unwrap().cgb_flag & 0xC0 > 0 {
            bus.cgb_mode = true;
        }
        bus
    }
    pub fn create_components(&mut self, this: Rc<RefCell<Bus>>){
        let mut cpu = SM83::new_init(Rc::downgrade(&this));
        if self.cgb_mode {
            cpu.set_initial_state_cgb();
        }
        else{
            cpu.set_initial_state_dmg();
        }
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
            0x0000..0x4000 => cart.raw_data[self.rom_bank_lower * 0x4000 + addr as usize],
            0x4000..0x8000 => cart.raw_data[self.rom_bank_upper * 0x4000 + addr as usize - 0x4000],
            0xA000..0xC000 => { // Ram auf Cartridge und tauschbare Bank, falls vorhanden
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC3 => {
                        if self.enable_ram_rtc{
                            match self.external_ram_bank{
                                0..8 => self.external_ram[self.external_ram_bank][addr as usize - 0xA000],
                                0x8 => self.rtc_s,
                                0x9 => self.rtc_m,
                                0xA => self.rtc_h,
                                0xB => self.rtc_dl,
                                0xC => self.rtc_dh,
                                _ => self.external_ram[self.external_ram_bank % 8][addr as usize - 0xA000],
                            }
                        }
                        else { 0xFF }
                    }
                    super::cartridge::Mapper::MBC5 => {
                        if self.enable_ram_rtc{
                            self.external_ram[self.external_ram_bank % 0x10][addr as usize - 0xA000]
                        }
                        else { 0xFF }
                    },
                    _ => self.external_ram[self.external_ram_bank][addr as usize - 0xA000]
                }
            },
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
            0xFF47 => ppu.bgp,
            0xFF48 => ppu.obp0,
            0xFF49 => ppu.obp1,
            0xFF4A => ppu.wy,
            0xFF4B => ppu.wx,
            0xFF44 => ppu.scanline,
            0xFF45 => ppu.lyc,
            0xFF4C => if self.cgb_mode {0} else {0xFF}, // Full CGB Mode
            0xFF6C => 0, //CGB style Priorität
            0xFF4D => {
                if self.cgb_mode {
                    ((cpu.dual_speed_mode as u8) << 7) | (cpu.speed_switch_armed as u8)
                }
                else { 0xFF}
            },
            0xFF4F => {
                if self.cgb_mode {
                    (ppu.bank_select as u8) | 0b11111110
                }
                else{0xFF}
            },
            0xFF55 => {
                if self.cgb_mode{
                    match self.vram_dma_active{
                        DmaMode::Finished => 0xFF,
                        DmaMode::VramDma => 0xFF,
                        DmaMode::Hblank => self.vram_dma_bytes_remaining,
                    }
                }
                else {0xFF}
            },
            0xFF68 => if self.cgb_mode {self.palette_address} else {0xFF},
            0xFF69 => if self.cgb_mode {ppu.palette_ram[(self.palette_address & 0b111111) as usize]} else {0xFF},
            0xFF6A => if self.cgb_mode {self.palette_address_obj} else {0xFF},
            0xFF6B => if self.cgb_mode {ppu.palette_ram_obj[(self.palette_address_obj & 0b111111) as usize]} else {0xFF},
            0xFF70 => {
                if self.cgb_mode {
                    self.wram_bank as u8
                }
                else {0xFF}
            },
            0xFF72 => self.ff72,
            0xFF73 => self.ff73,
            0xFF74 => if self.cgb_mode {self.ff74} else {0xFF},
            0xFF75 => self.ff75,
            0xFFFF => cpu.ie_reg,
            _ => 0xFF, // IO Register geben das zurück, wenn keine Tasten gedrückt
        }
    }
    fn write_side_effects(&mut self, addr: u16, val: u8){
        let cart = self.cart.as_mut().unwrap();
        let ppu = self.ppu.as_mut().unwrap();
        let cpu = self.cpu.as_mut().unwrap();
        match addr{
            0x0000..0x2000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC3 =>{
                        if (val & 0b1111) == 0x0A {
                            self.enable_ram_rtc = true;
                        }
                        else {
                            self.enable_ram_rtc = false;
                        }
                    },
                    super::cartridge::Mapper::MBC5 =>{
                        if (val & 0b1111) == 0x0A {
                            self.enable_ram_rtc = true;
                        }
                        else {
                            self.enable_ram_rtc = false;
                        }
                    },
                    _ => ()
                }
            }
            0x2000..0x3000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC3 => {
                        self.rom_bank_upper = (val & 127 & (cart.rom_size_mask as u8)) as usize;
                        if self.rom_bank_upper == 0 { self.rom_bank_upper = 1 }
                    },
                    super::cartridge::Mapper::MBC5 => { // Untere 8 bit der Bank number
                        self.rom_bank_upper = (self.rom_bank_upper & (!0xFF)) | (val as usize);
                    },
                    _ => ()
                }
            }
            0x3000..0x4000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC3 => {
                        self.rom_bank_upper = (val & 127 & (cart.rom_size_mask as u8)) as usize;
                        if self.rom_bank_upper == 0 { self.rom_bank_upper = 1 }
                    },
                    super::cartridge::Mapper::MBC5 => { // Neuntes Bit der Bank number
                        self.rom_bank_upper = (self.rom_bank_upper & !(1 << 8)) | (((val as usize) & 1) << 8);
                    },
                    _ => ()
                }
            }
            0x4000..0x5000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC3 => self.external_ram_bank = val as usize,
                    super::cartridge::Mapper::MBC5 => self.external_ram_bank = val as usize,
                    _ => (),
                }
            }
            0x5000..0x6000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC3 => self.external_ram_bank = val as usize,
                    _ => (),
                }
            }
            0xA000..0xC000 => { // Ram auf Cartridge und tauschbare Bank, falls vorhanden
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC3 => {
                        if self.enable_ram_rtc{
                            match self.external_ram_bank{
                                0..8 => self.external_ram[self.external_ram_bank][addr as usize - 0xA000] = val,
                                0x8 => self.rtc_s = val,
                                0x9 => self.rtc_m = val,
                                0xA => self.rtc_h = val,
                                0xB => self.rtc_dl = val,
                                0xC => self.rtc_dh = val,
                                _ => self.external_ram[self.external_ram_bank % 8][addr as usize - 0xA000] = val,
                            }
                        }
                    }
                    _ => self.external_ram[self.external_ram_bank][addr as usize - 0xA000] = val
                }
            },
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
            0xFF47 => ppu.bgp = val,
            0xFF48 => ppu.obp0 = val,
            0xFF49 => ppu.obp1 = val,
            0xFF4A => ppu.wy = val,
            0xFF4B => ppu.wx = val,
            0xFF44 => (),
            0xFF45 => {
                ppu.lyc = val;
                if (ppu.stat & 0b01000000) > 0 && (ppu.lyc == ppu.scanline) {
                    self.request_stat();
                }
            },
            0xFF4F => {
                if self.cgb_mode {
                    ppu.bank_select = (val & 1) as usize
                }
            }
            0xFF4D => {
                if self.cgb_mode {
                    cpu.speed_switch_armed = (val & 1) > 0
                }
            },
            0xFF51 => if self.cgb_mode {self.hdma1 = val},
            0xFF52 => if self.cgb_mode {self.hdma2 = val & 0b11110000},
            0xFF53 => if self.cgb_mode {self.hdma3 = val},
            0xFF54 => if self.cgb_mode {self.hdma4 = val & 0b11110000},
            0xFF55 => if self.cgb_mode {
                if self.vram_dma_active != DmaMode::Finished {
                    if (val & 128) == 0 {self.vram_dma_active = DmaMode::Finished;}
                }
                else{
                    self.vram_dma_pointer = 0;
                    self.vram_dma_bytes_remaining = val & 0b01111111;
                    if val & 128 > 0{
                        self.vram_dma_active = DmaMode::Hblank;
                    }
                    else{
                        self.vram_dma_active = DmaMode::VramDma;
                        cpu.remaining_cycles += if cpu.dual_speed_mode {64 * ((self.vram_dma_bytes_remaining as i32) + 1)} else {32 * ((self.vram_dma_bytes_remaining as i32) + 1)}
                    }
                }
            },
            0xFF68 => if self.cgb_mode {self.palette_address = val},
            0xFF69 => {
                if self.cgb_mode{
                    ppu.palette_ram[(self.palette_address & 0b111111) as usize] = val;

                    if (self.palette_address & 128) > 0{
                        self.palette_address += 1;
                        if self.palette_address & 0b1111111 > 0b111111 {self.palette_address -= 64}
                    }
                }
            },
            0xFF6A => if self.cgb_mode {self.palette_address_obj = val},
            0xFF6B => {
                if self.cgb_mode{
                    ppu.palette_ram_obj[(self.palette_address_obj & 0b111111) as usize] = val;

                    if (self.palette_address_obj & 128) > 0{
                        self.palette_address_obj += 1;
                        if self.palette_address_obj & 0b1111111 > 0b111111 {self.palette_address_obj -= 64}
                    }
                }
            },
            0xFF70 => {
                if self.cgb_mode {
                    let bank = val & 0b111;
                    if bank > 0 {self.wram_bank = bank as usize} else {self.wram_bank = 1}
                }
            },
            0xFF72 => self.ff72 = val,
            0xFF73 => self.ff73 = val,
            0xFF74 => self.ff74 = val,
            0xFF75 => self.ff75 = val & 0b01110000,
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
            0xA000..0xC000 => None, //Ram auf Cartridge und tauschbare Bank
            0xC000..0xD000 => Some(&mut self.work_ram[0][addr as usize - 0xC000]),
            0xD000..0xE000 => Some(&mut self.work_ram[self.wram_bank][addr as usize - 0xD000]), // In CGB tauschbar
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
        if (!self.br) || (self.cpu.as_mut().unwrap().remaining_steps > 0) {
            let dual_speed = self.cpu.as_mut().unwrap().dual_speed_mode;
            let div_mod = if dual_speed {128} else {256};
            if self.div_counter % div_mod == 0{
                self.div = self.div.wrapping_add(1);
            }
            self.div_counter = self.div_counter.wrapping_add(1);
    
            let tima_mod = if dual_speed {2} else {1};
    
            if self.tima_counter % ((TAC_TABLE[(self.tac & 0b11) as usize] as usize) / tima_mod) == 0{
                if self.tac % 0b100 > 0{
                    match self.tima.checked_add(1){
                        None => {self.tima = self.tma; self.request_timer();},
                        Some(val) => { self.tima = val },
                    }
                }
            }
            self.tima_counter = self.tima_counter.wrapping_add(1);
    
            self.ppu.as_mut().unwrap().clock(self.cgb_mode);
    
    
            let dma_source = ((self.hdma1 as u16) << 8) | (self.hdma2 as u16);
            let dma_dest = ((self.hdma3 as u16) << 8) | (self.hdma4 as u16);
            match self.vram_dma_active{
                DmaMode::Finished => {
                    self.cpu.as_mut().unwrap().clock();
                    if dual_speed {
                        self.cpu.as_mut().unwrap().clock();
                    }
                },
                DmaMode::VramDma => {
                    let byte_len = (self.vram_dma_bytes_remaining as u16 + 1) * 0x10;
                    for i in 0..byte_len{
                        let v = self.read_memory(dma_source + i);
                        self.write_memory(dma_dest + i, v);
                    }
                    self.vram_dma_bytes_remaining = 0;
                    self.vram_dma_active = DmaMode::Finished;
                },
                DmaMode::Hblank => {
                    if self.ppu.as_mut().unwrap().get_mode() == 0{
                        if self.ppu.as_mut().unwrap().cycle == 455 {
                            let byte_len = (self.vram_dma_bytes_remaining as u16 + 1) * 0x10;
                            for i in 0..0x10{
                                let v = self.read_memory(dma_source + i + self.vram_dma_pointer);
                                self.write_memory(dma_dest + i + self.vram_dma_pointer, v);
                            }
                            self.vram_dma_pointer += 0x10;
                            if self.vram_dma_bytes_remaining == 0{
                                self.vram_dma_active = DmaMode::Finished;
                            }
                            else{
                                self.vram_dma_bytes_remaining -= 1;
                            }
                        }
                    }
                    else{
                        self.cpu.as_mut().unwrap().clock();
                        if dual_speed {
                            if !self.br || self.cpu.as_mut().unwrap().remaining_steps > 0 {
                                self.cpu.as_mut().unwrap().clock();
                            }
                        }
                    }
                },
            }
    
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
        self.br = !self.br;
    }
    pub fn step(&mut self){
        self.cpu.as_mut().unwrap().remaining_steps += 1;
    }
    pub fn get_instruction_offset(&mut self, offset: i16) -> (String, u16) {
        let pc = self.cpu.as_mut().unwrap().reg_pc.saturating_add_signed(offset);
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