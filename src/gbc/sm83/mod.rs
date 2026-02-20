use core::panic;
use std::{cell::RefCell, rc::Rc};

use serde_json::Value;

use crate::gbc::bus::Bus;

mod sm83_instructions;
mod rom_tests;

#[derive(Default, Clone)]
struct Operand{
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

#[derive(Clone)]
struct Opcode{
    mnemonic: InstructionFn,
    bytes: i32,
    cycles: Vec<i32>,
    operands: Vec<Operand>,
}

impl Default for Opcode {
    fn default() -> Self { Opcode { mnemonic: SM83::prefix, bytes: 0, cycles: Vec::new(), operands: Vec::new() } }
}

type InstructionFn = fn(&mut SM83, OperandType, OperandType) -> bool;

#[derive(Default, Clone, PartialEq)]
pub enum CPUMode{
    #[default]
    Running,
    Halted,
    Stopped,
}

pub struct SM83 {
    regs: [u8; 8], // a,f,b,c,d,e,h,l
    reg_sp: u16, // Stack Pointer
    reg_pc: u16, // Program Counter

    bus: Option<Rc<RefCell<Bus>>>,

    opcodes_unprefixed: Vec<Opcode>,
    opcodes_prefixed: Vec<Opcode>,

    mode: CPUMode,
    ime: bool, // IME, erlaubt / verbietet Interrupt Handling
    set_ime: i32, // EI hat Verzögerung beim Setzen von ime
    remaining_cycles: i32,
    total_cycles: i64,
    total_operations: i64,

