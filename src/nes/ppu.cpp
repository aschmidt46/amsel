#include "ppu.h"
#include <iostream>
#include <cstring>
#include <format>
#include <algorithm>


void Ppu::clock()
{
	// NMI Suppression
	if(VBLWasSetThisCycle >= 0){
		if(suppressNMI){
			pullNMIIn = -1;
			suppressNMI = false;
		}
		VBLWasSetThisCycle--;
	}

	// Renderer fortschalten
	scanline = timings[ppuTiming].second.scanline;
	cycle = timings[ppuTiming].second.cycle;

	if(toggleBackgroundRenderIn>0){
		toggleBackgroundRenderIn--;
	}
	if(toggleSpriteRenderIn>0){
		toggleSpriteRenderIn--;
	}
	if(pullNMIIn>0){
		pullNMIIn--;
	}

	if(toggleBackgroundRenderIn==0){
		toggleBackgroundRenderIn = -1;
		PPUMASK.setRenderBackground(!PPUMASK.getRenderBackground());
	}
	if(toggleSpriteRenderIn==0){
		toggleSpriteRenderIn = -1;
		PPUMASK.setRenderSprites(!PPUMASK.getRenderSprites());
	}
	if(pullNMIIn==0){
		pullNMIIn = -1;
		mapper->pullNMI();
	}

	// Skip bei gerade Frames (Ende vom ungeraden Frame)
	if(!unevenFrame && PPUMASK.getRenderBackground()){
		if(scanline == 0 && cycle == 0){
			ppuTiming++;
			scanline = timings[ppuTiming].second.scanline;
			cycle = timings[ppuTiming].second.cycle;
		}
	}

	for(auto &cmd : timings[ppuTiming].first){
		(this->*cmd)();
	}
	ppuTiming++;
	if(ppuTiming >= numDots){
		ppuTiming = 0;
		swapBuffers();
		frameReady = true;
		unevenFrame = !unevenFrame;
	}
}

void Ppu::setPixel(int x, int y, uint32_t c)
{
    int index = (x + 256 * y);//alter Index: (3*x) + (3*256*y);
	if(x > 255 || y > 239 || x < 0 || y < 0){
		//std::cout << "Pixel out of bounds: " << "x: " << x << ", y: " << y << std::endl;
		return;
	}
    pixelBuffer.data()[index] = c;
}

void Ppu::swapBuffers()
{
	std::swap(pixelBuffer, backBuffer);
}

void Ppu::writeRegister(uint8_t *reg, uint8_t val)
{
    if(reg==(uint8_t*)&PPUCTRL){
		bool canTriggerNMI = false;
		if(!PPUCTRL.getEnableNMI())
			canTriggerNMI = true;
        // Wert schreiben
        PPUCTRL.raw = val;
		if(PPUSTATUS.getVerticalBlank() && canTriggerNMI && PPUCTRL.getEnableNMI()){
			if(pullNMIIn < 0)
				pullNMIIn = 14;
		}
        // Nametable select
        t.setNametableX(PPUCTRL.getNametableX());
        t.setNametableY(PPUCTRL.getNametableY());

    }
    else if(reg==(uint8_t*)&PPUMASK){
		bool spriteRender = PPUMASK.getRenderSprites();
		bool backgroundRender = PPUMASK.getRenderBackground();
		PPUMASK.raw = val;
		// "Toggling rendering takes effect approximately 3-4 dots after the write. This delay is required by Battletoads to avoid a crash." (NESDEV Wiki)
		if(PPUMASK.getRenderSprites() != spriteRender){
			PPUMASK.setRenderSprites(spriteRender);
			toggleSpriteRenderIn = 4;
		}
		if(PPUMASK.getRenderBackground() != backgroundRender){
			PPUMASK.setRenderBackground(backgroundRender);
			toggleBackgroundRenderIn = 4;
		}
    }
    else if(reg==&OAMADDR){
        OAMADDR = val;
    }
    else if(reg==&OAMDATA){
        if(blanking){
            pOAM[OAMADDR++] = val;
        }
    }
    else if(reg==&PPUSCROLL){
        if(!w){
            fineX = val & 0x07;
            t.setCoarseX(val >> 3);
            w = true;
        }
        else{
            t.setFineY(val & 0x07);
            t.setCoarseY(val >> 3);
            w = false;
        }
    }
    else if(reg==&PPUADDR){
        if(!w){
            t.raw = (uint16_t)((val & 0x3F) << 8) | (t.raw & 0x00FF);
            w = true;
        }
        else{
            t.raw = (t.raw & 0xFF00) | val;
            v = t;
            w = false;
        }
    }
    else if(reg==&PPUDATA){

        mapper->writeVRAM((uint8_t*)(uintptr_t)v.raw, val);
        v.raw += PPUCTRL.getIncrementMode() ? 32 : 1;
    }
}

