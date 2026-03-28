use std::{cell::RefCell, rc::Weak};

use crate::gbc::bus::Bus;



pub struct APU{
    bus: Weak<RefCell<Bus>>,
    pulse1: PulseChannel,
    pulse2: PulseChannel,
    wave: WaveChannel,
    noise: NoiseChannel,

    //Register
    master_control: u8,
    panning: u8,
    master_volume_panning: u8,

    //Interface
    pulse1_sample: u8,
    pulse2_sample: u8,
    wave_sample: u8,
    noise_sample: u8,

    //Internal
    total_clocks: usize,
    div_apu: usize,
}

impl APU {
    pub fn new(bus: Weak<RefCell<Bus>>) -> Self{
        APU { bus, pulse1: PulseChannel::new(true), pulse2: PulseChannel::new(false), noise: NoiseChannel::new(), wave: WaveChannel::new(),
             master_control: 0, panning: 0, master_volume_panning: 0, pulse1_sample: 0, pulse2_sample: 0, wave_sample: 0, noise_sample: 0, total_clocks: 0, div_apu: 0 }
    }
    pub fn clock_div_apu(&mut self){
        self.div_apu = self.div_apu.wrapping_add(1);
        if self.div_apu % 8 == 0{
            //envelope sweep
            self.pulse1.clock_envelope();
            self.pulse2.clock_envelope();
            self.noise.clock_envelope();
        }
        if self.div_apu % 2 == 0{
            //sound length
            self.pulse1.increment_length();
            self.pulse2.increment_length();
            self.wave.increment_length();
            self.noise.increment_length();
        }
        if self.div_apu % 4 == 0{
            //ch1 freq sweep
            self.pulse1.clock_sweep();
        }
    }
    fn channel_audible(&self, right: bool, num: usize) -> bool{
        let offset = if right {0} else {4};
        (self.panning & (1 << (num + offset))) > 0
    }
    pub fn clock(&mut self){
        self.total_clocks = self.total_clocks.wrapping_add(1);
        if self.total_clocks % 2 == 0{
            self.wave.on_divider_clock();
        }
        if self.total_clocks % 4 == 0{
            self.pulse1.on_divider_clock();
            self.pulse2.on_divider_clock();
        }
        if self.total_clocks % 16 == 0{ // ~262.000 Hz
            self.noise.on_divider_clock();
        }

        self.pulse1_sample = 0;
        self.pulse2_sample = 0;
        self.wave_sample = 0;
        self.noise_sample = 0;

        if self.pulse1.active(){
            self.pulse1_sample = self.pulse1.dac;
        }
        if self.pulse2.active(){
            self.pulse2_sample = self.pulse2.dac;
        }
        if self.wave.active(){
            self.wave_sample = self.wave.dac;
        }
        if self.noise.active(){
            self.noise_sample = self.noise.dac;
        }
    }
    pub fn get_sample_left(&self) -> f64{
        if (self.master_control & 128) == 0 /*|| (self.master_volume_panning & 128) == 0*/ { return 0.0; }
        let scaling = ((self.master_volume_panning & 0b01110000) >> 4) + 1;
        let mut current_sample_left: f64 = 0.0;
        if self.channel_audible(false, 0) {current_sample_left += self.pulse1_sample as f64 * (self.pulse1.volume as f64 / 15.0)};
        if self.channel_audible(false, 1) {current_sample_left += self.pulse2_sample as f64 * (self.pulse2.volume as f64 / 15.0)};
        if self.channel_audible(false, 2) {current_sample_left += self.wave_sample as f64 / 15.0};
        if self.channel_audible(false, 3) {current_sample_left += self.noise_sample as f64 * (self.noise.volume as f64 / 15.0)};
        current_sample_left * (scaling as f64 / 8.0)
    }
    pub fn get_sample_right(&self) -> f64{
        if (self.master_control & 128) == 0 /*|| (self.master_volume_panning & 8) == 0*/ { return 0.0; }
        let scaling = (self.master_volume_panning & 0b111) + 1;
        let mut current_sample_right: f64 = 0.0;
        if self.channel_audible(true, 0) {current_sample_right += self.pulse1_sample as f64 * (self.pulse1.volume as f64 / 15.0)};
        if self.channel_audible(true, 1) {current_sample_right += self.pulse2_sample as f64 * (self.pulse2.volume as f64 / 15.0)};
        if self.channel_audible(true, 2) {current_sample_right += self.wave_sample as f64 / 15.0};
        if self.channel_audible(true, 3) {current_sample_right += self.noise_sample as f64 * (self.noise.volume as f64 / 15.0)};
        current_sample_right * (scaling as f64 / 8.0)
    }
    pub fn maybe_write(&mut self, addr: u16, val: u8){
        if (self.master_control & 128) > 0{
            self.on_write(addr, val);
        }
        else{
            match addr{
                0xFF26 => {
                    self.master_control = val;
                    if (val & 128) == 0{
                        self.pulse1 = PulseChannel::new(true);
                        self.pulse2 = PulseChannel::new(false);
                        self.wave.reset();
                        self.noise = NoiseChannel::new();
                        self.master_control= 0;
                        self.panning = 0;
                        self.master_volume_panning = 0;
                        self.pulse1_sample = 0;
                        self.pulse2_sample = 0;
                        self.wave_sample = 0;
                        self.noise_sample = 0;
                    }
                },
                0xFF30..0xFF40 => self.wave.wave_ram[addr as usize - 0xFF30] = val,
                _ => (),
            }
        }
    }
    pub fn on_write(&mut self, addr: u16, val: u8){
        match addr{
            0xFF26 => {
                self.master_control = val;
                if (val & 128) == 0{
                    self.pulse1 = PulseChannel::new(true);
                    self.pulse2 = PulseChannel::new(false);
                    self.wave.reset();
                    self.noise = NoiseChannel::new();
                    self.master_control= 0;
                    self.panning = 0;
                    self.master_volume_panning = 0;
                    self.pulse1_sample = 0;
                    self.pulse2_sample = 0;
                    self.wave_sample = 0;
                    self.noise_sample = 0;
                }
            },
            0xFF25 => self.panning = val,
            0xFF24 => self.master_volume_panning = val,
            0xFF10 => self.pulse1.sweep = val,
            0xFF11 => {self.pulse1.length_duty = val; self.pulse1.length_timer = self.pulse1.length_timer_base()},
            0xFF12 => self.pulse1.vol_env = val,
            0xFF13 => self.pulse1.on_write_period_low(val),
            0xFF14 => self.pulse1.on_write_period_high_control(val),
            0xFF16 => {self.pulse2.length_duty = val; self.pulse2.length_timer = self.pulse2.length_timer_base()},
            0xFF17 => self.pulse2.vol_env = val,
            0xFF18 => self.pulse2.on_write_period_low(val),
            0xFF19 => self.pulse2.on_write_period_high_control(val),
            0xFF1A => self.wave.dac_enable = (val & 128) > 0,
            0xFF1B => {self.wave.initial_length_timer = val; self.wave.length_timer = 256 - self.wave.initial_length_timer as u16},
            0xFF1C => {self.wave.output_level = val & 0b01100000; self.wave.set_volume();},
            0xFF1D => self.wave.period_low = val,
            0xFF1E => self.wave.on_write_control(val),
            0xFF20 => {self.noise.length_timer_reg = val & 0b00111111; self.noise.length_timer = 64 - self.noise.length_timer_reg},
            0xFF21 => self.noise.vol_env = val,
            0xFF22 => self.noise.freq_rand = val,
            0xFF23 => self.noise.on_write_control(val),
            0xFF30..0xFF40 => self.wave.wave_ram[addr as usize - 0xFF30] = val,
            _ => ()
        }
    }
    pub fn on_read(&mut self, addr: u16) -> u8{
        match addr{
            0xFF26 => (self.master_control & 0b10000000) | ((self.pulse1.active() as u8) << 0) | ((self.pulse2.active() as u8) << 1) | ((self.wave.active() as u8) << 2) | ((self.noise.active() as u8) << 3),
            0xFF25 => self.panning,
            0xFF24 => self.master_volume_panning,
            0xFF10 => self.pulse1.sweep,
            0xFF11 => self.pulse1.length_duty & 0b11000000,
            0xFF12 => self.pulse1.vol_env,
            0xFF13 => 0xFF,
            0xFF14 => self.pulse1.period_high_control & 0b01000000,
            0xFF16 => self.pulse2.length_duty & 0b11000000,
            0xFF17 => self.pulse2.vol_env,
            0xFF18 => 0xFF,
            0xFF19 => self.pulse2.period_high_control & 0b01000000,
            0xFF1A => (self.wave.dac_enable as u8) << 7,
            0xFF1B => 0xFF,
            0xFF1C => self.wave.output_level,
            0xFF1D => 0xFF,
            0xFF1E => self.wave.on_read_control(),
            0xFF20 => 0xFF,
            0xFF21 => self.noise.vol_env,
            0xFF22 => self.noise.freq_rand,
            0xFF23 => self.noise.on_read_control(),
            0xFF30..0xFF40 => self.wave.wave_ram[addr as usize - 0xFF30],
            _ => 0xFF
        }
    }
    
}