    // Interrupts, ACHTUNG, betrifft HALT Instruktion
    ie_itr: bool,
    if_itr: bool,
}

enum ProcessorFlags{
    ZeroFlag, // z
    SubtractionFlag, // n
    HalfCarryFlag, // h
    CarryFlag, // c
}

static JSON_OPS: &str = include_str!("../../../resources/Opcodes.json");



impl SM83 {
    fn read(&self, addr: u16) -> u8{
        match &self.bus{
            None => 0,
            Some(b) => b.borrow_mut().read_memory(addr),
        }
    }
    fn read_16(&self, addr: u16) -> u16{
        match &self.bus{
            None => 0,
            Some(b) => b.borrow_mut().read_memory_16(addr),
        }
    }
    fn write(&self, addr: u16, val: u8){
        match &self.bus{
            None => (),
            Some(b) => b.borrow_mut().write_memory(addr, val),
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
    pub fn clock(&mut self){
        if self.remaining_cycles > 0 {
            self.remaining_cycles -= 1;
        }
        else{
            match self.mode {
                CPUMode::Running => {
                    self.run_next_instruction();
                },
                CPUMode::Halted => {
                    if self.ime {
                        // Checken und aufrufen von Interrupt Handler...
                        // dann...
                        if self.ie_itr || self.if_itr { self.mode = CPUMode::Running; }
                    }
                    else{
                        // Hier eigentlich Hardware Bug in HALT (nicht hier), weiß nicht ob wichtig zu emulieren
                        // Kein Interrupt Handler aufgerufen
                        if self.ie_itr || self.if_itr { self.mode = CPUMode::Running; }
                    }
                },
                CPUMode::Stopped =>{
                    // Nichts
                }
            }
        }
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
            0 => (OperandType::Dummy, OperandType::Dummy),
            1 => (instruction.operands[0].name.clone(), OperandType::Dummy),
            2 => (instruction.operands[0].name.clone(), instruction.operands[1].name.clone()),
            3 => (instruction.operands[1].name.clone(), instruction.operands[2].name.clone()), // NUR 0xF8 (LD  HL, SP+e8)
            _ => panic!("Mehr als drei Operanden!"),
        };
        // Instruktion durchführen
        let jump = (instruction.mnemonic)(self, op1, op2);
        if !jump {self.reg_pc += instruction.bytes as u16;}

        // Vergangene Zyklen bestimmen
        if instruction.cycles.len() > 1 {
            if jump { self.remaining_cycles += instruction.cycles[0]; }
            else { self.remaining_cycles += instruction.cycles[1]; }
        }
        else { self.remaining_cycles += instruction.cycles[0]; }
        self.total_cycles += self.remaining_cycles as i64;
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
                let msb = self.regs[reg.clone() as usize];
                let lsb = self.regs[reg.clone() as usize + 1];
                lsb as u16 | ((msb as u16) << 8)
            }
        }
    }
    fn set_16(&mut self, reg: Register16, val: u16) {
        match reg{
            Register16::PC => self.reg_pc = val,
            Register16::SP => self.reg_sp = val,
            _ => {
                let msb = (val >> 8) as u8;
                let lsb = (val & 0xFF) as u8;
                self.regs[reg.clone() as usize] = msb;
                self.regs[reg.clone() as usize + 1] = lsb;
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
            opcodes_unprefixed: unpref, opcodes_prefixed: pref, bus: None, mode: CPUMode::Running,
            ime: false, set_ime: -1, remaining_cycles: 0, total_cycles: 0, total_operations: 1, ie_itr: false, if_itr: false
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
        self.set_8(Register8::F, 0xB0);
        self.set_8(Register8::B, 0x00);
        self.set_8(Register8::C, 0x00);
        self.set_8(Register8::D, 0xFF);
        self.set_8(Register8::E, 0x56);
        self.set_8(Register8::H, 0x00);
        self.set_8(Register8::L, 0x0D);
        self.set_16(Register16::PC, 0x0100);
        self.set_16(Register16::SP, 0xFFFE);
    }
    pub fn new_init(b: Rc<RefCell<Bus>>) -> Self {
        let mut this = SM83::new();
        this.bus = Some(b);
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
    fn fill_instructions() -> (Vec<Opcode>, Vec<Opcode>){
        let json: Result<serde_json::Value, serde_json::Error> = serde_json::from_str(JSON_OPS);
        let parsed_json = match json{
            Ok(val) => val,
            Err(e) => panic!("Problem beim Parsing: {e:?}"),
        };

        let mut unpref: Vec<Opcode> = vec![Opcode::default(); 256];
        let mut pref: Vec<Opcode> = vec![Opcode::default(); 256];

        for ar in [("unprefixed", &mut unpref), ("cbprefixed", &mut pref)]{
            for i in 0..256{
                let index = format!("{i:#04X}"); // Form: 0x0A
                let instr = &parsed_json[ar.0][index];
                let opop = parse_opcode(&instr);
                let op = match opop{
                    None => panic!("Json Parse Fehler in Instruktion {} in {}", i, ar.0),
                    Some(val) => val
                };
                ar.1[i] = op;
            }
        }
        (unpref, pref)
    }
}

fn get_instruction_fun(mnemonic: &str) -> InstructionFn{
    match mnemonic{
        "ADC" => SM83::adc,
        "ADD" => SM83::add,
        "AND" => SM83::and,
        "BIT" => SM83::bit,
        "CALL" => SM83::call,
        "CCF" => SM83::ccf,
        "CP" => SM83::cp,
        "CPL" => SM83::cpl,
        "DAA" => SM83::daa,
        "DEC" => SM83::dec,
        "DI" => SM83::di,
        "EI" => SM83::ei,
        "HALT" => SM83::halt,
        "INC" => SM83::inc,
        "JP" => SM83::jp,
        "JR" => SM83::jr,
        "LD" => SM83::ld,
        "LDH" => SM83::ldh,
        "NOP" => SM83::nop,
        "OR" => SM83::or,
        "POP" => SM83::pop,
        "PUSH" => SM83::push,
        "RES" => SM83::res,
        "RET" => SM83::ret,
        "RETI" => SM83::reti,
        "RL" => SM83::rl,
        "RLA" => SM83::rla,
        "RLC" => SM83::rlc,
        "RLCA" => SM83::rlca,
        "RR" => SM83::rr,
        "RRA" => SM83::rra,
        "RRC" => SM83::rrc,
        "RRCA" => SM83::rrca,
        "RST" => SM83::rst,
        "SBC" => SM83::sbc,
        "SCF" => SM83::scf,
        "SET" => SM83::set,
        "SLA" => SM83::sla,
        "SRA" => SM83::sra,
        "SRL" => SM83::srl,
        "STOP" => SM83::stop,
        "SUB" => SM83::sub,
        "SWAP" => SM83::swap,
        "XOR" => SM83::xor,
        "PREFIX" => SM83::prefix,
        err => SM83::prefix, // Illegale Ops führen zu hardlock normalerweise
    }
}

fn parse_opcode(val: &Value) -> Option<Opcode>{
    let mnemonic_str: String = val["mnemonic"].as_str()?.to_string();
    let mnemonic = get_instruction_fun(&mnemonic_str);
    let bytes: i32 = val["bytes"].as_i64()? as i32;
    let mut cycles: Vec<i32> = Vec::new();
    for elem in val["cycles"].as_array()?{
        cycles.push(elem.as_i64()? as i32);
    }
    let mut operands: Vec<Operand> = Vec::new();
    for elem in val["operands"].as_array()?{
        let mut incr_decr: i32 = 0;
        let incr = &elem["increment"];
        let decr = &elem["decrement"];
        match incr{
            serde_json::Value::Bool(true) => incr_decr = 1,
            _ => ()
        }
        match decr{
            serde_json::Value::Bool(true) => incr_decr = -1,
            _ => ()
        }
        let name = parse_operand_name(elem["name"].as_str()?, &mnemonic_str, &elem["immediate"], incr_decr);
        let imm_op = elem["immediate"].as_bool()?;
        let bytes_op = match elem["bytes"].as_i64(){
            None => None,
            Some(v) => Some(v as i32),
        };
        operands.push(Operand {name, immediate: imm_op, bytes: bytes_op});
    }

    Some(Opcode { mnemonic, bytes, cycles, operands })
}

fn parse_operand_name(name: &str, op: &str, immediate: &Value, increment: i32) -> OperandType{
    let imm_bool = match immediate{
        serde_json::Value::Bool(b) => b,
        _ => panic!("Hä?"),
    };
    match name{
        "n8" => OperandType::N8,
        "n16" => OperandType::N16,
        "a8" => OperandType::A8(imm_bool.clone()),
        "a16" => OperandType::A16(imm_bool.clone()),
        "e8" => OperandType::E8,
        "A" => OperandType::Reg8(Register8::A, imm_bool.clone()),
        "B" => OperandType::Reg8(Register8::B, imm_bool.clone()),
        "C" => match op{
            "JR" => OperandType::ConditionOp(Condition::C),
            "RET" => OperandType::ConditionOp(Condition::C),
            "JP" => OperandType::ConditionOp(Condition::C),
            "CALL" => OperandType::ConditionOp(Condition::C),
            _ => OperandType::Reg8(Register8::C, imm_bool.clone()),
        }
        "D" => OperandType::Reg8(Register8::D, imm_bool.clone()),
        "E" => OperandType::Reg8(Register8::E, imm_bool.clone()),
        "H" => OperandType::Reg8(Register8::H, imm_bool.clone()),
        "L" => OperandType::Reg8(Register8::L, imm_bool.clone()),
        "SP" => OperandType::Reg16(Register16::SP, imm_bool.clone(), increment),
        "PC" => OperandType::Reg16(Register16::PC, imm_bool.clone(), increment),
        "AF" => OperandType::Reg16(Register16::AF, imm_bool.clone(), increment),
        "BC" => OperandType::Reg16(Register16::BC, imm_bool.clone(), increment),
        "DE" => OperandType::Reg16(Register16::DE, imm_bool.clone(), increment),
        "HL" => OperandType::Reg16(Register16::HL, imm_bool.clone(), increment),
        "Z" => OperandType::ConditionOp(Condition::Z),
        "NZ" => OperandType::ConditionOp(Condition::NZ),
        "NC" => OperandType::ConditionOp(Condition::NC),
        _ => match name.chars().nth(0).unwrap(){
            '$' => {
                assert_eq!(op, "RST");
                let without_prefix = name.trim_start_matches("$");
                match u8::from_str_radix(without_prefix, 16){
                    Ok(num) => OperandType::ImplicitLiteral(num),
                    Err(e) => panic!("RST-Vektor Parse Fehler: {}", e),
                }
            },
            _ => {
                // => Es muss sich um ein u3 Literal handeln (Nur BIT, RES, SET)
                assert!(op == "BIT" || op == "RES" || op == "SET");
                match u8::from_str_radix(name, 10){
                    Ok(res) => OperandType::U3(res),
                    Err(e) => panic!("U3-Parse Fehler: {}", e),
                }
            }
        }
    }
}

#[cfg(test)]
mod test {
    use crate::gbc::sm83::OperandType;
    use crate::gbc::sm83::Register8;
    use crate::gbc::sm83::Register16;

    use super::SM83;
    use super::ProcessorFlags;
    #[test]
    fn flag_test(){

        let mut cpu = SM83::new();
        assert_eq!(cpu.get_status_flag(ProcessorFlags::ZeroFlag), false);

        // Zero Flag
        cpu.set_status_flag(ProcessorFlags::ZeroFlag, true);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::ZeroFlag), true);

        // Carry Flag
        cpu.set_status_flag(ProcessorFlags::CarryFlag, true);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::CarryFlag), true);

