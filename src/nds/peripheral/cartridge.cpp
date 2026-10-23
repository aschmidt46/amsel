#include "cartridge.h"
#include <iostream>
#include "framework/stringlib.h"
#include "../bus.h"
#include "../resources/database.h"
#include "framework/global.h"

using namespace nds;

void nds::Cartridge::startTransfer()
{
    // std::cout << "Start Transfer:\n";
    switch(getCMD()){
        case 0x9F:{
            // Dummy, sendet 0xFF
            oncePerCycle = false;
            currentMode = GameCardEventType_Dummy;
            remainingLength = 0x2000;
            romControl.state.dataWordStatus = 0;
            bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index=currentMode}});
            break;
        }
        case 0x00:{
            // Header
            std::cout << "sent header\n";
            romControl.state.dataWordStatus = 0;
            remainingLength = 0x200;
            offsetAddress = 0;
            currentMode = GameCardEventType_DataWord;
            oncePerCycle = false;
            bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index=currentMode}});
            break;
        }
        case 0x3C:{
            // Aktiviere KEY1
            std::cout << "act key1\n";
            oncePerCycle = false;
            currentMode = GameCardEventType_Dummy;
            remainingLength = 0;
            useKEY1 = true;
            romControl.state.dataWordStatus = 0;
            romControl.state.blockStartStatus = 0;
            gameCardBus4byteDataIn = 0xFFFFFFFF;
            // bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index=currentMode}});
            break;
        }
        case 0x4:{
            // Aktiviere KEY2
            std::cout << "act key2\n";
            oncePerCycle = false;
            currentMode = GameCardEventType_Dummy;
            remainingLength = 0x1000;
            romControl.state.dataWordStatus = 0;
            // romControl.state.blockStartStatus = 0;
            gameCardBus4byteDataIn = 0xFFFFFFFF;
            // bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index=currentMode}});
            bus->scheduler->scheduleEvent({.timePoint =1000, .type = NDSEVENT_GameCardStopBlock});
            break;
        }
        case 0xA:{
            // Aktiviere Main
            std::cout << "Main Data Mode\n";
            useKEY1 = false;
            oncePerCycle = false;
            currentMode = GameCardEventType_Dummy;
            remainingLength = 0x910;
            romControl.state.dataWordStatus = 0;
            romControl.state.blockStartStatus = 0;
            gameCardBus4byteDataIn = 0xFFFFFFFF;
            // bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index=currentMode}});
            bus->scheduler->scheduleEvent({.timePoint =1000, .type = NDSEVENT_GameCardStopBlock});
            break;
        }
        case 0xB7:{
            // std::cout << "Gamecard Read Command\n";
            // std::cout << getHex0x(gameCardBusCommandOut, 16) << "\n";
            // std::cout << romControl.getTranferSize() << "\n";
            romControl.state.dataWordStatus = 0;
            remainingLength = romControl.getTranferSize();
            offsetAddress = (gameCardBusCommandOut >> 24) & 0xFFFFFFFF;
            currentMode = GameCardEventType_DataWord;
            if(offsetAddress < 0x8000){
                offsetAddress = 0x8000 + (offsetAddress & 0x1FF);
            }
            // bus->setHalt();
            oncePerCycle = false;
            if(remainingLength > 0){
                bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index=currentMode}});
            }
            else{
                checkAndResend();
            }
            break;
        }
        case 0x90:
        case 0xB8:{
            // std::cout << "CHIP ID Command\n";
            romControl.state.dataWordStatus = 0;
            remainingLength = romControl.getTranferSize();
            currentMode = GameCardEventType_ChipID;
            oncePerCycle = false;
            if(remainingLength > 0){
                bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index = currentMode}});
            }
            else{
                checkAndResend();
            }
            break;
        }
        case 0x1:{
            std::cout << "CHIP ID KEY1 Command\n";
            romControl.state.dataWordStatus = 0;
            remainingLength = romControl.getTranferSize();
            currentMode = GameCardEventType_ChipID;
            oncePerCycle = false;
            if(remainingLength > 0){
                bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index = currentMode}});
            }
            else{
                checkAndResend();
            }
            break;
        }
        case 0x2:{
            romControl.state.dataWordStatus = 0;
            remainingLength = romControl.getTranferSize();
            currentMode = GameCardEventType_DataWord;
            uint64_t command = gameCardBusCommandOut;
            decrypt64((Byte*)&command);
            const Word block = (command >> 44) & 0xFFFF;
            offsetAddress = block * 0x1000;
            oncePerCycle = false;
            if(remainingLength > 0){
                bus->scheduler->scheduleEvent({.timePoint = 80 + (romControl.state.transferCLKRate ? 8u : 5u), .type = NDSEVENT_GameCardEvent, .args={.index = currentMode}});
            }
            else{
                checkAndResend();
            }
            break;
        }
        default:{
            std::cout << "Unknown Gamecard Command: " << getHex0x(getCMD(), 2) << "\n";
        }
    }
}