uint8_t Ppu::readRegister(uint8_t *reg)
{
    uint8_t tmp;

	if(reg == (uint8_t*)&PPUSTATUS){
		if(VBLWasSetThisCycle >= 0){
			suppressNMI = true;
		}
		w = false;

        uint8_t value = PPUSTATUS.raw;
        // Clear Vblank
        PPUSTATUS.setVerticalBlank(false);
		// Ein Zyklus vor VBlank
		if(scanline==241 && cycle==0){
			suppressVBLThisFrame = true;
		}
		return value;
    }
	if(reg == &OAMDATA){
		if(scanline >= -1 && scanline <= 239 && cycle >= 0 && cycle < 64){
			return 0xFF;
		}
		return pOAM[OAMADDR];
    }
	if(reg == &PPUDATA){
		// Verzögertes Lesen von VRAM
		tmp = vramReadBuffer;
		vramReadBuffer = mapper->readVRAM((uint8_t*)(uintptr_t)v.raw);
		v.raw += PPUCTRL.getIncrementMode() ? 32 : 1;
        // Palette sofort lesen
		if (v.raw >= 0x3F00)
			tmp = vramReadBuffer;
		return tmp;
    }
	return 0;
}

void Ppu::fillTimings()
{
	timings.clear();
	int i = -1;
	for(int scanline = -1; scanline <= 260; scanline++){
		for(int cycle = 0; cycle <= 340; cycle++){
			i++;
			timings.push_back({std::vector<ppuCmd>(), {.scanline = scanline, .cycle = cycle}});

			if (scanline == -1 && cycle == 1){
				timings[i].first.push_back(&Ppu::clearFlags);
			}

			// sichtbare
			if(scanline <= 239){
				if(cycle > 0 && (cycle <= 260 || cycle >= 321)){
					if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338)){
						timings[i].first.push_back(&Ppu::updateShifters);

						switch(cycle % 8){
							case 0:
								timings[i].first.push_back(&Ppu::incrementX);
								break;
							case 1:
								timings[i].first.push_back(&Ppu::loadBackgroundShifters);
								timings[i].first.push_back(&Ppu::readNTByte);
								break;
							case 3:
								timings[i].first.push_back(&Ppu::readATByte);
								break;
							case 5:
								timings[i].first.push_back(&Ppu::readCHRByteLow);
								break;
							case 7:
								timings[i].first.push_back(&Ppu::readCHRByteHigh);
								break;
						}
					}
				}

				if(cycle == 256){
					timings[i].first.push_back(&Ppu::incrementY);
				}

				if(cycle == 257){
					timings[i].first.push_back(&Ppu::loadBackgroundShifters);
					timings[i].first.push_back(&Ppu::resetX);
				}

				if(scanline == -1 && (cycle >= 280 && cycle <= 304)){
					timings[i].first.push_back(&Ppu::resetY);
				}

				if (cycle == 337 || cycle == 339){
					timings[i].first.push_back(&Ppu::readNTByte);
				}
				
				if(cycle==257 && scanline >= 0){
					timings[i].first.push_back(&Ppu::evaluateSprites);
				}

				// Mapper4/MMC3 IRQ
				if(cycle==260){
					timings[i].first.push_back(&Ppu::maybeRiseA12);
				}
			}

			if(cycle == 340){
				timings[i].first.push_back(&Ppu::setSpriteShifters);
			}

			// v blank
			if(scanline == 241 && cycle == 1){
				timings[i].first.push_back(&Ppu::pullNMI);
			}


			timings[i].first.push_back(&Ppu::renderPixel);
		}
	}
}

