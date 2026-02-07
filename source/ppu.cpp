#include "ppu.h"
#include <iostream>
#include <cstring>
#include <format>
#include <algorithm>

std::string phex(uintptr_t input){
    std::string str = std::format("{:x}", input);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}


void Ppu::clock()
{
	// Renderer fortschalten
	scanline = timings[ppuTiming].second.scanline;
	cycle = timings[ppuTiming].second.cycle;
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

void Ppu::setPixel(int x, int y, glm::vec3 c)
{
    int index = (3*x) + (3*256*y);
	if(x > 255 || y > 239 || x < 0 || y < 0){
		//std::cout << "Pixel out of bounds: " << "x: " << x << ", y: " << y << std::endl;
		return;
	}
    pixelBuffer[index] = c.r;
    pixelBuffer[index + 1] = c.g;
    pixelBuffer[index + 2] = c.b;
}

void Ppu::swapBuffers()
{
	std::swap(pixelBuffer, backBuffer);
}

void Ppu::writeRegister(uint8_t *reg, uint8_t val)
{
    if(reg==(uint8_t*)&PPUCTRL){
        // Wert schreiben
        PPUCTRL.raw = val;
        // Nametable select
        t.setNametableX(PPUCTRL.getNametableX());
        t.setNametableY(PPUCTRL.getNametableY());

    }
    else if(reg==(uint8_t*)&PPUMASK){
		PPUMASK.raw = val;
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
            fine_x = val & 0x07;
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
		w = false;

        uint8_t value = PPUSTATUS.raw;
        // Clear Vblank
        PPUSTATUS.setVerticalBlank(false);
		return value;
    }
	if(reg == &OAMDATA){
		if(scanline >= -1 && scanline <= 239 && cycle >= 0 && cycle < 64){
			return 0xFF;
		}
		/* Read from OAM */
		return pOAM[OAMADDR];
    }
	if(reg == &PPUDATA){
		// Verzögertes Lesen von VRAM
		tmp = vramReadBuffer;
		vramReadBuffer = mapper->readVRAM((uint8_t*)(uintptr_t)v.raw);
		v.raw += PPUCTRL.getIncrementMode() ? 32 : 1;
        // 0x3F00 = BG_PALETTE_START
		if (v.raw >= 0x3F00)
			tmp = vramReadBuffer;
		return tmp;
    }
	return 0;
}

void Ppu::fillTimings()
{
	int i = -1;
	for(int scanline = -1; scanline <= 260; scanline++){
		for(int cycle = 0; cycle <= 340; cycle++){
			i++;
			timings.push_back({std::vector<ppuCmd>(), {.scanline = scanline, .cycle = cycle}});

			if (scanline == -1 && cycle == 1){
				timings[i].first.push_back(&Ppu::clearFlags);
			}

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
			}

			if(cycle == 340){
				timings[i].first.push_back(&Ppu::setSpriteShifters);
			}

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
	shifterATLow  = (shifterATLow & 0xFF00) | ((nextTileATByte & 0b01) ? 0xFF : 0x00);
	shifterATHigh  = (shifterATHigh & 0xFF00) | ((nextTileATByte & 0b10) ? 0xFF : 0x00);
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
		for(int i = 0; i < sprite_count; i++){
			if(sprite_count > 8) break;
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
{
	nextTileATByte = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                              0x23C0 | (v.getNametableY() << 11) 
					                                 | (v.getNametableX() << 10) 
					                                 | ((v.getCoarseY() >> 2) << 3) 
					                                 | (v.getCoarseX() >> 2)));			
	if (v.getCoarseY() & 0x02) nextTileATByte >>= 4;
	if (v.getCoarseX() & 0x02) nextTileATByte >>= 2;
	nextTileATByte &= 0x03;
}

void Ppu::readCHRByteLow()
{
	nextTileCHRLow = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                             (PPUCTRL.getPatternBackground() << 12) 
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
	std::memset(secondaryOAM, 0xFF, 8 * sizeof(OAMSprite));
	sprite_count = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		spriteShifterCHRLow[i] = 0;
		spriteShifterCHRHigh[i] = 0;
	}
	uint8_t iOAM = 0;
	spriteZeroHitPossible = false;
	while(iOAM < 64 && sprite_count < 9){
		int16_t diff = ((int16_t)scanline - (int16_t)OAM[iOAM].yPos);
		if(diff >= 0 && diff < (PPUCTRL.getSpriteSize() ? 16 : 8)){
			if(sprite_count < 8){
				if(iOAM == 0){
					spriteZeroHitPossible = true;
				}
				memcpy(&secondaryOAM[sprite_count], &OAM[iOAM], sizeof(OAMSprite));
				sprite_count++;
			}
		}
		iOAM++;
	}
	PPUSTATUS.setSpriteOverflow(sprite_count > 8);
}

void Ppu::setSpriteShifters()
{
	for(uint8_t i = 0; i < sprite_count; i++){
		uint8_t spriteCHRLow, spriteCHRHigh;
		uint16_t spriteAddrLow, spriteAddrHigh;
		if(!PPUCTRL.getSpriteSize()){
			// 8x8 Modus
			if(!(secondaryOAM[i].attributes & 0x80)){
				// Normale vertikale Ausrichtung
				spriteAddrLow = (PPUCTRL.getPatternSprite() << 12) 		// welche Pattern Tabelle
							  | (secondaryOAM[i].tileIndex << 4) 	// welches Tile
							  | (scanline - secondaryOAM[i].yPos);	// wo im Tile
			}
			else{
				// Vertikal umgedreht
				spriteAddrLow = (PPUCTRL.getPatternSprite() << 12) 			// welche Pattern Tabelle
							  | (secondaryOAM[i].tileIndex << 4) 		// welches Tile
							  | 7 - (scanline - secondaryOAM[i].yPos);	// wo im Tile
			}
		}
		else{
			// 8x16
			if(!(secondaryOAM[i].attributes & 0x80)){
				// Normale vertikale Ausrichtung
				if(scanline - secondaryOAM[i].yPos < 8){
					// Obere Hälfte
					spriteAddrLow = ((secondaryOAM[i].tileIndex & 0x01) << 12)
								  | ((secondaryOAM[i].tileIndex & 0xFE) << 4)
								  | ((scanline - secondaryOAM[i].yPos) & 0x07);
				}
				else{
					// Untere Hälfte
					spriteAddrLow = ((secondaryOAM[i].tileIndex & 0x01) << 12)
								  | (((secondaryOAM[i].tileIndex & 0xFE) + 1) << 4)
								  | ((scanline - secondaryOAM[i].yPos) & 0x07);
				}
			}
			else{
				// Vertikal umgedreht
				if(scanline - secondaryOAM[i].yPos < 8){
					// Obere Hälfte
					spriteAddrLow = ((secondaryOAM[i].tileIndex & 0x01) << 12)
								  | ((secondaryOAM[i].tileIndex & 0xFE) << 4)
								  | (7 - (scanline - secondaryOAM[i].yPos) & 0x07);
				}
				else{
					// Untere Hälfte
					spriteAddrLow = ((secondaryOAM[i].tileIndex & 0x01) << 12)
								  | (((secondaryOAM[i].tileIndex & 0xFE) + 1) << 4)
								  | (7 - (scanline - secondaryOAM[i].yPos) & 0x07);
				}
			}
		}
		spriteAddrHigh = spriteAddrLow + 8;
		spriteCHRLow = mapper->readVRAM((uint8_t*)(uintptr_t)spriteAddrLow);
		spriteCHRHigh = mapper->readVRAM((uint8_t*)(uintptr_t)spriteAddrHigh);
		// Flip horizontal
		if(secondaryOAM[i].attributes & 0x40){
			// This little lambda function "flips" a byte
			// so 0b11100000 becomes 0b00000111. It's very
			// clever, and stolen completely from here:
			// https://stackoverflow.com/a/2602885
			auto flipbyte = [](uint8_t b)
			{
				b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
				b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
				b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
				return b;
			};
			spriteCHRLow = flipbyte(spriteCHRLow);
			spriteCHRHigh = flipbyte(spriteCHRHigh);
		}
		spriteShifterCHRLow[i] = spriteCHRLow;
		spriteShifterCHRHigh[i] = spriteCHRHigh;
	}
}

void Ppu::pullNMI()
{
	PPUSTATUS.setVerticalBlank(true);
	if (PPUCTRL.getEnableNMI())
		mapper->pullNMI();
}

void Ppu::renderPixel()
{
	uint8_t bg_pixel = 0x00;   // The 2-bit pixel to be rendered
	uint8_t bg_palette = 0x00; // The 3-bit index of the palette the pixel indexes


	// Hintergrund-Evaluierung
	if (PPUMASK.getRenderBackground()){
		uint16_t bit_mux = 0x8000 >> fine_x;

		// Select Plane pixels by extracting from the shifter 
		// at the required location. 
		uint8_t p0_pixel = (shifterCHRLow & bit_mux) > 0;
		uint8_t p1_pixel = (shifterCHRHigh & bit_mux) > 0;

		// Combine to form pixel index
		bg_pixel = (p1_pixel << 1) | p0_pixel;

		// Get palette
		uint8_t bg_pal0 = (shifterATLow& bit_mux) > 0;
		uint8_t bg_pal1 = (shifterATHigh & bit_mux) > 0;
		bg_palette = (bg_pal1 << 1) | bg_pal0;
	}

	uint8_t fg_pixel = 0x00;
	uint8_t fg_palette = 0x00;
	uint8_t fg_priority = 0x00;

	if (PPUMASK.getRenderSprites()){
		spriteZeroBeingRendered = false;
		for(uint8_t i = 0; i < sprite_count; i++){
			if(secondaryOAM[i].xPos == 0){
				uint8_t fg_pixel_lo = (spriteShifterCHRLow[i] & 0x80) > 0;
				uint8_t fg_pixel_hi = (spriteShifterCHRHigh[i] & 0x80) > 0;
				fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

				fg_palette = (secondaryOAM[i].attributes & 0x03) + 0x04;
				fg_priority = (secondaryOAM[i].attributes & 0x20) == 0;

				if(fg_pixel != 0){
					// Erster nicht-transparenter Sprite gefunden
					if(i==0){
						spriteZeroBeingRendered = true;
					}
					break;
				}
			}
		}
	}

	uint8_t pixel = 0x00;
	uint8_t palette = 0x00;

	if(bg_pixel == 0 && fg_pixel == 0){
		// beide transparent
		pixel = 0x00;
		palette = 0x00;
	}
	else if(bg_pixel == 0 && fg_pixel > 0){
		// hintergrund transparent
		pixel = fg_pixel;
		palette = fg_palette;
	}
	else if(bg_pixel > 0 && fg_pixel == 0){
		// vordergrund transparent
		pixel = bg_pixel;
		palette = bg_palette;
	}
	else if(bg_pixel > 0 && fg_pixel > 0){
		if(fg_priority){
			pixel = fg_pixel;
			palette = fg_palette;
		}
		else{
			pixel = bg_pixel;
			palette = bg_palette;
		}

		// Sprite Zero Hit
		if(spriteZeroHitPossible && spriteZeroBeingRendered){
			if(PPUMASK.getRenderSprites() && PPUMASK.getRenderBackground()){
				if(!(PPUMASK.getRenderBackgroundLeft() || PPUMASK.getRenderSpritesLeft())){
					if(cycle >= 9 && cycle < 258){
						PPUSTATUS.setSpriteZeroHit(true);
					}
				}
				else{
					if(cycle >= 1 && cycle < 258){
						PPUSTATUS.setSpriteZeroHit(true);
					}
				}
			}
		}
	}

	// Zeichnen
	uint8_t color_index = ((palette << 2) + pixel );
	auto addr = (0x3F00) + color_index;
	
	uint8_t palette_val = mapper->readVRAM((uint8_t*)(uintptr_t)addr) & 0x3F;
	setPixel(cycle-1, scanline, pal.getColor(palette_val & (PPUMASK.getGrayScale() ? 0x30 : 0x3F)));
}