void nds::Cartridge::unselectDevices()
{
    if(eeprom != nullptr) eeprom->state = EEPROM_STATE_IDLE;
    if(flash != nullptr) flash->state = FLASH_IDLE;
}

void nds::Cartridge::startTransferSPI()
{
    AUXSPICNT.state.busy = 1;
    switch(romInfo.type){
        case SAVETYPE_NONE:{
            break;
        }
        case SAVETYPE_EEPROM:{
            eeprom->write(AUXSPIDATA);
            break;
        }
        case SAVETYPE_FLASH:{
            flash->write(AUXSPIDATA);
            break;
        }
    }
    bus->scheduler->scheduleEvent({.timePoint= 10, .type= NDSEVENT_AUXSPITransferComplete});
}

nds::Cartridge::Cartridge(SharedBus *bus, const std::vector<Byte> &cart, bool hle) : bus(bus), cartridge(cart)
{
    cartID = *(Word*)(cartridge.data() + 0xC);
    secureArea = cartridge.data() + 0x4000;
    bool found = false;

    for(size_t i = 0; i < romDB.size(); i++){
        if(cartID == romDB[i].id){
            romInfo = romDB[i];
            std::string saveType = romInfo.type == SAVETYPE_NONE ? "None" : (romInfo.type == SAVETYPE_EEPROM ? "EEPROM" : "FLASH");
            std::cout << "detected " << saveType << "\n";
            messageQueue.enqueue({.type = MT_INFO, .title = std::nullopt, .content = saveType});
            found = true;
            break;
        }
    }
    
    if(!found){
        messageQueue.enqueue({.type = MT_WARNING, .title = std::nullopt, .content = "Game not found in Database!"});
    }

    switch(romInfo.type){
        case SAVETYPE_NONE:{
            break;
        }
        case SAVETYPE_EEPROM:{
            eeprom = std::make_unique<EEPROM>(romInfo.size);
            break;
        }
        case SAVETYPE_FLASH:{
            flash = std::make_unique<Flash>(romInfo.size);
            break;
        }
    }

    if(!hle){ // Nur falls Cartridge bereits entschlüsselt ist!!
        if(*(Word*)secureArea == 0xE7FFDEFF && *(Word*)(secureArea + 4) == 0xE7FFDEFF){
            std::memcpy(secureArea, "encryObj", 8);
            initKeycode(cartID, 3, 8);
            for(size_t i = 0; i < 0x800; i += 8){
                encrypt64(secureArea + i);
            }
            initKeycode(cartID, 2, 8);
            encrypt64(secureArea);
        }
        else initKeycode(cartID, 2, 8);
    }
}

Byte nds::Cartridge::onRead(Word addr)
{
    if(addr == 0x40001A0){
        return AUXSPICNT.raw;
    }
    if(addr == 0x40001A1){
        return AUXSPICNT.raw >> 8;
    }
    if(addr == 0x40001A2){
        if(!AUXSPICNT.state.holdChipSelect && !AUXSPICNT.state.busy){
            std::cout << "Chip select pulled up\n";
            unselectDevices();
        }
        return AUXSPIDATA;
    }
    if(addr == 0x40001A4){
        return romControl.raw;
    }
    if(addr == 0x40001A5){
        return romControl.raw >> 8;
    }
    if(addr == 0x40001A6){
        return romControl.raw >> 16;
    }
    if(addr == 0x40001A7){
        return romControl.raw >> 24;
    }
    if(addr == 0x4100010){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn;
    }
    if(addr == 0x4100011){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn >> 8;
    }
    if(addr == 0x4100012){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn >> 16;
    }
    if(addr == 0x4100013){
        romControl.state.dataWordStatus = 0;
        checkAndResend();
        return gameCardBus4byteDataIn >> 24;
    }
    return 0;
}