        // Half Carry Flag
        cpu.set_status_flag(ProcessorFlags::HalfCarryFlag, true);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::HalfCarryFlag), true);

        // Subtraction Flag
        cpu.set_status_flag(ProcessorFlags::SubtractionFlag, true);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::SubtractionFlag), true);

        // Andere Flags sind immer noch gesetzt
        assert_eq!(cpu.get_status_flag(ProcessorFlags::ZeroFlag), true);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::CarryFlag), true);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::HalfCarryFlag), true);

        // Wieder deaktivieren
        cpu.set_status_flag(ProcessorFlags::ZeroFlag, false);
        cpu.set_status_flag(ProcessorFlags::CarryFlag, false);
        cpu.set_status_flag(ProcessorFlags::HalfCarryFlag, false);
        cpu.set_status_flag(ProcessorFlags::SubtractionFlag, false);

        assert_eq!(cpu.get_status_flag(ProcessorFlags::ZeroFlag), false);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::CarryFlag), false);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::HalfCarryFlag), false);
        assert_eq!(cpu.get_status_flag(ProcessorFlags::SubtractionFlag), false);
    }

    #[test]
    fn parse_test(){
        let v = SM83::fill_instructions();
        assert_eq!(v.1[0xBF].operands.len(), 2);
        assert_eq!(v.1[0xBF].cycles[0], 8);
        assert_eq!(v.1[0xBF].operands[1].name, OperandType::Reg8(Register8::A, true));
        assert_eq!(v.1[0xBF].operands[1].immediate, true);

        assert_eq!((v.0[0x02].operands[0].name), OperandType::Reg16(Register16::BC, false, 0));
        assert_eq!((v.0[0x22].operands[0].name), OperandType::Reg16(Register16::HL, false, 1));
        assert_eq!((v.1[0xFC].operands[0].name), OperandType::U3(7));
    }

    #[test]
    fn test_regs_16(){
        let mut cpu = SM83::new();
        cpu.set_16(super::Register16::AF, 0b0101101010100101);
        assert_eq!(cpu.regs[0], 0b01011010);
        assert_eq!(cpu.regs[1], 0b10100101);
        assert_eq!(cpu.get_16(super::Register16::AF), 0b0101101010100101);
        cpu.set_16(super::Register16::AF, 0);
        assert_eq!(cpu.get_16(super::Register16::AF), 0);

        cpu.set_16(super::Register16::SP, 0b0101101010100101);
        assert_eq!(cpu.get_16(super::Register16::SP), 0b0101101010100101);
        cpu.set_16(super::Register16::PC, 0b0101101010100101);
        assert_eq!(cpu.get_16(super::Register16::PC), 0b0101101010100101);
    }

    #[test]
    fn test_regs_8(){
        let mut cpu = SM83::new();
        for reg in [Register8::A, Register8::B, Register8::C, Register8::D, Register8::E, Register8::H, Register8::L]{
            cpu.set_8(reg.clone(), 0b10101010);
            assert_eq!(cpu.get_8(reg.clone()), 0b10101010);
        }
        for reg in [Register16::BC, Register16::DE, Register16::HL]{
            assert_eq!(cpu.get_16(reg), 0b1010101010101010);
        }
    }

    #[test]
    fn condition_test(){
        let mut cpu = SM83::new();
        assert_eq!(cpu.get_condition(super::Condition::Z), false);
        assert_eq!(cpu.get_condition(super::Condition::C), false);
        assert_eq!(cpu.get_condition(super::Condition::NZ), true);
        assert_eq!(cpu.get_condition(super::Condition::NC), true);
        cpu.set_status_flag(ProcessorFlags::ZeroFlag, true);
        assert_eq!(cpu.get_condition(super::Condition::Z), true);
        assert_eq!(cpu.get_condition(super::Condition::C), false);
        assert_eq!(cpu.get_condition(super::Condition::NZ), false);
        assert_eq!(cpu.get_condition(super::Condition::NC), true);
        cpu.set_status_flag(ProcessorFlags::CarryFlag, true);
        assert_eq!(cpu.get_condition(super::Condition::Z), true);
        assert_eq!(cpu.get_condition(super::Condition::C), true);
        assert_eq!(cpu.get_condition(super::Condition::NZ), false);
        assert_eq!(cpu.get_condition(super::Condition::NC), false);
    }
}
