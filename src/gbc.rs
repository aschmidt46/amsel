pub mod sm83;
pub mod ppu;
pub mod bus;
pub mod cartridge;
pub mod apu;

pub(crate) mod gbc{
    use std::{cell::RefCell, rc::Rc, sync::atomic::AtomicU8};
    use crate::gbc::{bus::Bus};

    const CGB_CLOCK: f64 = 4194304.0;
    const SAMPLE_RATE: f64 = 20000.0;
    
    const AUDIO_TIME_PER_CLOCK: f64 = 1.0 / CGB_CLOCK;
    const AUDIO_TIME_PER_SAMPLE: f64 = 1.0 / SAMPLE_RATE;
    
    pub struct CGB{
        bus: Rc<RefCell<Bus>>,
        has_frame: bool,
    
        audio_time: f64,
        audio_sample_ready: bool,
        audio_sample_left: f64,
        audio_sample_right: f64,
    
        sample_rate: f64,
        audio_time_per_clock: f64,
        audio_time_per_sample: f64,
    }
    
    impl CGB{
        pub fn new(path: &str) -> Self{
            let bus: Rc<RefCell<Bus>> = Rc::new(RefCell::new(Bus::new_init(path)));
            bus.borrow_mut().create_components(bus.clone());
            CGB { bus, has_frame: false, audio_time: 0.0, audio_sample_ready: false, audio_sample_left: 0.0, audio_sample_right: 0.0
                , sample_rate: 20000.0, audio_time_per_clock: 1.0 / CGB_CLOCK, audio_time_per_sample: 1.0 / 20000.0 }
        }
    
        pub fn has_frame(&mut self) -> bool{
            let tmp = self.has_frame;
            if tmp{
                self.has_frame = false;
            }
            tmp
        }
        pub fn press_button(&mut self, b: usize){
            self.bus.borrow_mut().press_button(b);
        }
        pub fn release_button(&mut self, b: usize){
            self.bus.borrow_mut().release_button(b);
        }
        pub fn press_joypad(&mut self, b: usize){
            self.bus.borrow_mut().press_joypad(b);
        }
        pub fn release_joypad(&mut self, b: usize){
            self.bus.borrow_mut().release_joypad(b);
        }
    
        pub fn access_framebuffer(&self) -> *const AtomicU8{
            (self.bus.borrow()).ppu.as_ref().unwrap().framebuffer.as_ptr()
        }
        pub fn access_float_framebuffer(&self) -> *const f32{
            (self.bus.borrow()).ppu.as_ref().unwrap().float_framebuffer.as_ptr()
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

        pub fn clock_until_sample_ready(&mut self){
            while !self.audio_sample_ready(){
                self.clock();
            }
        }
    }
    
    unsafe impl Send for CGB {}
}

