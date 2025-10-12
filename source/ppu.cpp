#include "ppu.h"
#include <iostream>
#include <cstring>


void Ppu::clock()
{

	auto IncrementScrollX = [&](){
		if (PPUMASK.render_background || PPUMASK.render_sprites){
			if (vram_addr.coarse_x == 31){
				vram_addr.coarse_x = 0;
				vram_addr.name_table_x = ~vram_addr.name_table_x;
			}
			else{
				vram_addr.coarse_x++;
			}
		}
	};

	auto IncrementScrollY = [&](){
		if (PPUMASK.render_background || PPUMASK.render_sprites){
			if (vram_addr.fine_y < 7){
				vram_addr.fine_y++;
			}
			else{
				vram_addr.fine_y = 0;

				if (vram_addr.coarse_y == 29){
					vram_addr.coarse_y = 0;
					vram_addr.name_table_y = ~vram_addr.name_table_y;
				}
				else if (vram_addr.coarse_y == 31){
					vram_addr.coarse_y = 0;
				}
				else{
					vram_addr.coarse_y++;
				}
			}
		}
	};

	auto TransferAddressX = [&](){
		// Passiert nur bei eingeschaltetem Rendering
		if (PPUMASK.render_background || PPUMASK.render_sprites){
			vram_addr.name_table_x = tram_addr.name_table_x;
			vram_addr.coarse_x     = tram_addr.coarse_x;
		}
	};

	auto TransferAddressY = [&](){
		// Passiert nur bei eingeschaltetem Rendering
		if (PPUMASK.render_background || PPUMASK.render_sprites){
			vram_addr.fine_y       = tram_addr.fine_y;
			vram_addr.name_table_y = tram_addr.name_table_y;
			vram_addr.coarse_y     = tram_addr.coarse_y;
		}
	};

	auto LoadBackgroundShifters = [&](){	
		shifterCHRLow = (shifterCHRLow & 0xFF00) | nextTileCHRLow;
		shifterCHRHigh = (shifterCHRHigh & 0xFF00) | nextTileCHRHigh;

		shifterATLow  = (shifterATLow & 0xFF00) | ((nextTileATByte & 0b01) ? 0xFF : 0x00);
		shifterATHigh  = (shifterATHigh & 0xFF00) | ((nextTileATByte & 0b10) ? 0xFF : 0x00);
	};

	auto UpdateShifters = [&](){
		if (PPUMASK.render_background){
			shifterCHRLow <<= 1;
			shifterCHRHigh <<= 1;
			shifterATLow <<= 1;
			shifterATHigh <<= 1;
		}

		if(PPUMASK.render_sprites && cycle >= 1 && cycle < 258){
			for(int i = 0; i < sprite_count; i++){
				if(secondaryOAM[i].xPos > 0){
					secondaryOAM[i].xPos--;
				}
				else{
					spriteShifterCHRLow[i] <<= 1;
					spriteShifterCHRHigh[i] <<= 1;
				}
			}
		}
	};

	if (scanline >= -1 && scanline < 240)
	{

		if (scanline == 0 && cycle == 0){
			// "Odd Frame" cycle skip
			cycle = 1;
		}

		if (scanline == -1 && cycle == 1){
			PPUSTATUS.vertical_blank = 0;

			PPUSTATUS.sprite_overflow = 0;

			PPUSTATUS.sprite_zero_hit = 0;

			for(int i = 0; i < 8; i++){
				spriteShifterCHRLow[i] = 0;
				spriteShifterCHRHigh[i] = 0;
			}
		}


		if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338)){
			UpdateShifters();
			
			
			// Alle 8 Zyklen-Rythmus
			switch ((cycle - 1) % 8){
			case 0:
				LoadBackgroundShifters();
				nextTileNTByte = mapper->readVRAM((uint8_t*)(uintptr_t)(0x2000 | (vram_addr.value & 0x0FFF)));
				break;
			case 2:			
				nextTileATByte = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                              0x23C0 | (vram_addr.name_table_y << 11) 
					                                 | (vram_addr.name_table_x << 10) 
					                                 | ((vram_addr.coarse_y >> 2) << 3) 
					                                 | (vram_addr.coarse_x >> 2)));			
				if (vram_addr.coarse_y & 0x02) nextTileATByte >>= 4;
				if (vram_addr.coarse_x & 0x02) nextTileATByte >>= 2;
				nextTileATByte &= 0x03;
				break;
			case 4:
				nextTileCHRLow = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                             (PPUCTRL.pattern_background << 12) 
					                       + ((uint16_t)nextTileNTByte << 4) 
					                       + (vram_addr.fine_y) + 0));

				break;
			case 6:
				nextTileCHRHigh = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                             (PPUCTRL.pattern_background << 12)
					                       + ((uint16_t)nextTileNTByte << 4)
					                       + (vram_addr.fine_y) + 8));
				break;
			case 7:
				IncrementScrollX();
				break;
			}
		}

		// End of a visible scanline, so increment downwards...
		if (cycle == 256){
			IncrementScrollY();
		}

		//...and reset the x position
		if (cycle == 257){
			LoadBackgroundShifters();
			TransferAddressX();
		}

		// Superfluous reads of tile id at end of scanline
		if (cycle == 338 || cycle == 340){
			nextTileATByte = mapper->readVRAM((uint8_t*)(uintptr_t)(0x2000 | (vram_addr.value & 0x0FFF)));
		}

		// Sprite Evaluation
		if(cycle==257 && scanline >=0){
			std::memset(secondaryOAM, 0xFF, 8 * sizeof(OAMSprite));
			sprite_count = 0;

			for (uint8_t i = 0; i < 8; i++)
			{
				spriteShifterCHRLow[i] = 0;
				spriteShifterCHRHigh[i] = 0;
			}

			uint8_t iOAM = 0;
			while(iOAM < 64 && sprite_count < 9){
				int16_t diff = ((int16_t)scanline - (int16_t)OAM[iOAM].yPos);

				if(diff >= 0 && diff < (PPUCTRL.sprite_size ? 16 : 8)){
					if(sprite_count < 8){
						memcpy(&secondaryOAM[sprite_count], &OAM[iOAM], sizeof(OAMSprite));
						sprite_count++;
					}
				}
				iOAM++;
			}
			PPUSTATUS.sprite_overflow = (sprite_count > 8);
		}

		if (scanline == -1 && cycle >= 280 && cycle < 305){
			// End of vertical blank period so reset the Y address ready for rendering
			TransferAddressY();
		}
	}

	if(cycle==340){
		for(uint8_t i = 0; i < sprite_count; i++){
			uint8_t spriteCHRLow, spriteCHRHigh;
			uint16_t spriteAddrLow, spriteAddrHigh;

			if(!PPUCTRL.sprite_size){
				// 8x8 Modus
				if(!(secondaryOAM[i].attributes & 0x80)){
					// Normale vertikale Ausrichtung
					spriteAddrLow = (PPUCTRL.pattern_sprite << 12) 		// welche Pattern Tabelle
								  | (secondaryOAM[i].tileIndex << 4) 	// welches Tile
								  | (scanline - secondaryOAM[i].yPos);	// wo im Tile
				}
				else{
					// Vertikal umgedreht
					spriteAddrLow = (PPUCTRL.pattern_sprite << 12) 			// welche Pattern Tabelle
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

	if (scanline == 240){
		// Post Render Scanline
	}

	if (scanline >= 241 && scanline < 261){
		if (scanline == 241 && cycle == 1){
			PPUSTATUS.vertical_blank = 1;
			if (PPUCTRL.enable_nmi) 
				mapper->pullNMI();
		}
	}

	uint8_t bg_pixel = 0x00;   // The 2-bit pixel to be rendered
	uint8_t bg_palette = 0x00; // The 3-bit index of the palette the pixel indexes


	// Hintergrund-Evaluierung
	if (PPUMASK.render_background){
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

	if (PPUMASK.render_sprites){
		for(uint8_t i = 0; i < sprite_count; i++){
			if(secondaryOAM[i].xPos == 0){
				uint8_t fg_pixel_lo = (spriteShifterCHRLow[i] & 0x80) > 0;
				uint8_t fg_pixel_hi = (spriteShifterCHRHigh[i] & 0x80) > 0;
				fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

				fg_palette = (secondaryOAM[i].attributes & 0x03) + 0x04;
				fg_priority = (secondaryOAM[i].attributes & 0x20) == 0;

				if(fg_pixel != 0){
					// Erster nicht-transparenter Sprite gefunden
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
	}

	// Zeichnen
	uint8_t color_index = ((palette << 2) + pixel );
	uint8_t palette_val = mapper->readVRAM((uint8_t*)(uintptr_t)(0x3F00) + color_index);
	setPixel(cycle-1, scanline, pal.getColor(palette_val ));

	// Renderer fortschalten
	cycle++;
	if (cycle >= 341){
		cycle = 0;
		scanline++;
		if (scanline >= 261){
			scanline = -1;
			frameReady = true;
			unevenFrame = !unevenFrame;
			screen->copyBufferToScreen(pixelBuffer);
		}
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


void Ppu::writeRegister(uint8_t *reg, uint8_t val)
{
    if(reg==(uint8_t*)&PPUCTRL){
        // Wert schreiben
        PPUCTRL.value = val;
        // Nametable select
        tram_addr.name_table_x = PPUCTRL.nametable_x;
        tram_addr.name_table_y = PPUCTRL.nametable_y;

    }
    else if(reg==(uint8_t*)&PPUMASK){
        PPUMASK.reg = val;
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
            tram_addr.coarse_x = val >> 3;
            w = true;
        }
        else{
            tram_addr.fine_y = val & 0x07;
            tram_addr.coarse_y = val >> 3;
            w = false;
        }
    }
    else if(reg==&PPUADDR){
        if(!w){
            tram_addr.value = (uint16_t)((val & 0x3F) << 8) | (tram_addr.value & 0x00FF);
            w = true;
        }
        else{
            tram_addr.value = (tram_addr.value & 0xFF00) | val;
            vram_addr = tram_addr;
            w = false;
        }
    }
    else if(reg==&PPUDATA){
        mapper->writeVRAM((uint8_t*)(uintptr_t)vram_addr.value, val);
        vram_addr.value += PPUCTRL.increment_mode ? 32 : 1;
    }
}

uint8_t Ppu::readRegister(uint8_t *reg)
{
    uint8_t tmp;

	if(reg == (uint8_t*)&PPUSTATUS){
		w = false;

        uint8_t value = PPUSTATUS.reg;
        // Clear Vblank
        PPUSTATUS.vertical_blank = 0;
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
		vramReadBuffer = mapper->readVRAM((uint8_t*)(uintptr_t)vram_addr.value);
		vram_addr.value += PPUCTRL.increment_mode ? 32 : 1;
        // 0x3F00 = BG_PALETTE_START
		if (vram_addr.value >= 0x3F00)
			tmp = vramReadBuffer;
		return tmp;
    }
	return 0;
}
