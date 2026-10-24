#include "cp15.h"
#include <iostream>
#include "framework/stringlib.h"
// #ifdef BUILD_DESKTOP
#include <imgui.h>
// #endif
#include <bitset>
#include "arm946e-s.h"

using namespace nds;

void nds::CP15::MCR(Word val, Word Cn, Word Cm, Word cp)
{
    if(Cn == 1 && Cm == 0 && cp == 0){
        const Word m = CP15_CONTROL_REG_T::getWriteMask();
        controlReg.raw = (controlReg.raw & ~m) | (val & m);
    }

    else if(Cn == 2 && Cm == 0 && cp == 0){
        cachabilityDataUnified = val & 0xFF;
    }
    else if(Cn == 2 && Cm == 0 && cp == 1){
        cachabiltiyInstruction = val & 0xFF;
    }
    else if(Cn == 3 && Cm == 0 && cp == 0){
        cacheWriteBufferabilityData = val & 0xFF;
    }

    else if(Cn == 5 && Cm == 0 && cp == 0){
        accessPermissionDataUnified = val & 0xFFFF;
    }
    else if(Cn == 5 && Cm == 0 && cp == 1){
        accessPermissionInstruction = val & 0xFFFF;
    }
    else if(Cn == 5 && Cm == 0 && cp == 2){
        extendedAccessPermissionDataUnified = val;
    }
    else if(Cn == 5 && Cm == 0 && cp == 3){
        extendedAccessPermissionInstruction = val;
    }
    else if(Cn == 6 && Cm < 8 && cp <= 1){
        protectionUnitAndMirror[Cm] = val;
    }

    else if(Cn == 9 && Cm == 1 && cp == 0){
        Word m = TCM_SIZE_T::getWriteMask();
        dtcmSize.raw = (dtcmSize.raw & ~m) | (val & m);
        calculateDTCMRegion();
    }
    else if(Cn == 9 && Cm == 1 && cp == 1){
        Word m = TCM_SIZE_T::getWriteMask();
        itcmSize.raw = (itcmSize.raw & ~m) | (val & m);
        calculateITCMRegion();
    }

    else if(Cn == 13 && Cm == 0 && cp == 0){
        processID = (processID & 0x00FFFFFF) | (val & 0xFF000000);
    }
    else if(Cn == 13 && Cm <= 1 && cp == 1){
        traceProcessID = val;
    }
    else if(Cn == 7){
        // Cache

        // Halt
        if((Cm == 0 && cp == 4) || (Cm == 8 && cp == 2)){
            cpu->enterHaltMode();
        }
    }
    else{
        std::cout << "MCR unhandled: C" << Cn << ", C" << Cm << ", " << cp << "\n";
    }
}

Word nds::CP15::MRC(Word Cn, Word Cm, Word cp)
{
    if(Cn == 0 && Cm == 0 && cp == 0){
        return mainID;
    }
    if(Cn == 0 && Cm == 0 && cp == 1){
        return cacheType;
    }
    if(Cn == 0 && Cm == 0 && cp == 2){
        return tcmSize;
    }
    if(Cn == 0 && Cm == 0 && cp >= 3 && cp <= 7){
        return mainID;
    }
    if(Cn == 1 && Cm == 0 && cp == 0){
        return controlReg.raw;
    }

    // PU
    if(Cn == 2 && Cm == 0 && cp == 0){
        return cachabilityDataUnified;
    }
    if(Cn == 2 && Cm == 0 && cp == 1){
        return cachabiltiyInstruction;
    }
    if(Cn == 3 && Cm == 0 && cp == 0){
        return cacheWriteBufferabilityData;
    }

    if(Cn == 5 && Cm == 0 && cp == 0){
        return accessPermissionDataUnified;
    }
    if(Cn == 5 && Cm == 0 && cp == 1){
        return accessPermissionInstruction;
    }
    if(Cn == 5 && Cm == 0 && cp == 2){
        return extendedAccessPermissionDataUnified;
    }
    if(Cn == 5 && Cm == 0 && cp == 3){
        return extendedAccessPermissionInstruction;
    }
    if(Cn == 6 && Cm < 8 && cp <= 1){
        return protectionUnitAndMirror[Cm];
    }

    // TCM
    if(Cn == 9 && Cm == 1 && cp == 0){
        return dtcmSize.raw;
    }
    if(Cn == 9 && Cm == 1 && cp == 1){
        return itcmSize.raw;
    }

    if(Cn == 13 && Cm == 0 && cp == 0){
        return processID;
    }
    if(Cn == 13 && Cm <= 1 && cp == 1){
        return traceProcessID;
    }
    std::cout << "MRC unhandled: C" << Cn << ", C" << Cm << ", " << cp << "\n";
    return 0;
}

void nds::CP15::renderTCM()
{
    //#ifdef BUILD_DESKTOP
        ImGui::Text("ITCM End: ");
        ImGui::SameLine();
        ImGui::Text(getHex0x(itcmEnd, 8).c_str());
        ImGui::Text((std::string("Is Enabled? ") + (itcmIsEnabled() ? "Yes" : "No")).c_str());
        ImGui::Text((std::string("Is Load Mode? ") + (itcmIsLoadMode() ? "Yes" : "No")).c_str());

        ImGui::Text("");

        ImGui::Text("DTCM Region: ");
        ImGui::SameLine();
        ImGui::Text((getHex0x(dtcmBase, 8) + "---" + getHex0x(dtcmEnd, 8)).c_str());
        ImGui::Text((std::string("Is Enabled? ") + (dtcmIsEnabled() ? "Yes" : "No")).c_str());
        ImGui::Text((std::string("Is Load Mode? ") + (dtcmIsLoadMode() ? "Yes" : "No")).c_str());

        ImGui::Text("CP15 Control Reg: ");
        ImGui::Text("Exception Base: ");
        ImGui::SameLine();
        ImGui::Text(std::to_string(exceptionBase()).c_str());
    //#endif
}
