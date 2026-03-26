
#[cfg(test)]
mod rom_tests {
    use std::cell::RefCell;
    use std::fs::File;
    use std::io::BufRead;
    use std::io::BufReader;
    use std::rc::Rc;

    use crate::gbc::CGB;
    use crate::gbc::bus::Bus;
    use crate::gbc::sm83::SM83;
    use crate::gbc::sm83::Register8;
    use crate::gbc::sm83::Register16;

    fn log_line(cpu: &SM83, include_end: bool, log_cycle: bool) -> String{
        let mut line: String = "A: ".to_owned();
        let a = format!("{:02X}", cpu.get_8(Register8::A));
        let fs = " F: ";
        let f = format!("{:02X}", cpu.get_8(Register8::F));
        let bs = " B: ";
        let b = format!("{:02X}", cpu.get_8(Register8::B));
        let cs = " C: ";
        let c = format!("{:02X}", cpu.get_8(Register8::C));
        let ds = " D: ";
        let d = format!("{:02X}", cpu.get_8(Register8::D));
        let es = " E: ";
        let e = format!("{:02X}", cpu.get_8(Register8::E));
        let hs = " H: ";
        let h = format!("{:02X}", cpu.get_8(Register8::H));
        let ls = " L: ";
        let l = format!("{:02X}", cpu.get_8(Register8::L));
        let sps = " SP: ";
        let sp = format!("{:04X}", cpu.get_16(Register16::SP));
        let pcs = " PC: 00:";
        let pc = format!("{:04X}", cpu.get_16(Register16::PC));
        line.push_str(&a);
        line.push_str(fs);
        line.push_str(&f);
        line.push_str(bs);
        line.push_str(&b);
        line.push_str(cs);
        line.push_str(&c);
        line.push_str(ds);
        line.push_str(&d);
        line.push_str(es);
        line.push_str(&e);
        line.push_str(hs);
        line.push_str(&h);
        line.push_str(ls);
        line.push_str(&l);
        line.push_str(sps);
        line.push_str(&sp);
        line.push_str(pcs);
        line.push_str(&pc);
        if include_end {
            let nextstart = " (";
            let ws = " ";
            let next1 = format!("{:02X}", cpu.read(cpu.get_16(Register16::PC)));
            let next2 = format!("{:02X}", cpu.read(cpu.get_16(Register16::PC) + 1));
            let next3 = format!("{:02X}", cpu.read(cpu.get_16(Register16::PC) + 2));
            let next4 = format!("{:02X}", cpu.read(cpu.get_16(Register16::PC) + 3));
            let nextend = ")";
            line.push_str(nextstart);
            line.push_str(&next1);
            line.push_str(ws);
            line.push_str(&next2);
            line.push_str(ws);
            line.push_str(&next3);
            line.push_str(ws);
            line.push_str(&next4);
            line.push_str(nextend);
        }
        if log_cycle{
            let cyc = format!("{}", cpu.total_cycles);
            let c = " CYC: ";
            line.push_str(c);
            line.push_str(&cyc);
        }
        line
    }

    // mesen log
    fn test_rom_against_log_new(rom_path: &str, log_path: &str, pc_off: u16){
        let mut cgb = CGB::new(rom_path);
        cgb.bus.borrow_mut().force_ly(0x92);
        cgb.bus.borrow_mut().force_ppu_cycle(0xAF);
        cgb.bus.borrow_mut().force_cpu_cycle(906458);
        cgb.bus.borrow_mut().cpu.as_mut().unwrap().remaining_cycles = 0;

        // while cpu.total_operations < 15000000 {
        //     cpu.clock()
        // };
        // panic!("Ende");

        // Für Einfachheit:
        // bus.borrow_mut().write_memory(0xFF44, 0x90);

        let file = File::open(log_path);
        match file{
            Ok(f) => {
                let reader = BufReader::new(f);
                let mut prev_line = "".to_string();

                let mut line_count = 1;
                for line in reader.lines() {
                    match line{
                        Ok(mut l) => {
                            l = l[6..].to_owned();
                            let mut current_line = log_line(&cgb.bus.borrow_mut().cpu.as_mut().unwrap(), false, true);
                            let cyc = format!(" PPUCYC: {}", cgb.bus.borrow_mut().ppu.as_mut().unwrap().cycle);
                            let ly = format!(" LY: {}", cgb.bus.borrow_mut().ppu.as_mut().unwrap().scanline);
                            current_line.push_str(&cyc);
                            current_line.push_str(&ly);
                            if current_line != l{
                                println!("vorherige:\t{}", prev_line);
                                println!("aktuelle: \t{}", current_line);
                                println!("---------------------------------------------------------------------------------------------------------");
                                println!("soll:     \t{}", l);
                                panic!("Test gescheitert! Log weicht ab in Zeile {}", line_count);
                            }
                            prev_line = current_line;
                            while !cgb.cpu_has_advanced(){
                                cgb.clock();
                            }
                            let num = cgb.bus.borrow_mut().cpu.as_mut().unwrap().remaining_cycles;
                            for i in 0..num-1{
                                cgb.clock();
                            }
                            line_count += 1;
                        },
                        Err(e) => panic!("Konnte Zeile nicht lesen: {}", e),
                    }
                }
            },
            Err(e) => panic!("Datei Lesen Fehler: {}", e),
        }
    }

