#pragma once
#include "../bus_types.h"
#include <algorithm>

namespace nds{

    class CPU;

    union CP15_CONTROL_REG_T{
        struct{
            Word mmuPuEnable : 1; // LSB
            Word alignmentFaultCheck : 1;
            Word dataUnifiedCache : 1;
            Word writeBuffer : 1;
            Word exceotionHandling : 1;
            Word addressfaults26bit : 1;
            Word abortModel : 1;
            Word endian : 1;
            Word systemProtectionBit : 1;
            Word romProtectionBit : 1;
            Word implementationDefined : 1;
            Word branchPrediction : 1;
            Word instructionCache : 1;
            Word exceptionVectors : 1;
            Word cacheReplacement : 1;
            Word preArmV5Mode : 1;
            Word dtcmEnable : 1;
            Word dtcmLoadMode : 1;
            Word itcmEnable : 1;
            Word itcmLoadMode : 1;
            Word _reserved1 : 2;
            Word unalignedAccess : 1;
            Word extendedPageTable : 1;
            Word _reserved2 : 1;
            Word cpsrEOnExceptions : 1;
            Word _reserved3 : 1;
            Word fiqBehaviour : 1;
            Word texRemapBit : 1;
            Word forceAP : 1;
            Word _reserved4 : 2;
        } state;
        Word raw;

        static Word getWriteMask(){
            return 0b00000000000011111111000010000101;
            // "On NDS ARM9, bit0,2,7,12..19 are R/W, bit3..6 are always set, all other bits are always zero."
        }
    };

    union TCM_SIZE_T{
        struct{
            Word _reserved1 : 1; // LSB
            Word virtualSize : 5;
            Word _reserved2 : 6;
            Word regionBase : 20;
        } state;
        Word raw;

        static Word getWriteMask(){
            return 0xFFFFF03E;
        }
    };

    class CP15{
        private:
        const Word mainID = 0x41059461;
        const Word cacheType = 0x0F0D2112;
        const Word tcmSize = 0x00140180;
        CP15_CONTROL_REG_T controlReg = {.raw = 0x12078};

        //PU
        Word cachabilityDataUnified = 0x42;
        Word cachabiltiyInstruction = 0x42;
        Word cacheWriteBufferabilityData = 0x2;
        
        Word accessPermissionDataUnified = 0x5545;
        Word accessPermissionInstruction = 0x1405;
        Word extendedAccessPermissionDataUnified = 0x15111011;
        Word extendedAccessPermissionInstruction = 0x5100011;
        Word protectionUnitAndMirror[8] = {0x4000033, 0x200002B, 0x0, 0x8000035, 0x300001B, 0x0, 0xFFFF001D, 0x27FF017};

        // TCM
        TCM_SIZE_T dtcmSize = {.raw = 0x0300000A};
        TCM_SIZE_T itcmSize = {.raw = 0};

        Word processID = 0;
        Word traceProcessID = 0;
        CPU* cpu;


        public:
        CP15(){
            itcmSize.state.virtualSize = 16;
        }
        CP15(CPU* cpu) : CP15(){
            this->cpu = cpu;
        }
        void setCPU(CPU* cpu){
            this->cpu = cpu;
        }
        void MCR(Word armRegister, Word Cn, Word Cm, Word cp);
        Word MRC(Word Cn, Word Cm, Word cp);
        inline bool itcmIsEnabled() const{
            return controlReg.state.itcmEnable;
        };
        inline bool itcmIsLoadMode() const{
            return controlReg.state.itcmLoadMode;
        };
        inline bool dtcmIsEnabled() const{
            return controlReg.state.dtcmEnable;
        };
        inline bool dtcmIsLoadMode() const{
            return controlReg.state.dtcmLoadMode;
        };
        inline Word itcmRegion() const{
            const Word vSize = 512u << std::clamp(itcmSize.state.virtualSize, 3u, 23u);
            return vSize;
        };
        inline std::pair<Word, Word> dtcmRegion() const{
            const Word vSize = 512u << std::clamp(dtcmSize.state.virtualSize, 3u, 23u);
            const Word base = dtcmSize.state.regionBase << 12;
            return {base, base + vSize};
        };

        inline Word exceptionBase(){
            return controlReg.state.exceptionVectors == 0 ? 0x0 : 0xFFFF0000;
        }

        inline bool setEBitOnExceptions(){
            return controlReg.state.cpsrEOnExceptions == 1;
        }

        void renderTCM();
    };
};