void nds::Cartridge::onWrite(Word addr, Byte val)
{
    if(addr == 0x40001A0){
        AUXSPICNT.raw = (AUXSPICNT.raw & 0xFF00) | (val & 0b01000011);
    }
    else if(addr == 0x40001A1){
        AUXSPICNT.raw = (AUXSPICNT.raw & 0x00FF) | (HalfWord(val & 0b11100000) << 8);
        if(AUXSPICNT.state.ndsSlotEnable == 0) unselectDevices();
    }
    else if(addr == 0x40001A2){
        AUXSPIDATA = val;
        if(AUXSPICNT.state.busy == 0 && AUXSPICNT.state.ndsSlotEnable && AUXSPICNT.state.ndsSlotMode){
            startTransferSPI();
        }
        if(AUXSPICNT.state.busy == 0 && (!AUXSPICNT.state.ndsSlotMode || !AUXSPICNT.state.ndsSlotEnable)){
            unselectDevices();
        }
    }
    else if(addr == 0x40001A4){
        romControl.raw = (romControl.raw & 0xFFFFFF00) | (val);
    }
    else if(addr == 0x40001A5){
        romControl.raw = (romControl.raw & 0xFFFF00FF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x40001A6){
        romControl.raw = (romControl.raw & 0xFF00FFFF) | (HalfWord(val & 127u) << 16);
    }
    else if(addr == 0x40001A7){
        bool before = romControl.state.blockStartStatus > 0;
        romControl.raw = (romControl.raw & 0x00FFFFFF) | (HalfWord(val) << 24);
        bool after = romControl.state.blockStartStatus > 0;
        if(!before && after){
            startTransfer();
        }
    }
    else if(addr >= 0x40001A8 && addr < 0x40001B0){
        addr -= 0x40001A8;
        // std::cout << "ROMCMD write byte:" << addr << " sz:1 " << "data:" << getHex0x(val,2)<<"\n";
        gameCardBusCommandOut = (gameCardBusCommandOut & ~(uint64_t(0xFF) << (56 - addr * 8))) | (uint64_t(val) << (56 - addr * 8));
    }
}

void nds::Cartridge::checkAndResend()
{
    if(oncePerCycle) return;
    oncePerCycle = true;
    if(remainingLength > 0){
        bus->scheduler->scheduleEvent({.timePoint = romControl.state.transferCLKRate ? 8u : 5u, .type = NDSEVENT_GameCardEvent, .args={.index = currentMode}});
    }
    else{
        romControl.state.blockStartStatus = 0;
        romControl.state.dataWordStatus = 0;

        if(AUXSPICNT.state.transferReadyIRQ > 0){
            if(bus->canAccessGameCart(false)){
                bus->setIFSingleBus(19, true, false);
            }
            else{
                bus->setIFSingleBus(19, true, true);
            }
        }
    }
}

void nds::Cartridge::completeTransfer()
{
    AUXSPICNT.state.busy = 0;
    AUXSPIDATA = 0;

    switch(romInfo.type){
        case SAVETYPE_NONE:
            break;
        case SAVETYPE_EEPROM:
            AUXSPIDATA = eeprom->read();
            break;
        case SAVETYPE_FLASH:
            AUXSPIDATA = flash->read();
            break;
    }
    if(!AUXSPICNT.state.holdChipSelect) unselectDevices();
}

bool nds::Cartridge::canSave()
{
    return romInfo.type != SAVETYPE_NONE;
}

std::vector<Byte> nds::Cartridge::getSaveData()
{
    switch(romInfo.type){
        case SAVETYPE_NONE:{
            break;
        }
        case SAVETYPE_EEPROM:{
            return eeprom->getSave();
            break;
        }
        case SAVETYPE_FLASH:{
            return flash->getSave();
            break;
        }
    }
    return std::vector<Byte>();
}

size_t nds::Cartridge::getSaveSize()
{
    return romInfo.size;
}

void nds::Cartridge::loadSave(const std::vector<Byte> &saveData)
{
    switch(romInfo.type){
        case SAVETYPE_NONE:{
            break;
        }
        case SAVETYPE_EEPROM:{
            eeprom = std::make_unique<EEPROM>(romInfo.size);
            eeprom->loadSave(saveData);
            break;
        }
        case SAVETYPE_FLASH:{
            flash = std::make_unique<Flash>(romInfo.size);
            flash->loadSave(saveData);
            break;
        }
    }
}

void nds::Cartridge::cardReply(int index)
{
    romControl.state.dataWordStatus = 1;
    remainingLength -= 4;
    oncePerCycle = false;
    switch((GameCardEventType)index){
        case GameCardEventType_DataWord:
            sendDataWord();
            break;
        case GameCardEventType_ChipID:
            sendChipID();
            break;
        case GameCardEventType_Dummy:
            sendDummy();
            break;
        case GameCardEventType_Dummy_Chip:
            if(remainingLength >= 0x16F0){
                sendDummy();
            }
            else{
                sendChipID();
            }
            break;
        case GameCardEventType_SecureBlock:
            sendSecureBlock();
            break;
        default: break;
    }
}



void nds::Cartridge::sendDataWord()
{
    gameCardBus4byteDataIn = cartridge[offsetAddress] | (Word(cartridge[offsetAddress + 1]) << 8) | (Word(cartridge[offsetAddress + 2]) << 16) | (Word(cartridge[offsetAddress + 3]) << 24);
    offsetAddress += 4;
    bus->onDataWordArrived();
}

void nds::Cartridge::sendChipID()
{
    gameCardBus4byteDataIn = bus->chipID;
    bus->onDataWordArrived();
}

void nds::Cartridge::sendDummy()
{
    gameCardBus4byteDataIn = 0xFFFFFFFF;
    bus->onDataWordArrived();
}

void nds::Cartridge::sendSecureBlock()
{

}
