use core::panic;
use std::{cell::RefCell, rc::{Weak}};

use crate::gbc::bus::Bus;

mod sm83_instructions;
mod rom_tests;
mod register_tests;
mod json_parser;

#[derive(Default, Clone, Debug)]
pub struct Operand{
    name: OperandType,
    immediate: bool,
    bytes: Option<i32>,
}

#[derive(Default, Clone, PartialEq, Debug, Copy)]
#[repr(usize)]
pub enum Register8{
    #[default]
    A = 0,
    F = 1,
    B = 2,
    C = 3,
    D = 4,
    E = 5,
    H = 6,
    L = 7,
}

#[derive(Default, Clone, PartialEq, Debug, Copy)]
#[repr(usize)]
pub enum Register16{
    #[default]
    AF = 0, // Zusammengesetzte
    BC = 2,
    DE = 4,
    HL = 6,

    SP, // Einzigartige
    PC,
}

#[derive(Default, Clone, PartialEq, Debug)]
pub enum Condition{
    #[default]
    Z, // Ausführen, falls Z gesetzt
    NZ,// Ausführen, falls Z nicht gesetzt
    C, // Ausführen, falls C gesetzt
    NC,// Ausführen, falls C nicht gesetzt
}

#[derive(Default, Clone, PartialEq, Debug)]
pub enum OperandType{
    #[default]
    N8, // Immediate 8-bit Wert
    N16, // Immediate 16-bit Wert
    A8(bool), // Offset $FF00 + n Adresse
    A16(bool), // 16-bit Adresse Little Endian
    E8, // signed 8-bit int
    U3(u8), // Impliziter 3-bit Index
    ImplicitLiteral(u8), // Nur RST (RST Vektor)
    Reg8(Register8, bool),
    Reg16(Register16, bool, i32), //, immediate, increment
    ConditionOp(Condition),
    Dummy, // Kein Operand
}

#[derive(Clone, Debug)]
pub struct Opcode{
    pub mnemonic: (InstructionFn, String),
    pub bytes: i32,
    pub cycles: Vec<i32>,
    pub operands: Vec<Operand>,
}

impl Default for Opcode {
    fn default() -> Self { Opcode { mnemonic: (SM83::prefix, "CB-Prefixed".into()), bytes: 0, cycles: Vec::new(), operands: Vec::new() } }
}

type InstructionFn = fn(&mut SM83, &OperandType, &OperandType) -> bool;

#[derive(Default, Clone, PartialEq)]
pub enum CPUMode{
    #[default]
    Running,
    Halted,
    HaltBug,
    Stopped,
}

pub struct SM83 {
    regs: [u8; 8], // a,f,b,c,d,e,h,l
    reg_sp: u16, // Stack Pointer
    pub (crate) reg_pc: u16, // Program Counter

    bus: Weak<RefCell<Bus>>,

    opcodes_unprefixed: [Opcode; 256],
    opcodes_prefixed: [Opcode; 256],

    pub (crate) mode: CPUMode,
    ime: bool, // IME, erlaubt / verbietet Interrupt Handling
    set_ime: i32, // EI hat Verzögerung beim Setzen von ime
    pub (crate) remaining_cycles: i32,
    pub (crate) total_cycles: usize,
    total_operations: i64,

    pub (crate) speed_switch_armed: bool,
    pub (crate) dual_speed_mode: bool,

    pub (crate) ie_reg: u8, // 0xFFFF
    pub (crate) if_reg: u8, // 0xFF0F

    //debug
    pub (crate) remaining_steps: i32,
}

enum ProcessorFlags{
    ZeroFlag, // z
    SubtractionFlag, // n
    HalfCarryFlag, // h
    CarryFlag, // c
}



