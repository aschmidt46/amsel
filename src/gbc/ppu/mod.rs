use std::{cell::RefCell, rc::{Rc, Weak}};

use crate::{FRAMEBUFFER, gbc::bus::Bus, index_framebuffer};


pub struct PPU{
    pub (crate) vram: [[u8; 0x2000]; 2], // Beide Bänke zu je 8KiB
    pub (crate) bank_select: usize,
    bus: Weak<RefCell<Bus>>,
    pub (crate) scanline: u8, // aka ly (read only)
    new_scanline: bool,
    pub (crate) oam: [u8; 160],
    secondary_oam: Vec<[u8; 4]>,
    cycle: usize,
    pub (crate) lcdc: u8, //LCD Control
    pub (crate) stat: u8, // FF41 (lcd status)
    pub (crate) lyc: u8, //FF45 (ly compare)
    pub (crate) scy: u8, // FF42 (y scroll bg)
    pub (crate) scx: u8, // FF42 (x scroll bg)
    pub (crate) wy: u8, // FF4A (y scroll window)
    pub (crate) wx: u8, // FF4B (x scroll window)
    pub (crate) bgp: u8, // FF47 (bg palette) nur dmg
    pub (crate) obp0: u8, // FF48 (ob palette 0) nur dmg
    pub (crate) obp1: u8, // FF49 (ob palette 1) nur dmg
}

