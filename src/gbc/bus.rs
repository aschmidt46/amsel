#![allow(dead_code)]

use std::{cell::RefCell, cmp::min, rc::Rc};

use crate::gbc::{apu::APU, cartridge::RomObject, ppu::PPU, sm83::{CPUMode, Register8, Register16, SM83}};

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
    pub ppu: Option<PPU>,
    pub apu: Option<APU>,
    cart: Option<RomObject>,
    pub div: u16, //FF04
    tima: u8, //FF05
    tma: u8, //FF06
    tac: u8, //FF07
    oam_dma: u8, // FF46
    tima_last_comparison: bool,
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
    hdma_initiated_now: bool,
    vram_dma_bytes_remaining: u8,
    vram_dma_pointer: u16,
    palette_address: u8,
    palette_address_obj: u8,
    ff72: u8, ff73: u8, ff74: u8, ff75: u8,

    //serial
    sb: u8,
    sc: u8,

    // mbc3
    enable_ram_rtc: bool,
    rtc_s: u8,
    rtc_m: u8,
    rtc_h: u8,
    rtc_dl: u8,
    rtc_dh: u8,

    //mbc1
    mbc1_bank_mode: bool,
    mbc1_magic_register: u8,

    //debug
    pub (crate) br: bool,
    pub (crate) cpu_advanced: bool,
    watch_breakpoints: bool,
    breakpoints: Vec<u16>,
    breakpoints_op: Vec<String>,
    test_mode: bool,
    pub test_output: Option<Vec<u8>>,
}

const TAC_TABLE: [u16; 4] = [256 * 4, 4 * 4, 16 * 4, 64 * 4];
const BIT_TABLE: [u8; 4] = [9,3,5,7];

