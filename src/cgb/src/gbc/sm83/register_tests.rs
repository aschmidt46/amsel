#[cfg(test)]
mod test {
    use crate::gbc::sm83::Condition;
    use crate::gbc::sm83::OperandType;
    use crate::gbc::sm83::Register8;
    use crate::gbc::sm83::Register16;
    use crate::gbc::sm83::SM83;
    use crate::gbc::sm83::ProcessorFlags;
    
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
        cpu.set_16(Register16::AF, 0b0101101010100101);
        assert_eq!(cpu.regs[0], 0b01011010);
        assert_eq!(cpu.regs[1], 0b10100101);
        assert_eq!(cpu.get_16(Register16::AF), 0b0101101010100101);
        cpu.set_16(Register16::AF, 0);
        assert_eq!(cpu.get_16(Register16::AF), 0);

        cpu.set_16(Register16::SP, 0b0101101010100101);
        assert_eq!(cpu.get_16(Register16::SP), 0b0101101010100101);
        cpu.set_16(Register16::PC, 0b0101101010100101);
        assert_eq!(cpu.get_16(Register16::PC), 0b0101101010100101);
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
        assert_eq!(cpu.get_condition(Condition::Z), false);
        assert_eq!(cpu.get_condition(Condition::C), false);
        assert_eq!(cpu.get_condition(Condition::NZ), true);
        assert_eq!(cpu.get_condition(Condition::NC), true);
        cpu.set_status_flag(ProcessorFlags::ZeroFlag, true);
        assert_eq!(cpu.get_condition(Condition::Z), true);
        assert_eq!(cpu.get_condition(Condition::C), false);
        assert_eq!(cpu.get_condition(Condition::NZ), false);
        assert_eq!(cpu.get_condition(Condition::NC), true);
        cpu.set_status_flag(ProcessorFlags::CarryFlag, true);
        assert_eq!(cpu.get_condition(Condition::Z), true);
        assert_eq!(cpu.get_condition(Condition::C), true);
        assert_eq!(cpu.get_condition(Condition::NZ), false);
        assert_eq!(cpu.get_condition(Condition::NC), false);
    }
}