void Ppu::incrementX()
{
	if (PPUMASK.getRenderBackground() || PPUMASK.getRenderSprites()){
		if (v.getCoarseX() == 31){
			v.setCoarseX(0);
			v.setNametableX(!v.getNametableX());
		}
		else{
			v.setCoarseX(v.getCoarseX()+1);
		}
	}
}

void Ppu::incrementY()
{
	if (PPUMASK.getRenderBackground() || PPUMASK.getRenderSprites()){
		if (v.getFineY() < 7){
			v.setFineY(v.getFineY()+1);
		}
		else{
			v.setFineY(0);
			if (v.getCoarseY() == 29){
				v.setCoarseY(0);
				v.setNametableY(!v.getNametableY());
			}
			else if (v.getCoarseY() == 31){
				v.setCoarseY(0);
			}
			else{
				v.setCoarseY(v.getCoarseY()+1);
			}
		}
	}
}

void Ppu::resetX()
{
	if (PPUMASK.getRenderBackground() || PPUMASK.getRenderSprites()){
		v.setNametableX(t.getNametableX());
		v.setCoarseX(t.getCoarseX());
	}
}

void Ppu::resetY()
{
	if (PPUMASK.getRenderBackground() || PPUMASK.getRenderSprites()){
		v.setFineY(t.getFineY());
		v.setNametableY(t.getNametableY());
		v.setCoarseY(t.getCoarseY());
	}
}

void Ppu::loadBackgroundShifters()
{
	shifterCHRLow = (shifterCHRLow & 0xFF00) | nextTileCHRLow;
	shifterCHRHigh = (shifterCHRHigh & 0xFF00) | nextTileCHRHigh;
	shifterATLow  = (shifterATLow & 0xFF00) | ((nextTileATByte & 0b01) ? 0xFF : 0);
	shifterATHigh  = (shifterATHigh & 0xFF00) | ((nextTileATByte & 0b10) ? 0xFF : 0);
}

void Ppu::updateShifters()
{
	if (PPUMASK.getRenderBackground()){
		shifterCHRLow <<= 1;
		shifterCHRHigh <<= 1;
		shifterATLow <<= 1;
		shifterATHigh <<= 1;
	}
	if(PPUMASK.getRenderSprites() && cycle >= 1 && cycle < 258){
		for(int i = 0; i < spriteCount; i++){
			if(spriteCount > 8) break;
			if(secondaryOAM[i].xPos > 0){
				secondaryOAM[i].xPos--;
			}
			else{
				spriteShifterCHRLow[i] <<= 1;
				spriteShifterCHRHigh[i] <<= 1;
			}
		}
	}
}

void Ppu::clearFlags()
{
	PPUSTATUS.setVerticalBlank(false);
	PPUSTATUS.setSpriteOverflow(false);
	PPUSTATUS.setSpriteZeroHit(false);

	for(int i = 0; i < 8; i++){
		spriteShifterCHRLow[i] = 0;
		spriteShifterCHRHigh[i] = 0;
	}
}

void Ppu::readNTByte()
{
	nextTileNTByte = mapper->readVRAM((uint8_t*)(uintptr_t)(0x2000 | (v.raw & 0x0FFF)));
}

void Ppu::readATByte()
{                                                                    // nametable xy      coarse y                coarse x
	nextTileATByte = mapper->readVRAM((uint8_t*)(uintptr_t)(0x23C0 | (v.raw & 0x0C00) | ((v.raw >> 4) & 0x38) | ((v.raw >> 2) & 7)));			
	if (v.getCoarseY() & 2) nextTileATByte >>= 4;
	if (v.getCoarseX() & 2) nextTileATByte >>= 2;
	nextTileATByte &= 3;
}

