
#[cfg(test)]
mod rom_tests {

    use crate::CGB;

    fn test_blargg_rom(rom_path: &str){
        let mut cgb = CGB::new(rom_path);
        cgb.bus.borrow_mut().set_test_mode();
        let mut count: usize = 0;
        let mut found: bool = false;
        let mut msg: String = "".to_string();
        let mut last_size: usize = 0;
        while count < 100000000 && !found{
            cgb.clock();
            count += 1;
            if count % 1000000 == 0{
                if cgb.bus.borrow_mut().test_output.as_mut().unwrap().len() > last_size{
                    msg = String::from_utf8(cgb.bus.borrow_mut().test_output.as_mut().unwrap().clone()).unwrap();
                    last_size = cgb.bus.borrow_mut().test_output.as_mut().unwrap().len();
                    if msg.contains("Passed"){
                        found = true;
                    }
                }
            }
        }
        if !found{
            panic!("{}",msg);
        }
    }

    #[test]
    fn instr_1(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/01-special.gb");
    }
    #[test]
    fn instr_2(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/02-interrupts.gb");
    }
    #[test]
    fn instr_3(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/03-op sp,hl.gb");
    }
    #[test]
    fn instr_4(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/04-op r,imm.gb");
    }
    #[test]
    fn instr_5(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/05-op rp.gb");
    }
    #[test]
    fn instr_6(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/06-ld r,r.gb");
    }
    // #[test]
    fn instr_7(){ // Log fehlerhaft
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb");
    }
    #[test]
    fn instr_8(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/08-misc instrs.gb");
    }
    #[test]
    fn instr_9(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/09-op r,r.gb");
    }
    #[test]
    fn instr_10(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/10-bit ops.gb");
    }
    #[test]
    fn instr_11(){
        test_blargg_rom("./resources/gb-test-roms-master/cpu_instrs/individual/11-op a,(hl).gb");
    }
}