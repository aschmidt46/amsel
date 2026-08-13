

use ffi::StereoTuple;
mod gbc;
use gbc::gbc::CGB;

use crate::ffi::CGBRegister8;
use crate::ffi::CGBRegister16;
use crate::ffi::InstructionPair;
use crate::gbc::sm83::Register8;
use crate::gbc::sm83::Register16;

#[cxx::bridge]
pub mod ffi{


    struct StereoTuple{
        left: f64,
        right: f64,
    }

    struct InstructionPair{
        first: String,
        second: Vec<i32>,
    }

    enum CGBRegister8{
        A,
        F,
        B,
        C,
        D,
        E,
        H,
        L,
    }

    enum CGBRegister16{
        AF,
        BC,
        DE,
        HL,
        
        SP,
        PC,
    }

    extern "Rust"{
        type CGB;

        fn new_cgb(path: &str) -> Box<CGB>;

        fn new_cgb_rom(rom: &CxxVector<u8>) -> Box<CGB>;

        fn has_frame(cgb: &mut Box<CGB>) -> bool;

        fn press_button(cgb: &mut Box<CGB>, b: usize);

        fn release_button(cgb: &mut Box<CGB>, b: usize);

        fn press_joypad(cgb: &mut Box<CGB>, b: usize);

        fn release_joypad(cgb: &mut Box<CGB>, b: usize);
    
        fn access_framebuffer(cgb: &Box<CGB>) -> *const u32;
    
        fn set_sample_rate(cgb: &mut Box<CGB>, sample_rate: f64);
    
        fn cpu_has_advanced(cgb: &mut Box<CGB>) -> bool;
        fn audio_sample_ready(cgb: &mut Box<CGB>) -> bool;
    
        fn get_stereo(cgb: &Box<CGB>) -> StereoTuple;
    
        fn cgb_clock(cgb: &mut Box<CGB>);
        fn cgb_clock_until_samle_ready(cgb: &mut Box<CGB>);
        fn cgb_set_break(cgb: &mut Box<CGB>, val: bool);
        fn cgb_is_halted(cgb: &mut Box<CGB>) -> bool;
        fn cgb_step(cgb: &mut Box<CGB>);
        fn cgb_read_cpu(cgb: &mut Box<CGB>, addr: u16) -> u8;
        fn get_next_n_instructions(cgb: &mut Box<CGB>, n: i32) -> InstructionPair;
        fn get_prev_10_instructions(cgb: &mut Box<CGB>) -> InstructionPair;
        fn add_breakpoint(cgb: &mut Box<CGB>, bp: u16) -> Vec<u16>;
        fn remove_breakpoint(cgb: &mut Box<CGB>, bp: u16) -> Vec<u16>;
        fn add_breakpoint_op(cgb: &mut Box<CGB>, bp: String) -> Vec<String>;
        fn remove_breakpoint_op(cgb: &mut Box<CGB>, bp: String) -> Vec<String>;
        fn get_mnemonic(cgb: &mut Box<CGB>, index: usize) -> String;
        fn cgb_read_register_8(cgb: &mut Box<CGB>, reg: CGBRegister8) -> u8;
        fn cgb_read_register_16(cgb: &mut Box<CGB>, reg: CGBRegister16) -> u16;

        fn cgb_can_save(cgb: &mut Box<CGB>) -> bool;
        // in bytes
        fn get_save_size(cgb: &mut Box<CGB>) -> usize;
        fn cgb_get_save_data(cgb: &mut Box<CGB>) -> Vec<u8>;
        fn cgb_load_save(cgb: &mut Box<CGB>, vec: &CxxVector<u8>);
    }
}

