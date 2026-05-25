#include <gtest/gtest.h>
#include "../arm/arm7tdmi.h"
#include <fstream>
#include "cpustate.h"
#include <memory>
#include "testbus.h"
#include "logging.h"
#include <filesystem>
#include <bitset>

using namespace gba;

// Demonstrate some basic assertions.
TEST(CPUTest, BasicAssertions){
    auto tb = std::make_shared<TestBus>();
    CPU cpu(tb);
    ASSERT_EQ(cpu.boardWRAM[0], 0);
    // Expect two strings not to be equal.
    // EXPECT_STRNE("hello", "world");
    // // Expect equality.
    // EXPECT_EQ(7 * 6, 42);
}

void performSingleStepTest(const char* path){
    auto tb = std::make_shared<TestBus>();
    CPU cpu(tb);
    std::ifstream f(path);
    json data = json::parse(f);
    std::vector<CpuTransition> cases = parseSingleStepTest(data);
    f.close();

    size_t number = 0;
    for(const auto &testcase : cases){
        number++;
        tb->setTransactions(testcase.transactions);
        cpu.setRegisterState(testcase.initial);
        auto iInfo = cpu.getInstructionInPipeline();
        auto cpsr = cpu.printCPSR();
        auto mode = cpu.printMode();

        cpu.advanceCPUToNextValidState();

        auto stateF = cpu.getRegisterState();

        auto iStr = CPU::printInstructionType(iInfo.type);
        auto iCode = std::bitset<32>(iInfo.code).to_string();
        separateEveryN(iCode, 5, "\t");

        ASSERT_EQ(stateF, testcase.final)
            << "\n\nFehler in Testfall " << number << ":\n"
            << "Gelesene Instruktion: " << iStr << "\n"
            << "\t31\t27\t23\t19\t15\t11\t7\t3\n"
            << "Code: " << iCode << "\n"
            << "CPSR: " << cpsr << "\n"
            << "Mode: " << mode << "\n"
            << printDiff(testcase.initial, stateF, testcase.final)
            << printTransactions(testcase.transactions);

        ASSERT_FALSE(tb->hadError);

    }
}


// TEST(CPUTest, arm_bx){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_bx.json");
// }

// TEST(CPUTest, arm_b_bl){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_b_bl.json");
// }

// TEST(CPUTest, arm_cdp){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_cdp.json");
// }

TEST(CPUTest, arm_data_proc_immediate){
    performSingleStepTest("../test/ARM7TDMI/v1/arm_data_proc_immediate.json");
}

TEST(CPUTest, arm_data_proc_immediate_shift){
    performSingleStepTest("../test/ARM7TDMI/v1/arm_data_proc_immediate_shift.json");
}

TEST(CPUTest, arm_data_proc_register_shift){
    performSingleStepTest("../test/ARM7TDMI/v1/arm_data_proc_register_shift.json");
}

// TEST(CPUTest, arm_ldm_stm){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_ldm_stm.json");
// }

// TEST(CPUTest, arm_ldrh_strh){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_ldrh_strh.json");
// }

// TEST(CPUTest, arm_ldrsb_ldrsh){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_ldrsb_ldrsh.json");
// }

// TEST(CPUTest, arm_ldr_str_immediate_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_ldr_str_immediate_offset.json");
// }

// TEST(CPUTest, arm_ldr_str_register_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_ldr_str_register_offset.json");
// }

// TEST(CPUTest, arm_mcr_mrc){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_mcr_mrc.json");
// }

// TEST(CPUTest, arm_mrs){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_mrs.json");
// }

// TEST(CPUTest, arm_msr_imm){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_msr_imm.json");
// }

// TEST(CPUTest, arm_msr_reg){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_msr_reg.json");
// }

// TEST(CPUTest, arm_mull_mlal){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_mull_mlal.json");
// }

// TEST(CPUTest, arm_mul_mla){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_mul_mla.json");
// }

// TEST(CPUTest, arm_stc_ldc){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_stc_ldc.json");
// }

// TEST(CPUTest, arm_swi){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_swi.json");
// }

// TEST(CPUTest, arm_swp){
//     performSingleStepTest("../test/ARM7TDMI/v1/arm_swp.json");
// }

// TEST(CPUTest, thumb_add_cmp_mov_hi){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_add_cmp_mov_hi.json");
// }

// TEST(CPUTest, thumb_add_sp_or_pc){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_add_sp_or_pc.json");
// }

// TEST(CPUTest, thumb_add_sub){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_add_sub.json");
// }

// TEST(CPUTest, thumb_add_sub_sp){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_add_sub_sp.json");
// }

// TEST(CPUTest, thumb_b){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_b.json");
// }

// TEST(CPUTest, thumb_bcc){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_bcc.json");
// }

// TEST(CPUTest, thumb_bl_blx_prefix){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_bl_blx_prefix.json");
// }

// TEST(CPUTest, thumb_bl_suffix){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_bl_suffix.json");
// }

// TEST(CPUTest, thumb_bx){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_bx.json");
// }

// TEST(CPUTest, thumb_data_proc){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_data_proc.json");
// }

// TEST(CPUTest, thumb_ldm_stm){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldm_stm.json");
// }

// TEST(CPUTest, thumb_ldrb_strb_imm_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldrb_strb_imm_offset.json");
// }

// TEST(CPUTest, thumb_ldrh_strh_imm_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldrh_strh_imm_offset.json");
// }

// TEST(CPUTest, thumb_ldrh_strh_reg_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldrh_strh_reg_offset.json");
// }

// TEST(CPUTest, thumb_ldrsb_strb_reg_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldrsb_strb_reg_offset.json");
// }

// TEST(CPUTest, thumb_ldrsh_ldrsb_reg_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldrsh_ldrsb_reg_offset.json");
// }

// TEST(CPUTest, thumb_ldr_pc_rel){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldr_pc_rel.json");
// }

// TEST(CPUTest, thumb_ldr_str_imm_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldr_str_imm_offset.json");
// }

// TEST(CPUTest, thumb_ldr_str_reg_offset){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldr_str_reg_offset.json");
// }

// TEST(CPUTest, thumb_ldr_str_sp_rel){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_ldr_str_sp_rel.json");
// }

// TEST(CPUTest, thumb_lsl_lsr_asr){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_lsl_lsr_asr.json");
// }

// TEST(CPUTest, thumb_mov_cmp_add_sub){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_mov_cmp_add_sub.json");
// }

// TEST(CPUTest, thumb_push_pop){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_push_pop.json");
// }

// TEST(CPUTest, thumb_swi){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_swi.json");
// }

// TEST(CPUTest, thumb_undefined_bcc){
//     performSingleStepTest("../test/ARM7TDMI/v1/thumb_undefined_bcc.json");
// }