struct PulseChannel{

    pub dac: u8,

    //Register
    sweep: u8, // Nur Kanal 1
    length_duty: u8,
    vol_env: u8,
    period_low: u8,
    period_high_control: u8,
    has_sweep: bool,

    // Internal
    period_divider: u16,
    waveform_counter: usize,
    volume: u8,
    envelope_counter: usize,
    sweep_enabled: bool,
    sweep_timer: usize,
    shadow_register: u16,
    length_timer: u8,
    length_enabled: bool,
    disabled_by_sweep: bool,
    sweep_pace: u8,

}

const WAVEFORM_PULSE: [[u8; 8]; 4] = [
    [1,0,0,0,0,0,0,0], //12,5%
    [1,1,0,0,0,0,0,0], //25%
    [1,1,1,1,0,0,0,0], //50%
    [1,1,1,1,1,1,0,0], //75%
];

impl PulseChannel{
    fn new(has_sweep: bool) -> Self{
        PulseChannel { dac: 0, sweep: 0, length_duty: 0, vol_env: 0, period_low: 0, period_high_control: 0, has_sweep, period_divider: 0, waveform_counter: 0, volume: 0,
        sweep_enabled: false, sweep_timer: 0, length_timer: 0, disabled_by_sweep: false, length_enabled: false, envelope_counter: 0, sweep_pace: 0, shadow_register: 0 }
    }
    fn active(&self) -> bool{
        let l = if self.length_enabled {self.length_timer > 0} else {true};
        (self.vol_env & 0xF8) > 0 && !self.disabled_by_sweep && l
    }
    fn increment_length(&mut self){
        if self.length_timer > 0{
            self.length_timer -= 1;
        }
    }
    fn length_timer_base(&self) -> u8{
        64 - (self.length_duty & 0b00111111)
    }
    fn envelope_increase_volume(&self) -> bool{
        (self.vol_env & 0b00001000) > 0
    }
    fn envelope_sweep_pace(&self) -> u8{
        self.vol_env & 0b111
    }
    fn envelope_initial_volume(&self) -> u8{
        (self.vol_env & 0b11110000) >> 4
    }
    fn get_sweep_pace(&self) -> u8{
        (self.sweep & 0b01110000) >> 4
    }
    fn get_sweep_individual_step(&self) -> u8{
        self.sweep & 0b111
    }
    fn get_sweep_direction(&self) -> bool{
        (self.sweep & 0b00001000) > 0
    }
    fn clock_envelope(&mut self){
        self.envelope_counter = self.envelope_counter.wrapping_add(1);
        if self.envelope_sweep_pace() > 0{
            if self.envelope_counter % (self.envelope_sweep_pace() as usize) == 0{
                if self.envelope_increase_volume(){
                    if self.volume < 15 {
                        self.volume += 1;
                    }
                }
                else{
                    self.volume = self.volume.saturating_sub(1);
                }
            }
        }
    }
    fn clock_sweep(&mut self){
        if self.sweep_timer > 0 {self.sweep_timer -= 1;}

        if self.sweep_timer == 0{
            self.sweep_timer = if self.sweep_pace == 0 {8} else {self.sweep_pace as usize};

            if self.sweep_enabled && self.sweep_pace > 0{
                let new_freq = self.calc_freq();

                //Zurückschreiben
                if self.get_sweep_individual_step() > 0 && new_freq <= 2047 {
                    self.period_divider = new_freq;
                    self.shadow_register = new_freq;
                    self.period_low = (new_freq & 255) as u8;
                    self.period_high_control = (self.period_high_control & !0b111) | ((new_freq >> 8) as u8);
                }
                self.sweep_pace = self.get_sweep_pace();
                let _ = self.calc_freq();
            }
        }
    }
    fn calc_freq(&mut self) -> u16{
        //Frequenzberechnung
        let mut new_freq = (self.shadow_register) >> self.get_sweep_individual_step();
        new_freq = if self.get_sweep_direction()
                 {self.shadow_register.saturating_sub(new_freq)}
            else {self.shadow_register.saturating_add(new_freq)};
        // Overflow Check...
        //Kanal ausschalten, falls zu groß (11 bit limit)
        if new_freq > 0x7FF{
            self.disabled_by_sweep = true;
        }
        new_freq
    }
    fn on_write_period_high_control(&mut self, val: u8){
        self.period_high_control = val;
        self.disabled_by_sweep = false;
        self.length_enabled = (self.period_high_control & 64) > 0;
        // Trigger event
        if (self.period_high_control & 128) > 0 && (self.vol_env & 0xF8) > 0 {
            // Hier fehlen noch Sachen
            // if length timer expired it reset
            if self.length_timer == 0{
                self.length_timer = self.length_timer_base();
            }
            // envelope timer reset
            self.envelope_counter = 0;
            
            self.period_divider = self.period_low as u16 | ((self.period_high_control as u16 & 0b111) << 8);
            self.volume = self.envelope_initial_volume();
            // sweep sachen
            if self.has_sweep{
                self.shadow_register = self.period_divider;
                self.sweep_timer = if self.get_sweep_pace() > 0 {self.get_sweep_pace() as usize} else {8};
                self.sweep_pace = self.get_sweep_pace();
                if self.get_sweep_pace() > 0 || self.get_sweep_individual_step() > 0{
                    self.sweep_enabled = true;
                }
                else{
                    self.sweep_enabled = false;
                }
                if self.get_sweep_individual_step() > 0{
                    let _ = self.calc_freq();
                }
            }
        }
    }
    fn on_write_period_low(&mut self, val: u8){
        self.period_low = val;
    }
    fn duty(&self) -> usize{
        ((self.length_duty & 0b11000000) >> 6) as usize
    }
    fn on_divider_clock(&mut self){
        if self.period_divider >= 2047{
            //reset
            self.period_divider = self.period_low as u16 | ((self.period_high_control as u16 & 0b111) << 8);
            // Inkrement Welle
            self.waveform_counter = self.waveform_counter.wrapping_add(1);
            if self.waveform_counter >= 8 {self.waveform_counter -= 8};
            self.dac = WAVEFORM_PULSE[self.duty()][self.waveform_counter];
        }
        else{
            self.period_divider += 1;
        }
    }
}