impl Bus{
    pub fn new() -> Self{
        Bus { work_ram: [[0; 0x1000]; 8], high_ram: [0; 0x80], cart: None, cpu: None, ppu: None, has_frame: false, external_ram_bank: 0, external_ram: [[0; 0x2000]; 16],
            div: 0, tima: 0,  tma: 0, tac: 0, tima_last_comparison: false, joypad: [true; 4], buttons: [true; 4], read_buttons: false, read_joypad: false,
            oam_dma: 0, dma_next_clock: -1, rom_bank_lower: 0, rom_bank_upper: 1, rtc_s: 0, rtc_m: 0, rtc_h: 0, rtc_dl: 0, rtc_dh: 0, enable_ram_rtc: false, // sicher?
            cgb_mode: false, wram_bank: 1, hdma1: 0, hdma2: 0, hdma3: 0, hdma4: 0, hdma5: 0, vram_dma_active: DmaMode::Finished, vram_dma_bytes_remaining: 0,
            vram_dma_pointer: 0, br: false, palette_address: 0, palette_address_obj: 0, ff72: 0, ff73: 0, ff74: 0, ff75: 0, mbc1_bank_mode: false, mbc1_magic_register: 0,
            sb: 0, sc: 0, hdma_initiated_now: false, cpu_advanced: false, apu: None, watch_breakpoints: false, breakpoints: Vec::new(), breakpoints_op: Vec::new(),
            test_mode: false, test_output: None
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
    pub fn set_test_mode(&mut self){
        self.test_mode = true;
        self.test_output = Some(Vec::new())
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
        let apu = APU::new(Rc::downgrade(&this));
        self.cpu = Some(cpu);
        self.ppu = Some(ppu);
        self.apu = Some(apu);
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
            0x0000..0x4000 => match cart.cartridge_type{
                super::cartridge::Mapper::MBC1 => {
                    if self.mbc1_bank_mode{
                        cart.raw_data[((self.mbc1_magic_register as usize) << 19) + self.rom_bank_lower * 0x4000 + addr as usize]
                    }
                    else{
                        cart.raw_data[addr as usize]
                    }
                },
                _ => cart.raw_data[self.rom_bank_lower * 0x4000 + addr as usize]
            }
            0x4000..0x8000 => cart.raw_data[((self.mbc1_magic_register as usize) << 19) + self.rom_bank_upper * 0x4000 + addr as usize - 0x4000],
            0xA000..0xC000 => { // Ram auf Cartridge und tauschbare Bank, falls vorhanden
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC1 => {
                        if self.enable_ram_rtc {
                            if self.mbc1_bank_mode{
                                self.external_ram[self.mbc1_magic_register as usize][addr as usize - 0xA000]
                            }
                            else{
                                self.external_ram[0][addr as usize - 0xA000]
                            }
                        }
                        else {0xFF}
                    }
                    super::cartridge::Mapper::MBC2 => {
                        let real_address = (addr - 0xA000) & 511; // untere 9 bits
                        self.external_ram[0][real_address as usize] & 0b1111 // untere 4 bits, ram besteht nur als "Half Bytes"
                    }
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
            0xFF01 => self.sb,
            0xFF02 => self.sc,
            0xFF04 => (self.div >> 8) as u8,
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
                    self.vram_dma_bytes_remaining
                }
                else {0xFF}
            },
            0xFF68 => if self.cgb_mode {self.palette_address} else {0xFF},
            0xFF69 => if self.cgb_mode && ppu.get_mode() != 3 {ppu.palette_ram[(self.palette_address & 0b111111) as usize]} else {0xFF},
            0xFF6A => if self.cgb_mode {self.palette_address_obj} else {0xFF},
            0xFF6B => if self.cgb_mode && ppu.get_mode() !=3 {ppu.palette_ram_obj[(self.palette_address_obj & 0b111111) as usize]} else {0xFF},
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
            0xFF10..0xFF27 => self.apu.as_mut().unwrap().on_read(addr),
            0xFF30..0xFF40 => self.apu.as_mut().unwrap().on_read(addr),
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
                    super::cartridge::Mapper::MBC1 => {
                        if (val & 0b1111) == 0x0A {
                            self.enable_ram_rtc = true;
                        }
                        else {
                            self.enable_ram_rtc = false;
                        }
                    }
                    super::cartridge::Mapper::MBC2 => {
                        let b8 = (addr & 256) > 0;
                        if b8{
                            let rb = val & 0b1111;
                            if rb == 0 {self.rom_bank_upper = 1} else {self.rom_bank_upper = rb as usize}
                        }
                        else{
                            if (val & 0b1111) == 0x0A {
                                self.enable_ram_rtc = true;
                            }
                            else {
                                self.enable_ram_rtc = false;
                            }
                        }
                    }
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
                    super::cartridge::Mapper::MBC1 => {
                        self.rom_bank_upper = (val & 31) as usize;
                        if self.rom_bank_upper == 0 { self.rom_bank_upper = 1 }
                        // Erst danach maskiert, um duplizieren von Bank 0 zu erlauben
                        self.rom_bank_upper = self.rom_bank_upper & (cart.rom_size_mask as usize);
                    },
                    super::cartridge::Mapper::MBC2 => {
                        let b8 = (addr & 256) > 0;
                        if b8{
                            let rb = val & 0b1111;
                            if rb == 0 {self.rom_bank_upper = 1} else {self.rom_bank_upper = rb as usize}
                        }
                        else{
                            if (val & 0b1111) == 0x0A {
                                self.enable_ram_rtc = true;
                            }
                            else {
                                self.enable_ram_rtc = false;
                            }
                        }
                    },
                    super::cartridge::Mapper::MBC3 => {
                        self.rom_bank_upper = (val & 127) as usize;
                        if self.rom_bank_upper == 0 { self.rom_bank_upper = 1 }
                        self.rom_bank_upper = self.rom_bank_upper & (cart.rom_size_mask as usize);
                    },
                    super::cartridge::Mapper::MBC5 => { // Untere 8 bit der Bank number
                        self.rom_bank_upper = (self.rom_bank_upper & (!0xFF)) | (val as usize);
                    },
                    _ => ()
                }
            }
            0x3000..0x4000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC1 => {
                        self.rom_bank_upper = (val & 31) as usize;
                        if self.rom_bank_upper == 0 { self.rom_bank_upper = 1 }
                        self.rom_bank_upper = self.rom_bank_upper & (cart.rom_size_mask as usize);
                    },
                    super::cartridge::Mapper::MBC2 => {
                        let b8 = (addr & 256) > 0;
                        if b8{
                            let rb = val & 0b1111;
                            if rb == 0 {self.rom_bank_upper = 1} else {self.rom_bank_upper = rb as usize}
                        }
                        else{
                            if (val & 0b1111) == 0x0A {
                                self.enable_ram_rtc = true;
                            }
                            else {
                                self.enable_ram_rtc = false;
                            }
                        }
                    },
                    super::cartridge::Mapper::MBC3 => {
                        self.rom_bank_upper = (val & 127) as usize;
                        if self.rom_bank_upper == 0 { self.rom_bank_upper = 1 }
                        self.rom_bank_upper = self.rom_bank_upper & (cart.rom_size_mask as usize);
                    },
                    super::cartridge::Mapper::MBC5 => { // Neuntes Bit der Bank number
                        self.rom_bank_upper = (self.rom_bank_upper & !(1 << 8)) | (((val as usize) & 1) << 8);
                    },
                    _ => ()
                }
            }
            0x4000..0x5000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC1 => self.mbc1_magic_register = val & 0b11,
                    super::cartridge::Mapper::MBC3 => self.external_ram_bank = val as usize,
                    super::cartridge::Mapper::MBC5 => self.external_ram_bank = val as usize,
                    _ => (),
                }
            }
            0x5000..0x6000 => {
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC1 => self.mbc1_magic_register = val & 0b11,
                    super::cartridge::Mapper::MBC3 => self.external_ram_bank = val as usize,
                    _ => (),
                }
            }
            0x6000..0x8000 => {
                self.mbc1_bank_mode = (val & 1) > 0;
            }
            0xA000..0xC000 => { // Ram auf Cartridge und tauschbare Bank, falls vorhanden
                match cart.cartridge_type{
                    super::cartridge::Mapper::MBC1 => {
                        if self.enable_ram_rtc {
                            self.external_ram[self.external_ram_bank % 4][addr as usize - 0xA000] = val
                        }
                    }
                    super::cartridge::Mapper::MBC2 => {
                        let real_address = (addr - 0xA000) & 511; // untere 9 bits
                        self.external_ram[0][real_address as usize] = val & 0b1111 // untere 4 bits, ram besteht nur als "Half Bytes"
                    }
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
            0xFF01 => self.sb = val,
            0xFF02 => {
                if self.test_mode && val == 0x81{
                    // Nur Debug Ausgabe
                    self.test_output.as_mut().unwrap().push(self.sb);
                }
                else{ // Normales Verhalten
                    let speed = if self.cgb_mode {val & 0b10} else {0};
                    let clock_select = val & 1;
                    let transfer_enable = val & 128;
                    self.sc = speed | clock_select | transfer_enable;
                }
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
                cpu.total_cycles += 640;
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
            0xFF53 => if self.cgb_mode {self.hdma3 = val & 0b00011111},
            0xFF54 => if self.cgb_mode {self.hdma4 = val & 0b11110000},
            0xFF55 => if self.cgb_mode {
                if self.vram_dma_active != DmaMode::Finished {
                    if (val & 128) == 0 {self.vram_dma_active = DmaMode::Finished; self.vram_dma_bytes_remaining |= 128;}
                }
                else{
                    self.vram_dma_pointer = 0;
                    self.vram_dma_bytes_remaining = val & 127;
                    if val & 128 > 0{
                        self.vram_dma_active = DmaMode::Hblank;
                        self.hdma_initiated_now = true;
                    }
                    else{
                        self.vram_dma_active = DmaMode::VramDma;
                        cpu.remaining_cycles += if cpu.dual_speed_mode {64 * ((self.vram_dma_bytes_remaining as i32) + 1)} else {32 * ((self.vram_dma_bytes_remaining as i32) + 1)};
                        cpu.total_cycles += if cpu.dual_speed_mode {64 * ((self.vram_dma_bytes_remaining as usize) + 1)} else {32 * ((self.vram_dma_bytes_remaining as usize) + 1)};
                    }
                }
            },
            0xFF68 => if self.cgb_mode {self.palette_address = val},
            0xFF69 => {
                if self.cgb_mode{
                    if ppu.get_mode() != 3 {ppu.palette_ram[(self.palette_address & 0b111111) as usize] = val};

                    if (self.palette_address & 128) > 0{
                        self.palette_address += 1;
                        if self.palette_address & 0b1111111 > 0b111111 {self.palette_address -= 64}
                    }
                }
            },
            0xFF6A => if self.cgb_mode {self.palette_address_obj = val},
            0xFF6B => {
                if self.cgb_mode{
                    if ppu.get_mode() != 3 {ppu.palette_ram_obj[(self.palette_address_obj & 0b111111) as usize] = val};

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
            0xFF10..0xFF27 => self.apu.as_mut().unwrap().maybe_write(addr, val),
            0xFF30..0xFF40 => self.apu.as_mut().unwrap().maybe_write(addr, val),
            0xFFFF => cpu.ie_reg = val,
            _ => (),//panic!("Konnte nicht schreiben!"),
        }
    }

    fn get_path(&mut self, addr: u16) -> Option<&mut u8>{
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
    pub fn cpu_has_advanced(&mut self) -> bool{
        let res = self.cpu_advanced;
        self.cpu_advanced = false;
        res
    }
    pub fn force_ly(&mut self, val: u8){
        self.ppu.as_mut().unwrap().force_ly(val);
    }
    pub fn force_ppu_cycle(&mut self, val: usize){
        self.ppu.as_mut().unwrap().force_cycle(val);
    }
    pub fn force_cpu_cycle(&mut self, val: usize){
        self.cpu.as_mut().unwrap().total_cycles = val;
    }
    pub fn get_audio_sample_left(&self) -> f64{
        self.apu.as_ref().unwrap().get_sample_left()
    }
    pub fn get_audio_sample_right(&self) -> f64{
        self.apu.as_ref().unwrap().get_sample_right()
    }
    pub fn clock(&mut self){

        if self.watch_breakpoints && self.cpu.as_mut().unwrap().remaining_steps == 0{
            let pc_val = self.cpu.as_mut().unwrap().reg_pc;
            if self.breakpoints.contains(&pc_val){
                self.br = true;
            }
            let memval = self.read_memory(pc_val);
            let opcode = self.cpu.as_mut().unwrap().get_mnemonic(memval);
            if self.breakpoints_op.contains(&opcode){
                self.br = true;
            }
        }

        if (!self.br) || (self.cpu.as_mut().unwrap().remaining_steps > 0) {
            let dual_speed = self.cpu.as_mut().unwrap().dual_speed_mode;

            // Timer Quelle: https://github.com/Ashiepaws/GBEDG/blob/master/timers/index.md

            let prev_div = (self.div >> 8) as u8;

            if self.cpu.as_mut().unwrap().mode != CPUMode::Stopped{
                self.div = self.div.wrapping_add(1);
                if dual_speed{
                    self.div = self.div.wrapping_add(1);
                }
            }
            let bit_pos = BIT_TABLE[(self.tac & 0b11) as usize];
            let div_bit = (self.div & (1 << bit_pos)) > 0;
            let and_res = ((self.tac & 0b100) > 0) && div_bit;

            if !and_res && self.tima_last_comparison {
                match self.tima.checked_add(1){
                    None => {self.tima = self.tma; self.request_timer();},
                    Some(val) => { self.tima = val },
                }
            }

            self.tima_last_comparison = and_res;

            //apu div clock
            let new_div = (self.div >> 8) as u8;
            let bits = if dual_speed {5} else {4};
            if ((prev_div & (1 << bits)) > 0) && ((new_div & (1 << bits)) == 0){
                self.apu.as_mut().unwrap().clock_div_apu();
            }
    
    
            let dma_source = ((self.hdma1 as u16) << 8) | (self.hdma2 as u16);
            let dma_dest = ((self.hdma3 as u16) << 8) | (self.hdma4 as u16) | 0x8000;
            match self.vram_dma_active{
                DmaMode::Finished => {
                    self.cpu_advanced = self.cpu.as_mut().unwrap().clock();
                    if dual_speed {
                        self.cpu_advanced = self.cpu.as_mut().unwrap().clock();
                    }
                },
                DmaMode::VramDma => {
                    let byte_len = (self.vram_dma_bytes_remaining as u16 + 1) * 0x10;
                    for i in 0..byte_len{
                        let v = self.read_memory(dma_source + i);
                        self.write_memory(dma_dest + i, v);
                    }
                    self.vram_dma_bytes_remaining = 0xFF;
                    self.vram_dma_active = DmaMode::Finished;
                },
                DmaMode::Hblank => {
                    if self.ppu.as_mut().unwrap().get_mode() == 0 && self.cpu.as_mut().unwrap().mode == CPUMode::Running{
                        if self.ppu.as_mut().unwrap().first_hblank_cycle || self.hdma_initiated_now {
                            self.hdma_initiated_now = false;
                            self.ppu.as_mut().unwrap().first_hblank_cycle = false;
                            let byte_len = (self.vram_dma_bytes_remaining as u16 + 1) * 0x10;
                            for i in 0..min(0x10, byte_len){
                                let v = self.read_memory(dma_source + i + self.vram_dma_pointer);
                                self.write_memory(dma_dest + i + self.vram_dma_pointer, v);
                            }
                            self.vram_dma_pointer += 0x10;
                            self.cpu.as_mut().unwrap().remaining_cycles += if self.cpu.as_mut().unwrap().dual_speed_mode {64} else {32};
                            self.cpu.as_mut().unwrap().total_cycles += if self.cpu.as_mut().unwrap().dual_speed_mode {64} else {32};
                            self.vram_dma_bytes_remaining = match self.vram_dma_bytes_remaining.checked_sub(1){
                                Some(vr) => vr,
                                None => {
                                    self.vram_dma_active = DmaMode::Finished;
                                    0xFF
                                },
                            }
                        }
                    }
                    self.cpu_advanced = self.cpu.as_mut().unwrap().clock();
                    if dual_speed {
                        self.cpu_advanced = self.cpu.as_mut().unwrap().clock();
                    }
                },
            }

            self.ppu.as_mut().unwrap().clock(self.cgb_mode);
            self.apu.as_mut().unwrap().clock();
    
            if self.dma_next_clock >= 0{
                let mut oams = self.ppu.as_mut().unwrap().oam;
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
    pub fn set_break(&mut self, val: bool){
        self.br = val;
    }
    pub fn step(&mut self){
        self.cpu.as_mut().unwrap().remaining_steps += 1;
    }
    pub fn get_next_n_instructions(&mut self, n: i32) -> (String, Vec<i32>){
        self.cpu.as_mut().unwrap().get_next_n_instructions(n)
    }
    pub fn get_prev_10_instructions(&mut self) -> (String, Vec<i32>){
        self.cpu.as_mut().unwrap().get_prev_10_instructions()
    }
    pub fn is_halted(&self) -> bool{
        self.br
    }
    pub fn add_breakpoint(&mut self, bp: u16) -> Vec<u16>{
        if !self.watch_breakpoints{
            self.watch_breakpoints = true;
        }
        let exists = self.breakpoints.contains(&bp);
        if !exists{
            self.breakpoints.push(bp);
        }
        self.breakpoints.clone()
    }
    pub fn add_breakpoint_op(&mut self, bp: String) -> Vec<String>{
        if !self.watch_breakpoints{
            self.watch_breakpoints = true;
        }
        let exists = self.breakpoints_op.contains(&bp);
        if !exists{
            self.breakpoints_op.push(bp);
        }
        self.breakpoints_op.clone()
    }
    pub fn remove_breakpoint(&mut self, bp: u16) -> Vec<u16>{
        self.breakpoints.retain(|x| *x != bp);

        if self.watch_breakpoints && self.breakpoints.len() == 0 && self.breakpoints_op.len() == 0{
            self.watch_breakpoints = false;
        }
        self.breakpoints.clone()
    }
    pub fn remove_breakpoint_op(&mut self, bp: String) -> Vec<String>{
        self.breakpoints_op.retain(|x| *x != bp);

        if self.watch_breakpoints && self.breakpoints.len() == 0 && self.breakpoints_op.len() == 0{
            self.watch_breakpoints = false;
        }
        self.breakpoints_op.clone()
    }
    pub fn get_mnemonic(&mut self, index: usize) -> String{
        self.cpu.as_mut().unwrap().get_mnemonic(index as u8)
    }
    pub fn read_register_8(&mut self, reg: Register8) -> u8{
        self.cpu.as_mut().unwrap().get_8(reg)
    }
    pub fn read_register_16(&mut self, reg: Register16) -> u16{
        self.cpu.as_mut().unwrap().get_16(reg)
    }
    pub fn game_can_save(&self) -> bool{
        self.cart.as_ref().unwrap().battery_ram
    }
    pub fn get_save_data(&mut self) -> Vec<u8>{
        let mut v = vec![0; 0x2000 * (self.cart.as_ref().unwrap().ram_size as usize)];
        for i in 0.. v.len(){
            v[i] = self.external_ram[i / 0x2000][i % 0x2000];
        }
        v
    }
    pub fn get_save_size(&self) -> usize{
        0x2000 * (self.cart.as_ref().unwrap().ram_size as usize)
    }
    pub fn load_save(&mut self, data: Vec<u8>){
        for i in 0..data.len(){
            self.external_ram[i / 0x2000][i % 0x2000] = data[i];
        }
    }
}

#[cfg(test)]
mod test{
    use std::{cell::RefCell, rc::Rc};

    #[test]
    fn read_write_test(){
        let b = Rc::new(RefCell::new(super::Bus::new()));
        b.borrow_mut().create_components(b.clone());
        assert_eq!(b.borrow_mut().read_memory(0xC000), 0);
        b.borrow_mut().write_memory(0xC000, 0xAA);
        assert_eq!(b.borrow_mut().read_memory(0xC000), 0xAA);
    }
}