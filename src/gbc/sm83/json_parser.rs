use serde_json::Value;

use crate::gbc::sm83::{Condition, InstructionFn, Opcode, Operand, OperandType, Register8, Register16, SM83};

static JSON_OPS: &str = include_str!("../../../resources/Opcodes.json");

impl SM83{
    pub fn fill_instructions() -> (Vec<Opcode>, Vec<Opcode>){
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
        err => SM83::illegal, // Illegale Ops führen zu hardlock normalerweise
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

    Some(Opcode { mnemonic: (mnemonic, mnemonic_str), bytes, cycles, operands })
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
