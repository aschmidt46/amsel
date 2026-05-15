

use ffi::StereoTuple;
mod gbc;
use gbc::gbc::CGB;

#[cxx::bridge]
pub mod ffi{

    struct StereoTuple{
        left: f64,
        right: f64,
    }

    extern "Rust"{
        type CGB;

        fn new_cgb(path: &str) -> Box<CGB>;

        fn has_frame(cgb: &mut Box<CGB>) -> bool;

        fn press_button(cgb: &mut Box<CGB>, b: usize);

        fn release_button(cgb: &mut Box<CGB>, b: usize);

        fn press_joypad(cgb: &mut Box<CGB>, b: usize);

        fn release_joypad(cgb: &mut Box<CGB>, b: usize);
    
        fn access_framebuffer(cgb: &Box<CGB>) -> *const u8;
    
        fn set_sample_rate(cgb: &mut Box<CGB>, sample_rate: f64);
    
        fn cpu_has_advanced(cgb: &mut Box<CGB>) -> bool;
        fn audio_sample_ready(cgb: &mut Box<CGB>) -> bool;
    
        fn get_stereo(cgb: &Box<CGB>) -> StereoTuple;
    
        fn clock(cgb: &mut Box<CGB>);
    }
}

fn new_cgb(path: &str) -> Box<CGB>{
    return Box::new(CGB::new(path))
}

fn has_frame(cgb: &mut Box<CGB>) -> bool{
    cgb.has_frame()
}

fn press_button(cgb: &mut Box<CGB>, b: usize){
    cgb.press_button(b);
}

fn release_button(cgb: &mut Box<CGB>, b: usize){
    cgb.release_button(b);
}
fn press_joypad(cgb: &mut Box<CGB>, b: usize){
    cgb.press_joypad(b);
}
fn release_joypad(cgb: &mut Box<CGB>, b: usize){
    cgb.release_joypad(b);
}
fn access_framebuffer(cgb: &Box<CGB>) -> *const u8{
    cgb.access_framebuffer().cast()
}
fn set_sample_rate(cgb: &mut Box<CGB>, sample_rate: f64){
    cgb.set_sample_rate(sample_rate);
}
fn cpu_has_advanced(cgb: &mut Box<CGB>) -> bool{
    cgb.cpu_has_advanced()
}
fn audio_sample_ready(cgb: &mut Box<CGB>) -> bool{
    cgb.audio_sample_ready()
}
fn get_stereo(cgb: &Box<CGB>) -> StereoTuple{
    let t = cgb.get_stereo();
    StereoTuple {left: t.0, right: t.1}
}
fn clock(cgb: &mut Box<CGB>){
    cgb.clock();
}