impl SM83 {
    fn read(&self, addr: u16) -> u8{
        match &self.bus.upgrade(){
            None => 0,
            Some(b) => unsafe {(*b.as_ptr()).read_memory(addr)},
        }
    }
    fn read_16(&self, addr: u16) -> u16{
        match &self.bus.upgrade(){
            None => 0,
            Some(b) => unsafe {(*b.as_ptr()).read_memory_16(addr)},
        }
    }
    fn write(&self, addr: u16, val: u8){
        match &self.bus.upgrade(){
            None => (),
            Some(b) => unsafe {(*b.as_ptr()).write_memory(addr, val)},
        };
    }
    fn push_stack_16(&mut self, value: u16){
        self.reg_sp -= 1;
        let high = (value >> 8) as u8;
        let low =(value & 0x00FF) as u8;
        self.write(self.reg_sp, high);
        self.reg_sp -= 1;
        self.write(self.reg_sp, low);
    }
    fn pop_stack_16(&mut self) -> u16{
        let low = self.read(self.reg_sp);
        self.reg_sp += 1;
        let high = self.read(self.reg_sp);
        self.reg_sp += 1;
        (low as u16) | ((high as u16) << 8)
    }
    fn poll_interrupts(&mut self) -> bool {
        if !self.ime { return false; }

        // Priorität von Niederen Bits höher als höhere Bits
        for bit in 0..=4{
            let enable: bool = (self.ie_reg & (1 << bit)) > 0;
            let flag: bool = (self.if_reg & (1 << bit)) > 0;
            if enable && flag {
                self.remaining_cycles += 5 * 4; // 20 t-Zyklen, bzw. 5 m-Zyklen
                self.total_cycles += 5 * 4; // 20 t-Zyklen, bzw. 5 m-Zyklen
                let vector = 0x40 + 0x8 * bit;
                self.push_stack_16(self.reg_pc);
                self.set_16(Register16::PC, vector);
                self.ime = false;
                self.if_reg = self.if_reg & !(1 << bit); // Interrupt bestätigen
                return true
            }
        }
        false
    }
    pub fn clock(&mut self) -> bool{
        let mut b = false;
        if self.remaining_cycles > 0 {
            self.remaining_cycles -= 1;
            b = false;
        }
        if self.remaining_cycles == 0{
            match self.mode {
                CPUMode::Running => {
                    if self.remaining_steps > 0{
                        self.remaining_steps -= 1;
                    }
                    if !self.poll_interrupts() {
                        // println!("Zyklus {}, PC: {:#x}", self.total_cycles, self.reg_pc);
                        self.run_next_instruction();
                        b = true;
                    }
                },
                CPUMode::Halted => {
                    if self.ime {
                        // Checken und aufrufen von Interrupt Handler...
                        if self.poll_interrupts(){
                            self.mode = CPUMode::Running;
                        }
                    }
                    else{
                        // Hier eigentlich Hardware Bug in HALT (nicht hier), weiß nicht ob wichtig zu emulieren
                        // Kein Interrupt Handler aufgerufen
                        if (self.if_reg & self.ie_reg) > 0 { self.mode = CPUMode::Running; }
                    }
                    b = false;
                },
                CPUMode::HaltBug => {
                    println!("Halt bug ausgelöst");
                    self.mode = CPUMode::Halted;
                    b =false;
                },
                CPUMode::Stopped =>{
                    // Speed Switch
                    if self.speed_switch_armed{
                        self.speed_switch_armed = false;
                        self.remaining_cycles += if self.dual_speed_mode {8200} else {16400}; // oder 8200?
                        self.total_cycles += if self.dual_speed_mode {8200} else {16400}; // oder 8200?
                        self.dual_speed_mode = !self.dual_speed_mode;
                        self.mode = CPUMode::Running;
                    }
                    if self.if_reg & 0b00010000 > 0 { self.mode = CPUMode::Running; } // Bei Joypad Interrupt
                    b =false
                }
            }
        }
        b
    }
    fn print_operands(&mut self, ops: &Vec<Operand>) -> String{
        let mut str = "".to_owned();
        let mut pc_offset = 1;
        for op in ops{
            str.push_str(" ");
            let valstr;
            match op.bytes{
                None => valstr = format!("{:?}", op.name),
                Some(0) => valstr = format!("{:?}", op.name),
                Some(n) => {
                    valstr = if n==1 {
                        format!("{:#04x}", self.read(self.reg_pc + pc_offset))
                    } else {
                        format!("{:#06x}", self.read_16(self.reg_pc + pc_offset))
                    };
                    pc_offset += n as u16;
                }
            }
            str.push_str(&valstr);
            str.push_str(",");
        }
        str.trim_end_matches(",").to_owned()
    }