struct WaveChannel{
    dac_enable: bool,

    //Register
    initial_length_timer: u8,
    output_level: u8,
    period_low: u8,
    period_high_control: u8,

    //Internal
    period_divider: u16,
    length_timer: u16,
    length_enable: bool,
    volume: u8,
    wave_ram_index: usize,
    wave_ram: [u8; 16],
    sample_buffer: u8,

    dac: u8,
}

impl WaveChannel{
    fn new() -> Self{
        WaveChannel { dac_enable: false, initial_length_timer: 0, output_level: 0, period_low: 0, period_high_control: 0, period_divider: 0, length_timer: 0, volume: 0, wave_ram_index: 0,
        length_enable: false, dac: 0, wave_ram: [0; 16], sample_buffer: 0 }
    }
    fn reset(&mut self){
        self.dac_enable= false;
        self.initial_length_timer= 0;
        self.output_level= 0;
        self.period_low= 0;
        self.period_high_control= 0;
        self.period_divider= 0;
        self.length_timer= 0;
        self.volume= 0;
        self.wave_ram_index= 0;
        self.length_enable= false;
        self.dac= 0;
    }
    fn active(&self) -> bool{
        let l = if self.length_enable {self.length_timer > 0} else {true};
        self.dac_enable && l
    }
    fn get_output_level(&self) -> u8{
        (self.output_level & 0b01100000) >> 5
    }
    fn get_output_shift(&self) -> usize{
        match self.volume{
            0b00 => 4,
            0b01 => 0,
            0b10 => 1,
            0b11 => 2,
            _ => 4,
        }
    }
    fn set_volume(&mut self){
        self.volume = self.get_output_level();
    }
    fn increment_length(&mut self){
        if self.length_timer > 0{
            self.length_timer -= 1;
        }
    }
    fn on_write_control(&mut self, val: u8){
        self.period_high_control = val;
        self.length_enable = (self.period_high_control & 64) > 0;
        if (val & 128) > 0 && self.dac_enable{
            //Trigger
            //enable channel
            if self.length_timer == 0{
                self.length_timer = 256 - self.initial_length_timer as u16;
            }
            self.period_divider = self.period_low as u16 | ((self.period_high_control as u16 & 0b111) << 8);
            self.set_volume();
            self.wave_ram_index = 0;
        }
    }
    fn on_read_control(&self) -> u8{
        self.period_high_control & 64
    }
    fn on_divider_clock(&mut self){
        if self.period_divider >= 2047{
            //reset
            self.period_divider = self.period_low as u16 | ((self.period_high_control as u16 & 0b111) << 8);
            // Sample lesen
            let index = self.wave_ram_index / 2;
            let nibble = self.wave_ram_index % 2;
            let sample = self.wave_ram[index];
            self.sample_buffer = if nibble > 0 { sample & 0b1111 } else { sample >> 4 };
            self.dac = self.sample_buffer >> self.get_output_shift();

            // Inkrement
            self.wave_ram_index += 1;
            if self.wave_ram_index >= 32{
                self.wave_ram_index -= 32;
            }
        }
        else{
            self.period_divider += 1;
        }
    }
}

