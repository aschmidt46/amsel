// Große Aufgabe, Großes Projekt, viel Erfolg und auf gutes Gelingen ;)
mod sm83 {
    use core::panic;

    use crate::gbc::sm83::{self, CPUMode, OperandType, Register8, Register16, SM83};

    impl SM83 {
        pub fn adc(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let val = match op2{
                OperandType::Reg8(reg, _) => self.get_8(reg),
                OperandType::N8 => self.read(self.reg_pc + 1),
                OperandType::Reg16(reg, _, _) => self.read(self.get_16(reg)),
                _ => panic!("Operation existiert so nicht {:?}", op2),
            };
            let a = self.get_8(sm83::Register8::A);
            let carry_add: u8 = self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8;
            let adc_res = a as u16 + val as u16 + carry_add as u16; // Addition A + (Op) + Carry
            let carry: bool = adc_res > 0xFF;
            let half_carry: bool = ((a & 0xF) + (val & 0xF) + carry_add) >= 0x10;
            self.set_8(sm83::Register8::A, adc_res as u8);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, adc_res as u8 == 0);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, half_carry);
            false
        }
        pub fn add(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(Register8::A,_) => { // Erste Variante
                    let val: u16 = match op2{
                        OperandType::Reg8(reg,_) => self.get_8(reg) as u16,
                        OperandType::Reg16(reg,_ ,_) => self.read(self.get_16(reg)) as u16,
                        OperandType::N8 => self.read(self.reg_pc + 1) as u16,
                        _ => panic!("Unbekannte Variante von ADD ausgelöst! (In Addition auf A)"),
                    };
                    let a = self.get_8(Register8::A) as u16;
                    let add_res = a + val;
                    let carry: bool = add_res > 0xFF;
                    let half_carry: bool = ((a & 0xF) + (val & 0xF)) >= 0x10;
                    self.set_8(sm83::Register8::A, add_res as u8);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, add_res as u8 == 0);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, half_carry);
                }
                OperandType::Reg16(Register16::HL, _, _) => { // Zweite Variante
                    let val: u32 = match op2{
                        OperandType::Reg16(reg,_,_) => self.get_16(reg) as u32,
                        _ => panic!("Unbekannte Variante von ADD ausgelöst! (In Addition auf A)"),
                    };
                    let a = self.get_16(Register16::HL) as u32;
                    let add_res = a + val;
                    let carry: bool = add_res > 0xFFFF;
                    let half_carry: bool = ((a & 0xFFF) + (val & 0xFFF)) >= 0x1000; // bit 11 overflow
                    self.set_16(Register16::HL, add_res as u16);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, half_carry);

                }
                OperandType::Reg16(Register16::SP, _, _) => { // SP-Variante
                    let val = self.read(self.reg_pc + 1) as i8; // Signed
                    let r16 = self.get_16(Register16::SP);
                    let mut add_res = r16;
                    if val < 0{ // Was für ein Schrott
                        let vval = (-val) as u16; 
                        let maxval = (0x10000 - (vval as u32)) as u16;
                        add_res = add_res.wrapping_add(maxval);
                        let carry: bool = (r16 & 0xFF) + (maxval & 0xFF) as u16 > 0xFF;
                        let half_carry: bool = ((r16 & 0xF) + (maxval as u16 & 0xF)) >= 0x10;
                        self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                        self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, half_carry);
                    }
                    else{
                        add_res = add_res.wrapping_add(val as u16);
                        let carry: bool = (r16 & 0xFF) + val as u16 > 0xFF;
                        let half_carry: bool = ((r16 & 0xF) + (val as u16 & 0xF)) >= 0x10;
                        self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                        self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, half_carry);
                    }

                    self.set_16(Register16::SP, add_res);

                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, false);
                }
                _ => panic!("Unbekannte Variante von ADD ausgelöst!"),
            }
            false
        }
        pub fn and(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let val: u8 = match op2{
                OperandType::Reg8(reg, _) => self.get_8(reg),
                OperandType::Reg16(Register16::HL, false, 0) => self.read(self.get_16(Register16::HL)),
                OperandType::N8 => self.read(self.reg_pc + 1),
                _ => panic!("Invalide Variante von AND ausgelöst!"),
            };
            let and_res = self.get_8(Register8::A) & val;
            self.set_8(Register8::A, and_res);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, and_res==0);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, true);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, false);
            false
        }
        pub fn bit(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::U3(n) => {
                    let mask = (1 as u8) << n;
                    let test = match op2{
                        OperandType::Reg8(reg, _) => self.get_8(reg),
                        OperandType::Reg16(reg,_ ,_ ) => self.read(self.get_16(reg)),
                        _ => panic!("Invalider Op2 von BIT!"),
                    };
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, test & mask == 0);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, true);
                },
                _ => panic!("Invalide Variante von BIT ausgelöst!"),
            }
            false
        }
        pub fn call(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let instr_after = self.reg_pc + 3;
            let mut cond = true;
            let goal = match op1{
                OperandType::ConditionOp(c) => {
                    cond = self.get_condition(c);
                    self.read_16(self.reg_pc + 1)
                },
                _ => self.read_16(self.reg_pc + 1)
            };
            if cond{
                self.push_stack_16(instr_after);
                self.set_16(Register16::PC, goal);
                true
            }
            else{
                false
            }
        }
        pub fn ccf(&mut self, op1: OperandType, op2: OperandType) -> bool{
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, !self.get_status_flag(sm83::ProcessorFlags::CarryFlag));
            false
        }
        pub fn cp(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let value = match op2{
                OperandType::Reg8(reg, _) => self.get_8(reg),
                OperandType::Reg16(reg, _, _) => self.read(self.get_16(reg)),
                OperandType::N8 => self.read(self.reg_pc + 1),
                _ => panic!("Unbekannte Variante von CP aufgerufen!"),
            };
            let a = self.get_8(Register8::A);
            let res = a.wrapping_sub(value);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, true);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, value & 0xF > a & 0xF);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, value > a);
            false
        }
        pub fn cpl(&mut self, op1: OperandType, op2: OperandType) -> bool{
            self.set_8(Register8::A, !self.get_8(Register8::A));
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, true);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, true);
            false
        }
        pub fn daa(&mut self, op1: OperandType, op2: OperandType) -> bool{ // Decimal modus
            let mut adjustment: u8 = 0;
            let result: u8;
            let a = self.get_8(Register8::A);
            if self.get_status_flag(sm83::ProcessorFlags::SubtractionFlag){
                if self.get_status_flag(sm83::ProcessorFlags::HalfCarryFlag){
                    adjustment += 0x06;
                }
                if self.get_status_flag(sm83::ProcessorFlags::CarryFlag){
                    adjustment += 0x60;
                }

                result = a.wrapping_sub(adjustment);                
            }
            else{
                if self.get_status_flag(sm83::ProcessorFlags::HalfCarryFlag) || ((a & 0xF) > 0x9) {
                    adjustment += 0x06;
                }
                if self.get_status_flag(sm83::ProcessorFlags::CarryFlag) || (a > 0x99) {
                    adjustment += 0x60;
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, true);
                }

                result = a.wrapping_add(adjustment);
            }
            self.set_8(Register8::A, result);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, result == 0);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn dec(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let reg_val = self.get_8(reg);
                    let res = reg_val.wrapping_sub(1);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, true);
                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, 1 > reg_val & 0xF);
                    self.set_8(reg, res);
                },
                OperandType::Reg16(Register16::HL, false, _) => {
                    let reg_val = self.read(self.get_16(Register16::HL));
                    let res = reg_val.wrapping_sub(1);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, true);
                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, 1 > reg_val & 0xF);
                    self.write(self.get_16(Register16::HL), res);
                },
                OperandType::Reg16(reg, true, _) => { // Inklusive immediate HL!
                    self.set_16(reg, self.get_16(reg).wrapping_sub(1));
                },
                _ => panic!("Unbekannte Version von DEC aufgerufen!"),
            }
            false
        }
        pub fn di(&mut self, op1: OperandType, op2: OperandType) -> bool{
            self.ime = false;
            false
        }
        pub fn ei(&mut self, op1: OperandType, op2: OperandType) -> bool{
            self.set_ime = 1;
            false
        }
        pub fn halt(&mut self, op1: OperandType, op2: OperandType) -> bool{
            self.mode = CPUMode::Halted;
            false
        }
        pub fn inc(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let reg_val: u8 = self.get_8(reg);
                    let res = reg_val.wrapping_add(1);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, (reg_val & 0xF) + 1 > 0xF);
                    self.set_8(reg, res);
                },
                OperandType::Reg16(Register16::HL, false, _) => {
                    let reg_val = self.read(self.get_16(Register16::HL));
                    let res = reg_val.wrapping_add(1);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, (reg_val & 0xF) + 1 > 0xF);
                    self.write(self.get_16(Register16::HL), res);
                },
                OperandType::Reg16(reg, true, _) => { // Inklusive immediate HL!
                    self.set_16(reg, self.get_16(reg).wrapping_add(1));
                },
                _ => panic!("Unbekannte Version von INC aufgerufen!"),
            }
            false
        }
        pub fn jp(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let mut cond = true;
            let target = match op1{
                OperandType::A16(true) => self.read_16(self.reg_pc + 1),
                OperandType::ConditionOp(c) => {
                    cond = self.get_condition(c);
                    self.read_16(self.reg_pc + 1)
                },
                OperandType::Reg16(reg, _, _) => self.get_16(reg),
                _ => panic!("Nicht existente Variante von JP aufgerufen!"),
            };
            if cond{
                self.set_16(Register16::PC, target);
                true
            }
            else{ // Sprung nicht genommen
                false
            }
        }
        pub fn jr(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let cond = match op1{
                OperandType::ConditionOp(c) => self.get_condition(c),
                _ => true,
            };
            let rel: i8 = self.read(self.reg_pc + 1) as i8;
            let mut target = self.reg_pc + 2; // Op ist immer 2 Bytes
            if rel < 0{ // Wieder dieser Schrott
                let rel_a = (-rel) as u16;
                target -= rel_a;
            }
            else{
                target += rel as u16;
            }
            
            if cond{
                self.set_16(Register16::PC, target);
                true
            }
            else{ // Sprung nicht genommen
                false
            }
        }
        pub fn ld(&mut self, op1: OperandType, op2: OperandType) -> bool{ // Achtung bei 0xF8
            match op2{
                // Spezialfälle
                OperandType::E8 => { // LD HL, SP+e8
                    let sp = self.get_16(Register16::SP);
                    let offset = self.read(self.reg_pc + 1) as i8;
                    let mut off_positive : u16 = 0x0000;
                    off_positive = off_positive.wrapping_add_signed(offset as i16);
                    let res = sp.wrapping_add(off_positive);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, false);
                    self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);

                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, 
                        (sp & 0xFF) + (off_positive & 0xFF) > 0xFF
                        );

                    self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, 
                        (sp & 0xF) + (off_positive & 0xF) > 0xF
                        );

                    self.set_16(Register16::HL, res);
                },
                OperandType::A16(false) => { // Immediate Wert lesen -> Wert an Adresse lesen
                    self.set_8(Register8::A, self.read(self.read_16(self.reg_pc + 1)));
                },
                // 8bit Immediate
                OperandType::N8 => {
                    let second = self.read(self.reg_pc + 1);
                    match op1{
                        OperandType::Reg8(reg, true) => self.set_8(reg, second),
                        OperandType::Reg16(Register16::HL, false, 0) => self.write(self.get_16(Register16::HL), second),
                        _ => panic!("Unbekannte Kombination in LD-Instruktion!"),
                    }
                },
                // 16 Bit Register op2's
                OperandType::Reg16(Register16::HL, true, 0) => { // LD SP, HL
                    self.set_16(Register16::SP, self.get_16(Register16::HL));
                }
                OperandType::Reg16(Register16::SP, true, 0) => { // LD [n16], SP
                    let low = (self.get_16(Register16::SP) & 0xFF) as u8;
                    let high = (self.get_16(Register16::SP) >> 8) as u8;
                    let addr = self.read_16(self.reg_pc + 1);
                    self.write(addr, low);
                    self.write(addr + 1, high);
                }
                OperandType::Reg16(reg, false, incr) => { // alle anderen sind nicht immediate LD r8, [...]
                    let addr = self.get_16(reg);
                    let rtarget = match op1{
                        OperandType::Reg8(reg, true) => reg,
                        _ => panic!("Unbekannte Variante von LD!"),
                    };
                    self.set_8(rtarget, self.read(addr));
                    self.set_16(reg, self.get_16(reg).wrapping_add_signed(incr as i16)); // Bei HLD und HLI
                },
                // 16-bit Immediate
                OperandType::N16 => {
                    let reg = match op1{
                        OperandType::Reg16(r, _, _) => r,
                        _ => panic!("Unbekannte Variante von LD!"),
                    };
                    self.set_16(reg, self.read_16(self.reg_pc + 1));
                },
                // 8-Bit Register
                OperandType::Reg8(source, true) => {
                    match op1{
                        OperandType::A16(_) => {
                            self.write(self.read_16(self.reg_pc+ 1), self.get_8(Register8::A));
                        },
                        OperandType::Reg16(dest, false, incr) => {
                            let target = self.get_16(dest);
                            self.write(target, self.get_8(source));
                            self.set_16(dest, self.get_16(dest).wrapping_add_signed(incr as i16)); // Inkrement addieren
                        },
                        OperandType::Reg8(dest, true) => {
                            self.set_8(dest, self.get_8(source));
                        },
                        _ => panic!("Unbekannter erster Operand von LD!"),
                    }
                },
                _ => panic!("Unbekannter zweiter Operand von LD!"),
            }
            false
        }
        pub fn ldh(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::A8(false) => {
                    self.write(0xFF00 + (self.read(self.reg_pc + 1) as u16), self.get_8(Register8::A));
                },
                OperandType::Reg8(Register8::C, false) => {
                    self.write(0xFF00 + (self.get_8(Register8::C) as u16), self.get_8(Register8::A));
                },
                OperandType::Reg8(Register8::A, true) => {
                    match op2{
                        OperandType::Reg8(Register8::C, false) => {
                            self.set_8(Register8::A, self.read(0xFF00 + (self.get_8(Register8::C) as u16)));
                        },
                        OperandType::A8(false) => {
                            self.set_8(Register8::A, self.read(0xFF00 + (self.read(self.reg_pc + 1) as u16)));
                        },
                        _ => panic!("Unbekannter zweiter Operand von LDH"),
                    }
                },
                _ => panic!("Unbekannte Variante von LDH aufgerufen!"),
            }
            false
        }
        pub fn nop(&mut self, op1: OperandType, op2: OperandType) -> bool{
            //NOP
            false
        }
        pub fn or(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let result: u8;
            match op2{
                OperandType::Reg8(reg, true) => {
                    result = self.get_8(Register8::A) | self.get_8(reg);
                },
                OperandType::Reg16(reg, false, _) => {
                    result = self.get_8(Register8::A) | self.read(self.get_16(reg));
                },
                OperandType::N8 => {
                    result = self.get_8(Register8::A) | self.read(self.reg_pc + 1);
                },
                _ => panic!("Unbekannte Variante von OR!"),
            }

            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, result == 0);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, false);

            self.set_8(Register8::A, result);
            false
        }
        pub fn pop(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg16(Register16::AF, _, _) => {
                    let popped = self.pop_stack_16();
                    let af = popped & 0xFFF0; // F-Register hat nur oberste 4 bit
                    self.set_16(Register16::AF, af);
                },
                OperandType::Reg16(reg, _, _) => {
                    let popped = self.pop_stack_16();
                    self.set_16(reg, popped);
                },
                _ => panic!("Unbekannte Variante von POP!"),
            }
            false
        }
        pub fn push(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg16(reg, _, _) => {
                    self.push_stack_16(self.get_16(reg));
                },
                _ => panic!("Unbekannte Variante von PUSH!"),
            }
            false
        }
        pub fn res(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let b = match op1{
                OperandType::U3(n) => n,
                _ => panic!("Unbekannte Variante von RES!"),
            };
            match op2{
                OperandType::Reg8(reg, _) => {
                    self.set_8(reg, self.get_8(reg) & (!(1 << b)));
                }
                OperandType::Reg16(reg, _, _) => { // Lese- und Schreiboperation?
                    let val = self.read(self.get_16(reg));
                    self.write(self.get_16(reg), val & (!(1 << b)));
                },
                _ => panic!("Unbekannter zweiter Operand von RES!"),
            }
            false
        }
        pub fn ret(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let cond = match op1{
                OperandType::ConditionOp(c) => self.get_condition(c),
                _ => true,
            };

            if cond {
                let new_pc = self.pop_stack_16();
                self.set_16(Register16::PC, new_pc);
                true
            }
            else {
                false
            }
        }
        pub fn reti(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let new_pc = self.pop_stack_16();
            self.set_16(Register16::PC, new_pc);
            self.ime = true;
            true
        }
        pub fn rl(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg);
                    let r = (val as u16) << 1;
                    let carry = (r & 0x0100) != 0;
                    let r8 = (r as u8) | (self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8);
                    self.set_8(reg, r8);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, r8 == 0);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg));
                    let r = (val as u16) << 1;
                    let carry = (r & 0x0100) != 0;
                    let r8 = (r as u8) | (self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8);
                    self.write(self.get_16(reg), r8);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, r8 == 0);
                },
                _ => panic!("Unbekannte Variante von RL: {:?}", op1),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn rla(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let val = self.get_8(Register8::A);
            let r = (val as u16) << 1;
            let carry = (r & 0x0100) != 0;
            let r8 = (r as u8) | (self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8);
            self.set_8(Register8::A, r8);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
            false
        }
        pub fn rlc(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg).rotate_left(1);
                    self.set_8(reg, val);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, (val & 1) != 0);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, val == 0);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg)).rotate_left(1);
                    self.write(self.get_16(reg), val);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, (val & 1) != 0);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, val == 0);
                },
                _ => panic!("Unbekannte Variante von RLC"),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn rlca(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let val = self.get_8(Register8::A).rotate_left(1);
            self.set_8(Register8::A, val);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, (val & 1) != 0);
            false
        }
        pub fn rr(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg);
                    let r = (val as u16).rotate_right(1);
                    let carry = (r & 0x8000) != 0;
                    let r8 = (r as u8) | ((self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8) << 7);
                    self.set_8(reg, r8);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, r8 == 0);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg));
                    let r = (val as u16).rotate_right(1);
                    let carry = (r & 0x8000) != 0;
                    let r8 = (r as u8) | ((self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8) << 7);
                    self.write(self.get_16(reg), r8);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, r8 == 0);
                },
                _ => panic!("Unbekannte Variante von RR"),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn rra(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let val = self.get_8(Register8::A);
            let r = (val as u16).rotate_right(1);
            let carry = (r & 0x8000) != 0;
            let r8 = (r as u8) | ((self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8) << 7);
            self.set_8(Register8::A, r8);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
            false
        }
        pub fn rrc(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg).rotate_right(1);
                    self.set_8(reg, val);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, (val & 0x80) != 0);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, val == 0);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg)).rotate_right(1);
                    self.write(self.get_16(reg), val);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, (val & 0x80) != 0);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, val == 0);
                },
                _ => panic!("Unbekannte Variante von RLC"),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn rrca(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let val = self.get_8(Register8::A).rotate_right(1);
            self.set_8(Register8::A, val);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, (val & 0x80) != 0);
            false
        }
        pub fn rst(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let vec = match op1{
                OperandType::ImplicitLiteral(n) => n,
                _ => panic!("Unbekannter Aufruf von RST!"),
            };
            self.push_stack_16(self.reg_pc + 1); // RST ist ein Byte groß
            self.reg_pc = vec as u16;
            true
        }
        pub fn sbc(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let a = self.get_8(Register8::A);
            let c = self.get_status_flag(sm83::ProcessorFlags::CarryFlag) as u8;
            let subtr = match op2{
                OperandType::Reg8(reg, _) => self.get_8(reg),
                OperandType::Reg16(reg, _, _) => self.read(self.get_16(reg)),
                OperandType::N8 => self.read(self.reg_pc + 1),
                _ => panic!("Unbekannte Variante von SBC aufgerufen!"),
            };
            let res = a.wrapping_sub(subtr).wrapping_sub(c);
            self.set_8(Register8::A, res);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, true);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, (subtr & 0xF) + c > a & 0xF);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, subtr as u16 + c as u16 > a as u16);
            false
        }
        pub fn scf(&mut self, op1: OperandType, op2: OperandType) -> bool{
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, true);
            false
        }
        pub fn set(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let b = match op1{
                OperandType::U3(n) => n,
                _ => panic!("Unbekannte Variante von SET!"),
            };
            match op2{
                OperandType::Reg8(reg, _) => {
                    self.set_8(reg, self.get_8(reg) | (1 << b));
                },
                OperandType::Reg16(reg, _, _) => {
                    let addr = self.get_16(reg);
                    let val = self.read(addr);
                    self.write(addr, val | (1 << b));
                },
                _ => panic!("Unbekannter zweiter Operand von SET!"),
            }
            false
        }
        pub fn sla(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg) as u16;
                    let sh = val << 1;
                    let carry = (sh & 0x100) != 0;
                    self.set_8(reg, sh as u8);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, (sh as u8) == 0);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg)) as u16;
                    let sh = val << 1;
                    let carry = (sh & 0x100) != 0;
                    self.write(self.get_16(reg), sh as u8);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, (sh as u8) == 0);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                },
                _ => panic!("Unbekannte Variante von SLA!"),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn sra(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg) as u16;
                    let sh = val.rotate_right(1);
                    let carry = (sh & 0x8000) != 0;
                    let bit7 = val & 0x80;
                    let res = (sh as u8) | (bit7 as u8);
                    self.set_8(reg, res);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg)) as u16;
                    let sh = val.rotate_right(1);
                    let carry = (sh & 0x8000) != 0;
                    let bit7 = val & 0x80;
                    let res = (sh as u8) | (bit7 as u8);
                    self.write(self.get_16(reg), res);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                },
                _ => panic!("Unbekannte Variante von SRA!"),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn srl(&mut self, op1: OperandType, op2: OperandType) -> bool{ // Das gleiche wie SRA, nur ohne Bit 7
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg) as u16;
                    let sh = val.rotate_right(1);
                    let carry = (sh & 0x8000) != 0;
                    let res = sh as u8;
                    self.set_8(reg, res);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg)) as u16;
                    let sh = val.rotate_right(1);
                    let carry = (sh & 0x8000) != 0;
                    let res = sh as u8;
                    self.write(self.get_16(reg), res);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                    self.set_status_flag(sm83::ProcessorFlags::CarryFlag, carry);
                },
                _ => panic!("Unbekannte Variante von SRL!"),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            false
        }
        pub fn stop(&mut self, op1: OperandType, op2: OperandType) -> bool{
            self.mode = CPUMode::Stopped;
            false
        }
        pub fn sub(&mut self, op1: OperandType, op2: OperandType) -> bool{
            // A - B
            // H gesetzt, wenn lower nibble von B > lower nibble von A
            let a = self.get_8(Register8::A);
            let subtr = match op2{
                OperandType::Reg8(reg, _) => self.get_8(reg),
                OperandType::Reg16(reg, _, _) => self.read(self.get_16(reg)),
                OperandType::N8 => self.read(self.reg_pc + 1),
                _ => panic!("Ungültige SUB Instruktion aufgerufen!"),
            };
            let res = a.wrapping_sub(subtr);
            self.set_8(Register8::A, res);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, true);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, subtr & 0xF > a & 0xF);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, subtr > a);
            false
        }
        pub fn swap(&mut self, op1: OperandType, op2: OperandType) -> bool{
            match op1{
                OperandType::Reg8(reg, _) => {
                    let val = self.get_8(reg);
                    let high = (val & 0xF0) >> 4;
                    let low = val & 0x0F;
                    let res = (low << 4) | high;
                    self.set_8(reg, res);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                },
                OperandType::Reg16(reg, _, _) => {
                    let val = self.read(self.get_16(reg));
                    let high = (val & 0xF0) >> 4;
                    let low = val & 0x0F;
                    let res = (low << 4) | high;
                    self.write(self.get_16(reg), res);
                    self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
                },
                _ => panic!("Unbekannte Variante von SWAP!"),
            }
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, false);
            false
        }
        pub fn xor(&mut self, op1: OperandType, op2: OperandType) -> bool{
            let a = self.get_8(Register8::A);
            let val = match op2{
                OperandType::Reg8(reg, _) => self.get_8(reg),
                OperandType::Reg16(reg, _, _) => self.read(self.get_16(reg)),
                OperandType::N8 => self.read(self.reg_pc + 1),
                _ => panic!("Unbekannte Variante von XOR!"),
            };
            let res = a ^ val;
            self.set_8(Register8::A, res);
            self.set_status_flag(sm83::ProcessorFlags::ZeroFlag, res == 0);
            self.set_status_flag(sm83::ProcessorFlags::SubtractionFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::HalfCarryFlag, false);
            self.set_status_flag(sm83::ProcessorFlags::CarryFlag, false);
            false
        }
        // Geschafft!!
        pub fn prefix(&mut self, op1: OperandType, op2: OperandType) -> bool{
            // dummy Funktion
            panic!("Dummy-Funktion (Prefix) aufgerufen!");
            // false
        }
    }
}