    fn test_rom_against_log(rom_path: &str, log_path: &str, pc_off: u16){
        let bus = Rc::new(RefCell::new(Bus::new()));
        let mut cpu = SM83::new_init(Rc::downgrade(&bus));
        cpu.set_initial_state_dmg();
        cpu.reg_pc += pc_off;
        let bytes = std::fs::read(rom_path).unwrap();
        for i in 0..bytes.len(){
            bus.borrow_mut().write_memory(i as u16, bytes[i]);
        }

        // while cpu.total_operations < 15000000 {
        //     cpu.clock()
        // };
        // panic!("Ende");

        // Für Einfachheit:
        bus.borrow_mut().write_memory(0xFF44, 0x90);

        let file = File::open(log_path);
        match file{
            Ok(f) => {
                let reader = BufReader::new(f);
                let mut prev_line = "".to_string();

                let mut line_count = 1;
                for line in reader.lines() {
                    match line{
                        Ok(l) => {
                            let current_line = log_line(&cpu, true, false);
                            if current_line != l{
                                println!("vorherige:\t{}", prev_line);
                                println!("aktuelle: \t{}", current_line);
                                println!("---------------------------------------------------------------------------------------------------------");
                                println!("soll:     \t{}", l);
                                panic!("Test gescheitert! Log weicht ab in Zeile {}", line_count);
                            }
                            prev_line = current_line;
                            cpu.run_next_instruction();
                            line_count += 1;
                        },
                        Err(e) => panic!("Konnte Zeile nicht lesen: {}", e),
                    }
                }
            },
            Err(e) => panic!("Datei Lesen Fehler: {}", e),
        }
    }

    #[test]
    fn kristall(){
        test_rom_against_log_new("./resources/pokemonkristall.gbc",
        "./pokemon_log_kristall.txt", 0);
    }
    #[test]
    fn instr_1(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/01-special.gb",
        "./resources/Gameboy-logs-master/Blargg1LYStubbed/EpicLog.txt", 0);
    }
    #[test]
    fn instr_3(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/03-op sp,hl.gb",
        "./resources/Gameboy-logs-master/Blargg3LYStubbed/EpicLog.txt", 0);
    }
    #[test]
    fn instr_4(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/04-op r,imm.gb",
        "./resources/Gameboy-logs-master/Blargg4LYStubbed/Blargg4.txt", 0);
    }
    #[test]
    fn instr_5(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/05-op rp.gb",
        "./resources/Gameboy-logs-master/Blargg5LYStubbed/Blargg5.txt", 0);
    }
    #[test]
    fn instr_6(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/06-ld r,r.gb",
        "./resources/Gameboy-logs-master/Blargg6LYStubbed/EpicLog.txt", 1);
    }
    // #[test]
    fn instr_7(){ // Log fehlerhaft
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb",
        "./resources/Gameboy-logs-master/Blargg7LYStubbed/Blargg7.txt", 0);
    }
    #[test]
    fn instr_8(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/08-misc instrs.gb",
        "./resources/Gameboy-logs-master/Blargg8LYStubbed/EpicLog.txt", 0);
    }
    #[test]
    fn instr_9(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/09-op r,r.gb",
        "./resources/Gameboy-logs-master/Blargg9LYStubbed/Blargg9.txt", 0);
    }
    #[test]
    fn instr_10(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/10-bit ops.gb",
        "./resources/Gameboy-logs-master/Blargg10LYStubbed/Blargg10.txt", 0);
    }
    #[test]
    fn instr_11(){
        test_rom_against_log("./resources/gb-test-roms-master/cpu_instrs/individual/11-op a,(hl).gb",
        "./resources/Gameboy-logs-master/Blargg11LYStubbed/Blargg11.txt", 0);
    }
}