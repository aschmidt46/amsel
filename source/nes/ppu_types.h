#pragma once
#include <cstdint>

struct loopyReg{
    uint16_t raw = 0; // eigentlich 15bit

    unsigned int getFineY(){
        return (raw & 0b111000000000000u) >> 12;
    };
    unsigned int getNametableSelect(){
        return (raw & 0b000110000000000u) >> 10;
    };

    // Wiederholung
    bool getNametableX(){
        return (raw & 0b000010000000000u);
    };
    bool getNametableY(){
        return (raw & 0b000100000000000u);
    };


    unsigned int getCoarseY(){
        return (raw & 0b000001111100000u) >> 5;
    };
    unsigned int getCoarseX(){
        return (raw & 0b000000000011111u);
    };

    void setFineY(unsigned int value){
        raw = (raw & 0b000111111111111u) | (value << 12);
    };
    void setNametableSelect(unsigned int value){
        raw = (raw & 0b111001111111111u) | (value << 10);
    };
    void setCoarseY(unsigned int value){
        raw = (raw & 0b111110000011111u) | (value << 5);
    };
    void setCoarseX(unsigned int value){
        raw = (raw & 0b111111111100000u) | (value);
    };


    void setNametableX(bool value){
        raw = (raw & 0b111101111111111u) | (value << 10);
    };
    void setNametableY(bool value){
        raw = (raw & 0b111011111111111u) | (value << 11);
    };
};


struct ctrlReg {
    uint8_t raw = 0;

    bool getNametableX(){
        return raw & 0b1u;
    };
    bool getNametableY(){
        return raw & 0b10u;
    };
    bool getIncrementMode(){
        return raw & 0b100u;
    };
    bool getPatternSprite(){
        return raw & 0b1000u;
    };
    bool getPatternBackground(){
        return raw & 0b10000u;
    };
    // 1 - 8x16, 0 - 8x8
    bool getSpriteSize(){
        return raw & 0b100000u;
    };
    bool getUnused(){
        return raw & 0b1000000u;
    };
    bool getEnableNMI(){
        return raw & 0b10000000u;
    };

    void setNametableX(bool value){
        raw = (raw & 0b11111110) | value;
    };
    void setNametableY(bool value){
        raw = (raw & 0b11111101) | (value << 1);
    };
    void setIncrementMode(bool value){
        raw = (raw & 0b11111011) | (value << 2);
    };
    void setPatternSprite(bool value){
        raw = (raw & 0b11110111) | (value << 3);
    };
    void setPatternBackground(bool value){
        raw = (raw & 0b11101111) | (value << 4);
    };
    void setSpriteSize(bool value){
        raw = (raw & 0b11011111) | (value << 5);
    };
    void setUnused(bool value){
        raw = (raw & 0b10111111) | (value << 6);
    };
    void setEnableNMI(bool value){
        raw = (raw & 0b01111111) | (value << 7);
    };
};

struct maskReg {
    uint8_t raw = 0;

    bool getGrayScale(){
        return raw & 0b1u;
    };
    bool getRenderBackgroundLeft(){
        return raw & 0b10u;
    };
    bool getRenderSpritesLeft(){
        return raw & 0b100u;
    };
    bool getRenderBackground(){
        return raw & 0b1000u;
    };
    bool getRenderSprites(){
        return raw & 0b10000u;
    };
    bool getEnhanceRed(){
        return raw & 0b100000u;
    };
    bool getEnhanceGreen(){
        return raw & 0b1000000u;
    };
    bool getEnhanceBlue(){
        return raw & 0b10000000u;
    };

    void setGrayscale(bool value){
        raw = (raw & 0b11111110) | value;
    };
    void setRenderBackgroundLeft(bool value){
        raw = (raw & 0b11111101) | (value << 1);
    };
    void setRenderSpritesLeft(bool value){
        raw = (raw & 0b11111011) | (value << 2);
    };
    void setRenderBackground(bool value){
        raw = (raw & 0b11110111) | (value << 3);
    };
    void setRenderSprites(bool value){
        raw = (raw & 0b11101111) | (value << 4);
    };
    void setEnhanceRed(bool value){
        raw = (raw & 0b11011111) | (value << 5);
    };
    void setEnhanceGreen(bool value){
        raw = (raw & 0b10111111) | (value << 6);
    };
    void setEnhanceBlue(bool value){
        raw = (raw & 0b01111111) | (value << 7);
    };
};

struct statusReg {
    uint8_t raw = 0;

    
    bool getSpriteOverflow(){
        return raw & 0b100000u;
    };
    bool getSpriteZeroHit(){
        return raw & 0b1000000u;
    };
    bool getVerticalBlank(){
        return raw & 0b10000000u;
    };

    
    void setSpriteOverflow(bool value){
        raw = (raw & 0b11011111) | (value << 5);
    };
    void setSpriteZeroHit(bool value){
        raw = (raw & 0b10111111) | (value << 6);
    };
    void setVerticalBlank(bool value){
        raw = (raw & 0b01111111) | (value << 7);
    };
};
