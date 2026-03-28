use std::{cell::RefCell, rc::Rc};

use crate::gbc::{bus::Bus};

pub mod sm83;
pub mod ppu;
pub mod bus;
pub mod cartridge;
pub mod apu;

const CGB_CLOCK: f64 = 4194304.0;
const SAMPLE_RATE: f64 = 20000.0;

const AUDIO_TIME_PER_CLOCK: f64 = 1.0 / CGB_CLOCK;
const AUDIO_TIME_PER_SAMPLE: f64 = 1.0 / SAMPLE_RATE;

pub struct CGB{
    pub bus: Rc<RefCell<Bus>>,
    pub has_frame: bool,

    audio_time: f64,
    audio_sample_ready: bool,
    pub audio_sample_left: f64,
    pub audio_sample_right: f64,

    sample_rate: f64,
    audio_time_per_clock: f64,
    audio_time_per_sample: f64,
}

impl CGB{
    pub fn new(path: &str) -> Self{
        let bus: Rc<RefCell<Bus>> = Rc::new(RefCell::new(Bus::new_init(path)));
        bus.borrow_mut().create_components(bus.clone());
        CGB { bus, has_frame: false, audio_time: 0.0, audio_sample_ready: false, audio_sample_left: 0.0, audio_sample_right: 0.0, sample_rate: 20000.0, audio_time_per_clock: 0.0, audio_time_per_sample: 0.0 }
    }

    pub fn set_sample_rate(&mut self, sample_rate: f64){
        self.sample_rate = sample_rate;
        self.audio_time_per_clock = 1.0 / CGB_CLOCK;
        self.audio_time_per_sample = 1.0 / self.sample_rate;
    }

    pub fn cpu_has_advanced(&mut self) -> bool{
        self.bus.borrow_mut().cpu_has_advanced()
    }
    pub fn audio_sample_ready(&mut self) -> bool{
        if self.audio_sample_ready{
            self.audio_sample_ready = false;
            return true;
        }
        false
    }

    pub fn get_stereo(&self) -> (f64, f64){
        (self.audio_sample_left, self.audio_sample_right)
    }

    pub fn clock(&mut self){
        self.bus.borrow_mut().clock();
        if self.bus.borrow().has_frame{
            self.has_frame = true;
            self.bus.borrow_mut().has_frame = false;
        }
        self.audio_time += self.audio_time_per_clock;
        if self.audio_time >= self.audio_time_per_sample{
            self.audio_time -= self.audio_time_per_sample;
            self.audio_sample_left = self.bus.borrow().get_audio_sample_left();
            self.audio_sample_right = self.bus.borrow().get_audio_sample_right();
            self.audio_sample_ready = true;
        }
    }
}

unsafe impl Send for CGB {}