void Ppu::readCHRByteLow()
{
	nextTileCHRLow = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                             (PPUCTRL.getPatternBackground() << 12) // 0x1000
					                       + ((uint16_t)nextTileNTByte << 4) 
					                       + (v.getFineY()) + 0));
}

void Ppu::readCHRByteHigh()
{
	nextTileCHRHigh = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                             (PPUCTRL.getPatternBackground() << 12)
					                       + ((uint16_t)nextTileNTByte << 4)
					                       + (v.getFineY()) + 8));
}

void Ppu::evaluateSprites()
{
	for(size_t i = 0; i < 8; i++){
		secondaryOAM[i] = OAMSprite{};
	}
	spriteCount = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		spriteShifterCHRLow[i] = 0;
		spriteShifterCHRHigh[i] = 0;
	}
	uint8_t iOAM = 0;
	spriteZeroHitPossible = false;
	while(iOAM < 64 && spriteCount < 9){
		int16_t diff = ((int16_t)scanline - (int16_t)OAM[iOAM].yPos);
		if(diff >= 0 && diff < (PPUCTRL.getSpriteSize() ? 16 : 8)){
			if(spriteCount < 8){
				if(iOAM == 0){
					spriteZeroHitPossible = true;
				}
				memcpy(&secondaryOAM[spriteCount], &OAM[iOAM], sizeof(OAMSprite));
				spriteCount++;
			}
		}
		iOAM++;
	}
	PPUSTATUS.setSpriteOverflow(spriteCount > 8);
}

void Ppu::setSpriteShifters()
{
	for(uint8_t i = 0; i < spriteCount; i++){
		uint8_t spriteCHRLow, spriteCHRHigh;
		uint16_t spriteAddrLow, spriteAddrHigh;
		int invert = secondaryOAM[i].attributes & 0x80 ? -1 : 1; // vertikal umgedreht
		int invertOffset = secondaryOAM[i].attributes & 0x80 ? 7 : 0;
		if(!PPUCTRL.getSpriteSize()){
			// 8x8 Modus
			spriteAddrLow = (PPUCTRL.getPatternSprite() << 12) // Pattern Table wählen
							  | (secondaryOAM[i].tileIndex << 4) // Tile wählen
							  | (invertOffset + invert * (scanline - secondaryOAM[i].yPos)); // Position im Tile
		}
		else{
			// 8x16
			uint8_t indexOffset = scanline - secondaryOAM[i].yPos >= 8 ? 1 : 0; // untere hälfte / obere  hälfte
			spriteAddrLow = ((secondaryOAM[i].tileIndex & 1) << 12)
								  | (((secondaryOAM[i].tileIndex & 254u) + indexOffset) << 4)
								  | (invertOffset - invert * ((scanline - secondaryOAM[i].yPos) & 7));
		}
		spriteAddrHigh = spriteAddrLow + 8;
		spriteCHRLow = mapper->readVRAM((uint8_t*)(uintptr_t)spriteAddrLow);
		spriteCHRHigh = mapper->readVRAM((uint8_t*)(uintptr_t)spriteAddrHigh);
		// Flip horizontal
		if(secondaryOAM[i].attributes & 0x40){
			auto reverse = [](uint8_t b){
				b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
				b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
				b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
				return b;
			};
			spriteCHRLow = reverse(spriteCHRLow);
			spriteCHRHigh = reverse(spriteCHRHigh);
		}
		spriteShifterCHRLow[i] = spriteCHRLow;
		spriteShifterCHRHigh[i] = spriteCHRHigh;
	}
}

void Ppu::pullNMI()
{
	if(!suppressVBLThisFrame){
		PPUSTATUS.setVerticalBlank(true);
		VBLWasSetThisCycle = 1;
		if (PPUCTRL.getEnableNMI())
			// Experimentell ergeben aus der blargg 5-nmi-timing testrom
			pullNMIIn = 14;
	}
	else suppressVBLThisFrame = false;
}

