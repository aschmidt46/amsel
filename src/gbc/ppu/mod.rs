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
    pub (crate) cycle: usize,
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
    pub (crate) palette_ram: [u8; 64], // nur cgb
    pub (crate) palette_ram_obj: [u8; 64], // nur cgb
    pub (crate) first_hblank_cycle: bool,
    mode_3_penalty: usize,
    num_frames: usize,
    last_coincidence_lyc: u8,

    //window
    wy_condition: bool,
    wx_condition: bool,
    wy_count: u8,
}

impl PPU{
    pub fn new(bus: Weak<RefCell<Bus>>) -> Self{
        PPU { vram: [[0; 0x2000]; 2], bus, bank_select: 0 , scanline: 0, cycle: 0, new_scanline: true, stat: 0, lyc: 0, lcdc: 128, scx: 0, scy: 0, wx: 0, wy: 0,
             secondary_oam: Vec::new(), oam: [0; 160], bgp: 0, obp0: 0, obp1: 0, palette_ram: [255; 64], palette_ram_obj: [255; 64], wy_condition: false, wx_condition: false, wy_count: 0,
             mode_3_penalty: 12, num_frames: 0, last_coincidence_lyc: 255, first_hblank_cycle: false}
    }
    pub fn write_stat(&mut self, val: u8){
        self.stat = (self.stat & !0b01111000) | (val & 0b01111000);
    }
    pub fn get_mode(&self) -> u8 {
        self.stat & 0b11
    }
    pub fn ppu_enable(&self) -> bool{
        (self.lcdc & 128) > 0
    }
    pub fn force_ly(&mut self, val: u8){
        self.scanline = val;
    }
    pub fn force_cycle(&mut self, val: usize){
        self.cycle = val;
    }
    pub fn clock(&mut self, color_mode: bool){
        if !self.ppu_enable(){
            self.scanline = 0;
            self.stat &= !(0b111);
            self.secondary_oam.clear();
            return;
        }
        let prev_mode = self.stat & 0b11;
        if !color_mode {self.next_dot()} else {self.next_dot_color()};

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
                else if self.cycle < 240 + self.mode_3_penalty {
                    self.stat |= 3;
                }
                else {self.stat &= !0b11}
            }
        }

        let mode_changed = prev_mode != (self.stat & 0b11);
        if mode_changed && self.get_mode()==0{
            self.first_hblank_cycle = true;
        }
        else {self.first_hblank_cycle = false;}

        //stat interrupt
        let mut cond = ((self.stat & 0b01000000) > 0) && (self.lyc == self.scanline) && (self.cycle == 0); // Nur einmal feuern
        
        if mode_changed {

            for m in 3..6{
                let mode = m - 3;
                if self.stat & (1 << m) > 0{
                    cond |= (self.stat & 0b11) == mode;
                }
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

        self.cycle += 1;
        if self.cycle >= 456{
            self.cycle = 0;
            self.scanline += 1;
            self.mode_3_penalty = 12;
            self.wx_condition = false;
        }
        if self.scanline >= 154{
            self.scanline = 0;
            self.new_scanline = true;
            self.wy_count = 0;
            self.num_frames = self.num_frames.wrapping_add(1);
            self.last_coincidence_lyc = 255;
            unsafe{
                match self.bus.upgrade(){
                    Some(b) => (*b.as_ptr()).has_frame = true,
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
    fn lcdc_0(&self) -> bool{
        self.lcdc & 1 > 0
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
    fn scan_sprites(&mut self, cgb_mode: bool){
        self.secondary_oam.clear();

        let mut oam_ptr = 0;

        while self.secondary_oam.len() < 10 && oam_ptr < 40 {
            let y_pos = self.oam[oam_ptr * 4];
            let x_pos = self.oam[oam_ptr * 4 + 1];
            let tile_index = self.oam[oam_ptr * 4 + 2];
            let attributes = self.oam[oam_ptr * 4 + 3];
            let y_pos_i = (y_pos as i32) - 16;
            let y_size = if self.mode_8_16() {16} else {8};
            let on_line = (self.scanline as i32) >= y_pos_i && (self.scanline as i32) < y_pos_i + y_size;

            if on_line{
                self.secondary_oam.push([y_pos, x_pos, tile_index, attributes]);
                self.mode_3_penalty += 6;
            }
            oam_ptr += 1;
        }
        // Hier eigentlich IF Mode DMG else nur reverse
        if !cgb_mode{
            self.secondary_oam.reverse();
            self.secondary_oam.sort_by(|a, b| a[1].cmp(&b[1]).reverse());
        }
        else {
            self.secondary_oam.reverse();
        }
    }
    fn next_dot(&mut self){

        if self.cycle == 0 && self.scanline < 144{
            if self.scanline == self.wy {
                self.wy_condition = true;
                self.wy_count = self.scanline; 
            }
        }
        if self.cycle == 79 && self.scanline < 144{
            self.scan_sprites(false);
        }

        if self.scanline < 144 && self.cycle == 240 && self.wx_condition{
            if self.window_enable() && self.wy_condition {self.wy_count += 1;}
        }

        if self.cycle >= 80 && self.cycle < 240 && self.scanline < 144 {
            if self.cycle == 80 {
                self.mode_3_penalty += (self.scx as usize) % 8;
            }

            if !self.wx_condition {
                self.wx_condition = ((self.cycle - 80) + 7) == (self.wx as usize);
                if self.wx_condition {self.mode_3_penalty += 6;}
            }

            // Background

            let mut bg_color: u8 = 0;

            if self.lcdc_0(){
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
            }

            // Window

            if self.window_enable() && self.lcdc_0(){
                let current_x = (self.cycle - 80) as i32;
                let wx = (self.wx as i32).saturating_sub(7);
                if self.wx_condition && self.wy_condition {
                    let px = (current_x).wrapping_sub(wx) as u8;
                    let py = self.wy_count.wrapping_sub(self.wy);
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

                for i in 0..self.secondary_oam.len(){
                // for &mut oam in &mut self.secondary_oam{

                    let oam = &self.secondary_oam[i];
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
                                    if y_flip {oam[2] & !1} else {oam[2] | 1}
                                } else {
                                    if y_flip {oam[2] | 1} else {oam[2] & !1}
                                }
                            } else {oam[2]};
                            if self.mode_8_16() && y_offset >= 8 {y_offset -= 8};
                            let ad_y = if y_flip {7 - y_offset} else {y_offset};
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
            self.wy_condition = false;
            unsafe{
                match self.bus.upgrade(){
                    Some(b) => (*b.as_ptr()).request_vblank(),
                    _ => panic!("Kein Bus in PPU vorhanden!"),
                }
            }
        }
    }
    fn get_color_cgb(&self, background: bool, palette_index: usize, color_index: u8) -> (u8,u8,u8){
        let arr = if background {&self.palette_ram} else {&self.palette_ram_obj};
        let palette_start = 8 * palette_index;
        let color_start = palette_start + ((color_index as usize) * 2);
        let low = arr[color_start];
        let high = arr[color_start+1];
        let rgb = (low as u16) | ((high as u16) << 8);
        let red = (rgb & 0b11111) as u8;
        let green = ((rgb >> 5) & 0b11111) as u8;
        let blue = ((rgb >> 10) & 0b11111) as u8;
        (red * 8, green * 8, blue * 8)
    }
    fn next_dot_color(&mut self){

        if self.cycle == 0 && self.scanline < 144{
            if self.scanline == self.wy {
                self.wy_condition = true;
                self.wy_count = self.scanline; 
            }
        }
        if self.cycle == 79 && self.scanline < 144{
            self.scan_sprites(true);
        }
        if self.scanline < 144 && self.cycle == 240 && self.wx_condition{
            if self.window_enable() && self.wy_condition {self.wy_count += 1;}
        }

        if self.cycle >= 80 && self.cycle < 240 && self.scanline < 144 {
            if self.cycle == 80 {
                self.mode_3_penalty += (self.scx as usize) % 8;
            }

            if !self.wx_condition {
                self.wx_condition = ((self.cycle - 80) + 7) == (self.wx as usize);
                if self.wx_condition {self.mode_3_penalty += 6;}
            }
            // Background

            let mut bg_color: (u8, u8, u8) = (255, 255, 255);
            let mut bg_index;
            let mut bg_attr_bit_7: bool;

            let px = self.scx.wrapping_add((self.cycle as u8).saturating_sub(80));
            let py = self.scy.wrapping_add(self.scanline);
            let tile_x = px / 8;
            let tile_y = py / 8;
            let x_offset = px % 8;
            let y_offset = py % 8;
                                    // start            select                         größe    x index            y index * zeile
            let tile_index =   self.vram[0][0x1800 + ((self.bg_tile_map_area() as usize) * 0x400) + tile_x as usize + (tile_y as usize * 32)];
            let tile_attribs = self.vram[1][0x1800 + ((self.bg_tile_map_area() as usize) * 0x400) + tile_x as usize + (tile_y as usize * 32)];
            let y_flip = (tile_attribs & 64) > 0;
            let x_flip = (tile_attribs & 32) > 0;
            let tile_bank = ((tile_attribs & 8) > 0) as usize;
            let palette = (tile_attribs & 0b111) as usize;
            bg_attr_bit_7 = (tile_attribs & 128) > 0;
            let mut tile_start = if !self.tile_address_mode() && tile_index < 128{   
                0x1000 + (tile_index as u16 * 16)
            }
            else { (tile_index as u16 * 16) };
            tile_start += if y_flip {2 * (7-y_offset as u16)} else {2 * (y_offset as u16)};
            let tile_lower =  self.vram[tile_bank][tile_start as usize];
            let tile_higher = self.vram[tile_bank][(tile_start + 1) as usize];
            let val = if x_flip {
                ((tile_lower >> (x_offset)) & 1) | (((tile_higher >> (x_offset)) & 1) << 1)
            } else{
                ((tile_lower >> (7 - x_offset)) & 1) | (((tile_higher >> (7 - x_offset)) & 1) << 1)
            };
            bg_color = self.get_color_cgb(true, palette, val);
            bg_index = val;

            // Window

            if self.window_enable(){
                let current_x = (self.cycle - 80) as i32;
                let wx = (self.wx as i32).saturating_sub(7);
                if self.wx_condition && self.wy_condition {
                    let px = (current_x).wrapping_sub(wx) as u8;
                    let py = self.wy_count.wrapping_sub(self.wy);
                    let tile_x = px / 8;
                    let tile_y = py / 8;
                    let x_offset = px % 8;
                    let y_offset = py % 8;
                                            // start            select                              größe    x index            y index * zeile
                    let tile_index =   self.vram[0][0x1800 + ((self.window_tile_map() as usize) * 0x400) + tile_x as usize + (tile_y as usize * 32)];
                    let tile_attribs = self.vram[1][0x1800 + ((self.window_tile_map() as usize) * 0x400) + tile_x as usize + (tile_y as usize * 32)];
                    let y_flip = (tile_attribs & 64) > 0;
                    let x_flip = (tile_attribs & 32) > 0;
                    let tile_bank = ((tile_attribs & 8) > 0) as usize;
                    let palette = (tile_attribs & 0b111) as usize;
                    bg_attr_bit_7 = (tile_attribs & 128) > 0;
                    let mut tile_start = if !self.tile_address_mode() && tile_index < 128{   
                        0x1000 + (tile_index as u16 * 16)
                    }
                    else { (tile_index as u16 * 16) };
                    tile_start += if y_flip {2 * (7-y_offset as u16)} else {2 * (y_offset as u16)};
                    let tile_lower =  self.vram[tile_bank][tile_start as usize];
                    let tile_higher = self.vram[tile_bank][(tile_start + 1) as usize];
                    let val = if x_flip {
                        ((tile_lower >> (x_offset)) & 1) | (((tile_higher >> (x_offset)) & 1) << 1)
                    } else{
                        ((tile_lower >> (7 - x_offset)) & 1) | (((tile_higher >> (7 - x_offset)) & 1) << 1)
                    };
                    bg_color = self.get_color_cgb(true, palette, val);
                    bg_index = val;
                }
            }

            // Sprites

            if self.obj_enable(){
                let pixel_x = self.cycle.saturating_sub(80) as u8;
                for i in 0..self.secondary_oam.len(){
                    let oam = &self.secondary_oam[i];

                    let y_pos_i = (oam[0] as i32) - 16;
                    let x_pos = (oam[1] as i32) - 8;
                    let mut  y_offset = (self.scanline as i32) - y_pos_i;
                    let x_offset = (pixel_x as i32) - x_pos;
                    let priority_bit = oam[3] & 128 > 0;
                    let y_flip = oam[3] & 64 > 0;
                    let x_flip = oam[3] & 32 > 0;
                    let sprite_bank = ((oam[3] & 8) > 0) as usize;
                    let fg_palette = oam[3] & 0b111;

                    let priority = if bg_index==0 {true} else {
                        if (self.lcdc & 1) == 0 {true}
                        else{
                            if !bg_attr_bit_7 && !priority_bit {true}
                            else {false}
                        }
                    };

                    // Pixel wird gezeichnet für diesen Sprite
                    if priority{
                        if x_pos <= pixel_x as i32 && x_pos + 8 > pixel_x as i32{
                            let tile_index = if self.mode_8_16() {
                                if y_offset >= 8 {
                                    if y_flip {oam[2] & !1} else {oam[2] | 1}
                                } else {
                                    if y_flip {oam[2] | 1} else {oam[2] & !1}
                                }
                            } else {oam[2]};
                            if self.mode_8_16() && y_offset >= 8 {y_offset -= 8};
                            let ad_y = if y_flip {7 - y_offset} else {y_offset};
                            let tile_low = self.vram[sprite_bank][((tile_index as usize) * 16) + (ad_y as usize) * 2];
                            let tile_high = self.vram[sprite_bank][((tile_index as usize) * 16) + (ad_y as usize) * 2 + 1];
                            let fg_color;
                            if x_flip{
                                fg_color = ((tile_low >> (x_offset)) & 1) | (((tile_high >> (x_offset)) & 1) << 1);
                            }
                            else{
                                fg_color = ((tile_low >> (7 - x_offset)) & 1) | (((tile_high >> (7 - x_offset)) & 1) << 1);
                            }
                            if fg_color > 0{
                                let real_fg_color = self.get_color_cgb(false, fg_palette as usize, fg_color);
                                bg_color = real_fg_color;
                            }
                        }
                    }
                }
            }

            self.set_pixel(self.cycle.saturating_sub(80), self.scanline as usize, bg_color.0, bg_color.1, bg_color.2);


        }

        if self.scanline == 144 && self.cycle == 0 {
            self.wy_condition = false;
            unsafe{
                match self.bus.upgrade(){
                    Some(b) => (*b.as_ptr()).request_vblank(),
                    _ => panic!("Kein Bus in PPU vorhanden!"),
                }
            }
        }
    }
    fn set_pixel(&self, x: usize, y: usize, r: u8, g: u8, b: u8){
        // match FRAMEBUFFER.lock().as_mut(){
        //     Ok(m) => {
                FRAMEBUFFER[index_framebuffer(x, y)].store(r, std::sync::atomic::Ordering::Relaxed);
                FRAMEBUFFER[index_framebuffer(x, y) + 1].store(g, std::sync::atomic::Ordering::Relaxed);
                FRAMEBUFFER[index_framebuffer(x, y) + 2].store(b, std::sync::atomic::Ordering::Relaxed);
                FRAMEBUFFER[index_framebuffer(x, y) + 3].store(255, std::sync::atomic::Ordering::Relaxed);
        //     },
        //     Err(e) => panic!("Konnte Framebuffer nicht sperren!"),
        // }
    }
    fn set_pixel_palette(&self, x: usize, y: usize, val: u8){
        let col = 255 - ((val as f32 / 3.0) * 255.0) as u8;
        self.set_pixel(x, y, col, col, col);
    }
}