fn get_cgb_register8(r8: CGBRegister8) -> Register8{
    match r8{
        CGBRegister8::A => Register8::A,
        CGBRegister8::F => Register8::F,
        CGBRegister8::B => Register8::B,
        CGBRegister8::C => Register8::C,
        CGBRegister8::D => Register8::D,
        CGBRegister8::E => Register8::E,
        CGBRegister8::H => Register8::H,
        CGBRegister8::L => Register8::L,
        _ => Register8::A,
    }
}

fn get_cgb_register16(r16: CGBRegister16) -> Register16{
    match r16{
        CGBRegister16::AF => Register16::AF,
        CGBRegister16::BC => Register16::BC,
        CGBRegister16::DE => Register16::DE,
        CGBRegister16::HL => Register16::HL,
        CGBRegister16::SP => Register16::SP,
        CGBRegister16::PC => Register16::PC,
        _ => Register16::AF,
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
fn access_framebuffer(cgb: &Box<CGB>) -> *const u32{
    cgb.access_framebuffer()
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
fn cgb_clock(cgb: &mut Box<CGB>){
    cgb.clock();
}

fn cgb_clock_until_samle_ready(cgb: &mut Box<CGB>){
    cgb.clock_until_sample_ready();
}

fn cgb_set_break(cgb: &mut Box<CGB>, val: bool){
    cgb.set_break(val);
}

fn cgb_is_halted(cgb: &mut Box<CGB>) -> bool{
    cgb.is_halted()
}

fn cgb_step(cgb: &mut Box<CGB>){
    cgb.step();
}

fn cgb_read_cpu(cgb: &mut Box<CGB>, addr: u16) -> u8{
    cgb.read_cpu(addr)
}

fn get_next_n_instructions(cgb: &mut Box<CGB>, n: i32) -> InstructionPair{
    let (s, v) = cgb.get_next_n_instructions(n);
    InstructionPair { first: s, second: v }
}

fn get_prev_10_instructions(cgb: &mut Box<CGB>) -> InstructionPair{
    let (s, v) = cgb.get_prev_10_instructions();
    InstructionPair { first: s, second: v }
}

fn add_breakpoint(cgb: &mut Box<CGB>, bp: u16) -> Vec<u16>{
    cgb.add_breakpoint(bp)
}
fn remove_breakpoint(cgb: &mut Box<CGB>, bp: u16) -> Vec<u16>{
    cgb.remove_breakpoint(bp)
}
fn add_breakpoint_op(cgb: &mut Box<CGB>, bp: String) -> Vec<String>{
    cgb.add_breakpoint_op(bp)
}
fn remove_breakpoint_op(cgb: &mut Box<CGB>, bp: String) -> Vec<String>{
    cgb.remove_breakpoint_op(bp)
}
fn get_mnemonic(cgb: &mut Box<CGB>, index: usize) -> String{
    cgb.get_mnemonic(index)
}

fn cgb_read_register_8(cgb: &mut Box<CGB>, reg: CGBRegister8) -> u8{
    cgb.read_register_8(get_cgb_register8(reg))
}
fn cgb_read_register_16(cgb: &mut Box<CGB>, reg: CGBRegister16) -> u16{
    cgb.read_register_16(get_cgb_register16(reg))
}

fn cgb_can_save(cgb: &mut Box<CGB>) -> bool{
    cgb.game_can_save()
}
fn get_save_size(cgb: &mut Box<CGB>) -> usize{
    cgb.get_save_size()
}
fn cgb_get_save_data(cgb: &mut Box<CGB>) -> Vec<u8>{
    cgb.get_save_data()
}

fn cgb_load_save(cgb: &mut Box<CGB>, vec: &cxx::CxxVector<u8>){
    let mut v : Vec<u8> = Vec::new();
    for el in vec{
        v.push(el.clone());
    }
    cgb.load_save(v);
}

fn new_cgb_rom(rom: &cxx::CxxVector<u8>) -> Box<CGB>{
    let mut v : Vec<u8> = Vec::new();
    for el in rom{
        v.push(el.clone());
    }
    return Box::new(CGB::new_rom(&v))
}