struct NoiseChannel{
    //Register
    length_timer_reg: u8,
    vol_env: u8,
    freq_rand: u8,
    control: u8,

    //Internal
    volume: u8,
    envelope_counter: usize,
    length_enabled: bool,
    length_timer: u8,
    lfsr: u16,
    internal_divider: usize,

    dac: u8,
}

impl NoiseChannel{
    fn new() -> Self{
        NoiseChannel { length_timer_reg: 0, vol_env: 0, freq_rand: 0, control: 0, volume: 0, envelope_counter: 0, length_enabled: false, length_timer: 0, dac: 0, lfsr: 0, internal_divider: 0 }
    }
    fn active(&self) -> bool{
        let l = if self.length_enabled {self.length_timer > 0} else {true};
        (self.vol_env & 0xF8) > 0 && l
    }
    fn envelope_increase_volume(&self) -> bool{
        (self.vol_env & 0b00001000) > 0
    }
    fn envelope_sweep_pace(&self) -> u8{
        self.vol_env & 0b111
    }
    fn envelope_initial_volume(&self) -> u8{
        (self.vol_env & 0b11110000) >> 4
    }
    fn get_clock_shift(&self) -> u8{
        (self.freq_rand & 0b11110000) >> 4
    }
    fn get_lfsr_width(&self) -> bool{
        (self.freq_rand & 0b00001000) > 0
    }
    fn get_clock_divider(&self) -> u8{
        self.freq_rand & 0b111
    }
    fn clock_envelope(&mut self){
        self.envelope_counter = self.envelope_counter.wrapping_add(1);
        if self.envelope_sweep_pace() > 0{
            if self.envelope_counter % (self.envelope_sweep_pace() as usize) == 0{
                if self.envelope_increase_volume(){
                    if self.volume < 15 {
                        self.volume += 1;
                    }
                }
                else{
                    self.volume = self.volume.saturating_sub(1);
                }
            }
        }
    }
    fn increment_length(&mut self){
        if self.length_timer > 0{
            self.length_timer -= 1;
        }
    }
    fn on_write_control(&mut self, val: u8){
        self.control = val;
        if (self.control & 128) > 0 && (self.vol_env & 0xF8) > 0{
            //Trigger
            //enable channel
            if self.length_timer == 0{
                self.length_timer = 64 - (self.length_timer_reg & 0b00111111);
            }
            self.envelope_counter = 0;
            self.volume = self.envelope_initial_volume();
            self.lfsr = 0;
        }
        self.length_enabled = (self.control & 64) > 0;
    }
    fn on_read_control(&self) -> u8{
        (self.length_enabled as u8) << 6
    }
    fn on_divider_clock(&mut self){
        self.internal_divider = self.internal_divider.wrapping_add(1);
        let divider = if self.get_clock_divider() > 0 {self.get_clock_divider() as f32} else {0.5};
        let division = (divider * (2.0 as f32).powf(self.get_clock_shift() as f32)).ceil() as usize;
        if self.internal_divider % division == 0{
            self.lfsr_clock();
        }
    }
    fn lfsr_clock(&mut self){
        let shifted_out = (self.lfsr & 1) == ((self.lfsr & 2) >> 1);
        self.lfsr = (self.lfsr & !0x8000) | ((shifted_out as u16) << 15);
        if self.get_lfsr_width(){
            self.lfsr = (self.lfsr & !128) | ((shifted_out as u16) << 7);
        }
        self.lfsr >>= 1;
        self.dac = shifted_out as u8;
    }
}