void Ppu::renderPixel()
{
	//Pixelwert und AT-Palette
	uint8_t backgroundPixelValue = 0;
	uint8_t backgroundPaletteValue = 0;

	// Hintergrund-Evaluierung
	if (PPUMASK.getRenderBackground()){
		// Verschiebung wählt das maskierte Bit des aktuellen Pixels
		uint16_t fineXMux = 0x8000 >> fineX;

		// Aus den beiden Pattern Table Bitplanes wird der Pixel gewählt
		bool chrLow = shifterCHRLow & fineXMux;
		bool chrHigh = shifterCHRHigh & fineXMux;
		backgroundPixelValue = chrLow + (chrHigh << 1);

		// Gleiches für Palette (Hintergrund immer untere Paletten)
		bool atLow = shifterATLow & fineXMux;
		bool atHigh = shifterATHigh & fineXMux;

		backgroundPaletteValue = atLow + (atHigh << 1);
	}

	uint8_t spritePixelValue = 0;
	uint8_t spritePaletteValue = 0;
	bool spritePriority = false;

	spriteZeroBeingRendered = false;
	if (PPUMASK.getRenderSprites()){
		for(uint8_t i = 0; i < spriteCount; i++){
			if(secondaryOAM[i].xPos == 0){
				bool spritePixelLow = spriteShifterCHRLow[i]  & 0b10000000;
				bool spritePixelHigh  = spriteShifterCHRHigh[i] & 0b10000000;
				spritePixelValue = spritePixelLow + (spritePixelHigh << 1);

				spritePaletteValue = (secondaryOAM[i].attributes & 0b11) + 0b100;
				spritePriority = !(secondaryOAM[i].attributes & 0b100000);

				if(spritePixelValue != 0){
					if(i==0){
						spriteZeroBeingRendered = true;
					}
					break;
				}
			}
		}
	}

	if(!PPUMASK.getRenderBackgroundLeft() && cycle >= 1 && cycle <=9){
		backgroundPixelValue = 0;
		backgroundPaletteValue = 0;
	}

	if(!PPUMASK.getRenderSpritesLeft() && cycle >= 1 && cycle <=9){
		spritePixelValue = 0;
		spritePaletteValue= 0;
	}



	uint8_t pixel = 0;
	uint8_t palette = 0;
	if(!backgroundPixelValue && spritePixelValue){
		// hintergrund transparent
		pixel = spritePixelValue;
		palette = spritePaletteValue;
	}
	else if(backgroundPixelValue && !spritePixelValue){
		// vordergrund transparent
		pixel = backgroundPixelValue;
		palette = backgroundPaletteValue;
	}
	else if(backgroundPixelValue && spritePixelValue){
		if(spritePriority){
			pixel = spritePixelValue;
			palette = spritePaletteValue;
		}
		else{
			pixel = backgroundPixelValue;
			palette = backgroundPaletteValue;
		}

		// Sprite Zero Hit
		if(spriteZeroHitPossible && spriteZeroBeingRendered && backgroundPixelValue > 0 && spritePixelValue > 0){
			if(PPUMASK.getRenderSprites() && PPUMASK.getRenderBackground()){
				if(cycle >= 1 && cycle < 256){
					PPUSTATUS.setSpriteZeroHit(true);
				}
			}
		}
	}

	// Zeichnen
	uint8_t colorIndex = ((palette << 2) + pixel );
	auto addr = 0x3F00 + colorIndex;
	
	uint8_t paletteVal = mapper->readVRAM((uint8_t*)(uintptr_t)addr) & 0x3F;
	setPixel(cycle-1, scanline, pal.getColor(paletteVal & (PPUMASK.getGrayScale() ? 0x30 : 0x3F)));
}

void Ppu::maybeRiseA12()
{
	if(PPUMASK.getRenderBackground() || PPUMASK.getRenderSprites())
		mapper->riseA12();
}