    pub fn get_instruction_print_at(&mut self, addr: u16) -> String{
        let mut opcode = self.read(addr);
        let mut instruction = self.opcodes_unprefixed[opcode as usize].clone();
        if opcode == 0xCB {
            opcode = self.read(addr + 1);
            instruction = self.opcodes_prefixed[opcode as usize].clone();
        }
        let oplen = instruction.operands.len();
        let (op1, op2) = match oplen {
            0 => (OperandType::Dummy, OperandType::Dummy),
            1 => (instruction.operands[0].name.clone(), OperandType::Dummy),
            2 => (instruction.operands[0].name.clone(), instruction.operands[1].name.clone()),
            3 => (instruction.operands[1].name.clone(), instruction.operands[2].name.clone()), // NUR 0xF8 (LD  HL, SP+e8)
            _ => panic!("Mehr als drei Operanden!"),
        };
        let m = instruction.mnemonic.1;
        let operands = self.print_operands(&instruction.operands);
        [m, operands].concat()
    }
    pub fn run_next_instruction(&mut self){
        let mut opcode = self.read(self.reg_pc);
        let mut instruction = self.opcodes_unprefixed[opcode as usize].clone();
        if opcode == 0xCB {
            opcode = self.read(self.reg_pc + 1);
            instruction = self.opcodes_prefixed[opcode as usize].clone();
        }
        let oplen = instruction.operands.len();
        let (op1, op2) = match oplen {
            0 => (&OperandType::Dummy, &OperandType::Dummy),
            1 => (&instruction.operands[0].name, &OperandType::Dummy),
            2 => (&instruction.operands[0].name, &instruction.operands[1].name),
            3 => (&instruction.operands[1].name, &instruction.operands[2].name), // NUR 0xF8 (LD  HL, SP+e8)
            _ => panic!("Mehr als drei Operanden!"),
        };
        // Instruktion durchführen
        let jump = (instruction.mnemonic.0)(self, op1, op2);
        if !jump {self.reg_pc += instruction.bytes as u16;}
        // Vergangene Zyklen bestimmen
        if instruction.cycles.len() > 1 {
            if jump { self.remaining_cycles += instruction.cycles[0];
            self.total_cycles += instruction.cycles[0] as usize; }
            else { self.remaining_cycles += instruction.cycles[1];
            self.total_cycles += instruction.cycles[1] as usize; }
        }
        else { self.remaining_cycles += instruction.cycles[0];
        self.total_cycles += instruction.cycles[0] as usize; }

        self.total_operations += 1;

        // Verspätetes Setzen von ime nach der Instruktion
        if self.set_ime == 0 {
            self.set_ime = -1;
            self.ime = true;
        }
        else if self.set_ime > 0 {self.set_ime -= 1}
    }
    fn get_16(&self, reg: Register16) -> u16{
        match reg{
            Register16::PC => self.reg_pc,
            Register16::SP => self.reg_sp,
            _ => {
                // Das "hohe" Register steht zuerst, bzw im oberen Byte
                let high = self.regs[reg.clone() as usize];
                let low = self.regs[reg.clone() as usize + 1];
                low as u16 | ((high as u16) << 8)
            }
        }
    }
    fn set_16(&mut self, reg: Register16, val: u16) {
        match reg{
            Register16::PC => self.reg_pc = val,
            Register16::SP => self.reg_sp = val,
            _ => {
                let high = (val >> 8) as u8;
                let low = (val & 0xFF) as u8;
                self.regs[reg.clone() as usize] = high;
                self.regs[reg.clone() as usize + 1] = low;
            }
        }
    }
    fn get_8(&self, reg: Register8) -> u8{
        self.regs[reg as usize]
    }
    fn set_8(&mut self, reg: Register8, val: u8){
        self.regs[reg as usize] = val;
    }
    fn get_condition(&self, cond: Condition) -> bool{
        match cond{
            Condition::Z => self.get_status_flag(ProcessorFlags::ZeroFlag),
            Condition::NZ => !self.get_status_flag(ProcessorFlags::ZeroFlag),
            Condition::C => self.get_status_flag(ProcessorFlags::CarryFlag),
            Condition::NC => !self.get_status_flag(ProcessorFlags::CarryFlag),
        }
    }
    pub fn new() -> Self{
        let (unpref, pref) = SM83::fill_instructions();
        let cpu = SM83 { regs: [0; 8], reg_sp: 0, reg_pc: 0,
            opcodes_unprefixed: unpref.try_into().unwrap(), opcodes_prefixed: pref.try_into().unwrap(), bus: Weak::new(), mode: CPUMode::Running,
            ime: false, set_ime: -1, remaining_cycles: 0, total_cycles: 0, total_operations: 1, ie_reg: 0, if_reg: 0, remaining_steps: 0,
            speed_switch_armed: false, dual_speed_mode: false
        };
        cpu
    }
    pub fn set_initial_state_dmg(&mut self){
        self.set_8(Register8::A, 0x01);
        self.set_8(Register8::F, 0xB0);
        self.set_8(Register8::B, 0x00);
        self.set_8(Register8::C, 0x13);
        self.set_8(Register8::D, 0x00);
        self.set_8(Register8::E, 0xD8);
        self.set_8(Register8::H, 0x01);
        self.set_8(Register8::L, 0x4D);
        self.set_16(Register16::PC, 0x0100);
        self.set_16(Register16::SP, 0xFFFE);
    }
    pub fn set_initial_state_cgb(&mut self){
        self.set_8(Register8::A, 0x11);
        self.set_8(Register8::F, 0x80);
        self.set_8(Register8::B, 0x00);
        self.set_8(Register8::C, 0x00);
        self.set_8(Register8::D, 0xFF);
        self.set_8(Register8::E, 0x56);
        self.set_8(Register8::H, 0x00);
        self.set_8(Register8::L, 0x0D);
        self.set_16(Register16::PC, 0x0100);
        self.set_16(Register16::SP, 0xFFFE);
    }
    pub fn new_init(b: Weak<RefCell<Bus>>) -> Self {
        let mut this = SM83::new();
        this.bus = b;
        this
    }
    fn set_status_flag(&mut self, f : ProcessorFlags, value : bool){
        let mut reg_f = self.regs[1];
        reg_f = match f{
            ProcessorFlags::ZeroFlag        => (reg_f & 0b01111111) | (value as u8) << 7,
            ProcessorFlags::SubtractionFlag => (reg_f & 0b10111111) | (value as u8) << 6,
            ProcessorFlags::HalfCarryFlag   => (reg_f & 0b11011111) | (value as u8) << 5,
            ProcessorFlags::CarryFlag       => (reg_f & 0b11101111) | (value as u8) << 4,
        };
        self.regs[1] = reg_f
    }
    fn get_status_flag(&self, f : ProcessorFlags) -> bool{
        let result = match f{
            ProcessorFlags::ZeroFlag        => self.regs[1] & 0b10000000,
            ProcessorFlags::SubtractionFlag => self.regs[1] & 0b01000000,
            ProcessorFlags::HalfCarryFlag   => self.regs[1] & 0b00100000,
            ProcessorFlags::CarryFlag       => self.regs[1] & 0b00010000,
        };
        return result != 0;
    }
}