impl PPU{
    pub fn new(bus: Weak<RefCell<Bus>>) -> Self{
        PPU { vram: [[0; 0x2000]; 2], bus, bank_select: 0 , scanline: 0, cycle: 0, new_scanline: true, stat: 0, lyc: 0, lcdc: 0, scx: 0, scy: 0, wx: 0, wy: 0, secondary_oam: Vec::new(), oam: [0; 160], bgp: 0, obp0: 0, obp1: 0}
    }
    pub fn write_stat(&mut self, val: u8){
        self.stat = (self.stat & !0b01111000) | (val & 0b01111000);
    }
    pub fn clock(&mut self){
        self.next_dot();

        self.stat &= !0b111;
        if self.scanline == self.lyc {
            self.stat |= 0b100;
        }

        // ppu mode
        if self.lcdc & 0b10000000 > 0 {
            if self.scanline >= 144 {
                self.stat |= 1;
            }
            else{
                if self.cycle < 80{
                    self.stat |= 2;
                }
                else if self.cycle < 80 + 160 {
                    self.stat |= 3;
                }
            }
        }

        //stat interrupt
        let mut cond = if (self.stat & 0b01000000) > 0 && self.lyc == self.scanline {true} else {false};
        for m in 3..6{
            let mode = m - 3;
            if self.stat & (1 << m) > 0{
                cond |= (self.stat & 0b11) == mode;
            }
        }
        if cond {
            unsafe{
                match self.bus.upgrade(){
                    Some(b) => (*b.as_ptr()).request_stat(),
                    _ => panic!("Kein Bus in PPU vorhanden!"),
                }
            }
        }
    }
    pub fn read(&mut self, addr: u16) -> u8{
        match &self.bus.upgrade(){
            None => 0,
            Some(b) => unsafe {(*b.as_ptr()).read_memory(addr)},
        }
    }
    fn bg_tile_map_area(&self) -> bool{
        self.lcdc & 0b00001000 > 0
    }
    fn tile_address_mode(&self) -> bool{
        self.lcdc & 0b00010000 > 0
    }
    fn window_enable(&self) -> bool{
        self.lcdc & 0b00100000 > 0
    }
    fn window_tile_map(&self) -> bool{
        self.lcdc & 0b01000000 > 0
    }
    fn mode_8_16(&self) -> bool{
        self.lcdc & 0b100 > 0
    }
    fn obj_enable(&self) -> bool{
        self.lcdc & 0b10 > 0
    }
    fn get_color_bgp(&self, index: u8) -> u8{
        (self.bgp & (0b11 << (index * 2))) >> (index * 2)
    }
    fn get_color_obp0(&self, index: u8) -> u8{
        (self.obp0 & (0b11 << (index * 2))) >> (index * 2)
    }
    fn get_color_obp1(&self, index: u8) -> u8{
        (self.obp1 & (0b11 << (index * 2))) >> (index * 2)
    }
    fn scan_sprites(&mut self){
        self.secondary_oam = Vec::new();

        let mut oam_ptr = 0;

        while self.secondary_oam.len() <= 10 && oam_ptr < 40 {
            let y_pos = self.oam[oam_ptr * 4];
            let x_pos = self.oam[oam_ptr * 4 + 1];
            let tile_index = self.oam[oam_ptr * 4 + 2];
            let attributes = self.oam[oam_ptr * 4 + 3];
            let y_pos_i = (y_pos as i32) - 16;
            let y_size = if self.mode_8_16() {16} else {8};
            let on_line = (self.scanline as i32) >= y_pos_i && (self.scanline as i32) < y_pos_i + y_size;

            if on_line{
                self.secondary_oam.push([y_pos, x_pos, tile_index, attributes]);
            }
            oam_ptr += 1;
        }
        // Hier eigentlich IF Mode DMG else nur reverse
        self.secondary_oam.sort_by(|a, b| a[1].cmp(&b[1]).reverse());
    }
    fn next_dot(&mut self){

        if self.cycle == 79 && self.scanline < 144{
            self.scan_sprites();
        }

        if self.cycle >= 80 && self.cycle < 240 && self.scanline < 144 {

            // Background

            let mut bg_color: u8 = 0;

            let px = self.scx.wrapping_add((self.cycle as u8).saturating_sub(80));
            let py = self.scy.wrapping_add(self.scanline);
            let tile_x = px / 8;
            let tile_y = py / 8;
            let x_offset = px % 8;
            let y_offset = py % 8;
                                    // start            select                         größe    x index            y index * zeile
            let tile_index = self.read(0x9800 + ((self.bg_tile_map_area() as u16) * 0x400) + tile_x as u16 + (tile_y as u16 * 32));
            let mut tile_start = if !self.tile_address_mode() && tile_index < 128{   
                0x9000 + (tile_index as u16 * 16)
            }
            else { 0x8000 + (tile_index as u16 * 16) };
            tile_start += 2 * (y_offset as u16);
            let tile_lower = self.read(tile_start);
            let tile_higher = self.read(tile_start + 1);
            let val = ((tile_lower >> (7 - x_offset)) & 1) | (((tile_higher >> (7 - x_offset)) & 1) << 1);
            bg_color = val;

            // Window

            if self.window_enable(){
                let pxx = (self.wx as i32).saturating_sub(7).wrapping_add((self.cycle as i32).saturating_sub(80));
                if pxx >= 0 && self.scanline >= self.wy{
                    let px = pxx as u8;
                    let py = self.wy.wrapping_add(self.scanline);
                    let tile_x = px / 8;
                    let tile_y = py / 8;
                    let x_offset = px % 8;
                    let y_offset = py % 8;
                                            // start            select                              größe    x index            y index * zeile
                    let tile_index = self.read(0x9800 + ((self.window_tile_map() as u16) * 0x400) + tile_x as u16 + (tile_y as u16 * 32));
                    let mut tile_start = if !self.tile_address_mode() && tile_index < 128{   
                        0x9000 + (tile_index as u16 * 16)
                    }
                    else { 0x8000 + (tile_index as u16 * 16) };
                    tile_start += 2 * (y_offset as u16);
                    let tile_lower = self.read(tile_start);
                    let tile_higher = self.read(tile_start + 1);
                    let val = ((tile_lower >> (7 - x_offset)) & 1) | (((tile_higher >> (7 - x_offset)) & 1) << 1);
                    bg_color = val;
                }
            }

            // Sprites

            let mut sprite_overwrote_bg = false;

            if self.obj_enable(){
                let pixel_x = self.cycle.saturating_sub(80) as u8;
                for oam in self.secondary_oam.clone(){

                    let y_pos_i = (oam[0] as i32) - 16;
                    let x_pos = (oam[1] as i32) - 8;
                    let mut  y_offset = (self.scanline as i32) - y_pos_i;
                    let x_offset = (pixel_x as i32) - x_pos;
                    let priority = oam[3] & 128 > 0;
                    let y_flip = oam[3] & 64 > 0;
                    let x_flip = oam[3] & 32 > 0;
                    let fg_palette = oam[3] & 16 > 0;
                    // Pixel wird gezeichnet für diesen Sprite
                    if !(priority && bg_color > 0){
                        if x_pos <= pixel_x as i32 && x_pos + 8 > pixel_x as i32{
                            let tile_index = if self.mode_8_16() {
                                if y_offset >= 8 {
                                    if y_flip {2 * oam[2]} else {2 * oam[2] + 1}
                                } else {
                                    if y_flip {2 * oam[2] + 1} else {2 * oam[2]}
                                }
                            } else {oam[2]};
                            if self.mode_8_16() && y_offset > 8 {y_offset -= 8};
                            let ad_y = if y_flip {8 - y_offset} else {y_offset};
                            let tile_low = self.read(0x8000 + ((tile_index as u16) * 16) + (ad_y as u16) * 2);
                            let tile_high = self.read(0x8000 + ((tile_index as u16) * 16) + (ad_y as u16) * 2 + 1);
                            let fg_color;
                            if x_flip{
                                fg_color = ((tile_low >> (x_offset)) & 1) | (((tile_high >> (x_offset)) & 1) << 1);
                            }
                            else{
                                fg_color = ((tile_low >> (7 - x_offset)) & 1) | (((tile_high >> (7 - x_offset)) & 1) << 1);
                            }
                            if fg_color > 0{
                                let real_fg_color = if fg_palette {self.get_color_obp1(fg_color)} else {self.get_color_obp0(fg_color)};
                                bg_color = real_fg_color;
                                sprite_overwrote_bg = true;
                            }
                        }
                    }
                }
            }

            if sprite_overwrote_bg{ // Index ist bereits Farbe
                self.set_pixel_palette(self.cycle.saturating_sub(80), self.scanline as usize, bg_color);
            }
            else{ // BGP Index
                self.set_pixel_palette(self.cycle.saturating_sub(80), self.scanline as usize, self.get_color_bgp(bg_color));
            }


        }

        if self.scanline == 144 && self.cycle == 0 {
            unsafe{
                match self.bus.upgrade(){
                    Some(b) => (*b.as_ptr()).request_vblank(),
                    _ => panic!("Kein Bus in PPU vorhanden!"),
                }
            }
        }


        self.cycle += 1;
        if self.cycle >= 456{
            self.cycle = 0;
            self.scanline += 1;
        }
        if self.scanline >= 154{
            self.scanline = 0;
            self.new_scanline = true;
            unsafe{
                match self.bus.upgrade(){
                    Some(b) => (*b.as_ptr()).has_frame = true,
                    _ => panic!("Kein Bus in PPU vorhanden!"),
                }
            }
        }
    }
    fn set_pixel(&self, x: usize, y: usize, r: u8, g: u8, b: u8){
        match FRAMEBUFFER.lock().as_mut(){
            Ok(m) => {
                m[index_framebuffer(x, y)] = r;
                m[index_framebuffer(x, y) + 1] = g;
                m[index_framebuffer(x, y) + 2] = b;
                m[index_framebuffer(x, y) + 3] = 255;
            },
            Err(e) => panic!("Konnte Framebuffer nicht sperren!"),
        }
    }
    fn set_pixel_palette(&self, x: usize, y: usize, val: u8){
        let col = 255 - ((val as f32 / 3.0) * 255.0) as u8;
        self.set_pixel(x, y, col, col, col);
    }
}
