#include "ppu.h"
#include <iostream>

FrameRoutine Ppu::fakeFrame()
{



    // Pre-render scanline

    // 341 dots pro scanline, außer in der Pre-Render scanline bei ungerader Frameanzahl
    int dotsPerLine = 341;
    for(int i = 0; i < dotsPerLine; i++){
        co_await std::suspend_always{};
    }

    // Sichtbare Scanlines
    for(int i = 0; i <= 239; i++){

        // Idle-Zyklus
        if(i!=0 || unevenFrame)
            co_await std::suspend_always{};

        // Zyklen 1-256
        for(int j = 0; j <= 255; j++){
            int x,y;
            y = i + vram_addr.fine_y;
            x = j + fine_x;
            // Background Evalutation
            uint16_t nametable_start = 0x2000;
            // Nametable Byte
            uint16_t tile_address = nametable_start + (32 * (y / 8)) + (x / 8);
            uint8_t nt_index = mapper->readVRAM((uint8_t*)(uintptr_t)tile_address);
            uintptr_t nt_entry = (((uintptr_t)(nt_index)) * 16 + (y%8));
			
            // Niedriges Pattern Table Tile
            uint8_t pt_entry_plane_1 = mapper->readVRAM((uint8_t*)backgroundTable + nt_entry);
            // Hohes Pattern Table Tile (+8 bytes vom ersten)
            uint8_t pt_entry_plane_2 = mapper->readVRAM((uint8_t*)(backgroundTable + nt_entry+8));
			
            // Attribute Table Byte
            uint16_t attribute_address = nametable_start + 0x3C0 + (8 * (y / 32)) + (x / 32);
            uint8_t at_entry = mapper->readVRAM((uint8_t*)(uintptr_t)attribute_address);
            bool top = false, left = false;
            if(y%32 <16) top = true;
            if(x%32 <16) left = true;
            uint8_t att_bits;
            if(top && left) att_bits = (at_entry & 0b00000011) << 2; // top left
            if(top && !left) att_bits = (at_entry & 0b00001100); // top right
            if(!top && left) att_bits = (at_entry & 0b00110000) >> 2; // bottom left
            if(!top && !left) att_bits = (at_entry & 0b11000000) >> 4; // bottom right
            bool p1 = pt_entry_plane_1 & (128u >> (x%8));
            bool p2 = pt_entry_plane_2 & (128u >> (x%8));
            uint8_t color_index;
            if(p1 && p2) color_index = 3;
            else if(p2) color_index = 2;
            else if(p1) color_index = 1;
            else color_index = 0;
            color_index = color_index | att_bits | 0b00000000; // Hintergrund
            uint8_t pallete_value = mapper->readVRAM((uint8_t*)(uintptr_t)0x3F00 + color_index);
            setPixel(x, y, pal.getColor(pallete_value));



            co_await std::suspend_always{};
        }

        // Zyklen 257-320 (Tile-Daten für nächste Zeile fetchen)
        for(int j = 257; j <= 320; j++){
            OAMADDR = 0;
            co_await std::suspend_always{};
        }

        // Zyklen 321-336
        for(int j = 321; j <= 336; j++){
            co_await std::suspend_always{};
        }

        // Zyklen 337-340
        for(int j = 337; j <= 340; j++){
            co_await std::suspend_always{};
        }
    }

    // Post-Render Scanline (Idle)
    for(int i = 0; i < dotsPerLine; i++){
        co_await std::suspend_always{};
    }

    // Vertical Blanking (241-260)
    for(int i = 241; i <= 259; i++){
        PPUSTATUS.vertical_blank = 1;
        blanking = true;
        if(PPUSTATUS.vertical_blank && PPUCTRL.enable_nmi) mapper->pullNMI();
        for(int i = 0; i < dotsPerLine; i++){
            co_await std::suspend_always{};
        }   
    }
    // Für die letzte Zeile von VBlank -1, damit im letzten dot die Coroutine beendet wird
    for(int i = 0; i < dotsPerLine-1; i++){
        co_await std::suspend_always{};
    }   

    co_return;
}

void Ppu::fakeClock()
{
    [[unlikely]] if(!state.resume()) {
        // Frame fertig-gerendert
        state = fakeFrame();
        frameReady = true;
        unevenFrame = !unevenFrame;
        screen->copyBufferToScreen(pixelBuffer);
    }
}

void Ppu::clock()
{
	// As we progress through scanlines and cycles, the PPU is effectively
	// a state machine going through the motions of fetching background 
	// information and sprite information, compositing them into a pixel
	// to be output.

	// The lambda functions (functions inside functions) contain the various
	// actions to be performed depending upon the output of the state machine
	// for a given scanline/cycle combination

	// ==============================================================================
	// Increment the background tile "pointer" one tile/column horizontally
	auto IncrementScrollX = [&]()
	{
		// Note: pixel perfect scrolling horizontally is handled by the 
		// data shifters. Here we are operating in the spatial domain of 
		// tiles, 8x8 pixel blocks.
		
		// Ony if rendering is enabled
		if (mask.render_background || mask.render_sprites)
		{
			// A single name table is 32x30 tiles. As we increment horizontally
			// we may cross into a neighbouring nametable, or wrap around to
			// a neighbouring nametable
			if (vram_addr.coarse_x == 31)
			{
				// Leaving nametable so wrap address round
				vram_addr.coarse_x = 0;
				// Flip target nametable bit
				vram_addr.name_table_x = ~vram_addr.name_table_x;
			}
			else
			{
				// Staying in current nametable, so just increment
				vram_addr.coarse_x++;
			}
		}
	};

	// ==============================================================================
	// Increment the background tile "pointer" one scanline vertically
	auto IncrementScrollY = [&]()
	{
		// Incrementing vertically is more complicated. The visible nametable
		// is 32x30 tiles, but in memory there is enough room for 32x32 tiles.
		// The bottom two rows of tiles are in fact not tiles at all, they
		// contain the "attribute" information for the entire table. This is
		// information that describes which palettes are used for different 
		// regions of the nametable.
		
		// In addition, the NES doesnt scroll vertically in chunks of 8 pixels
		// i.e. the height of a tile, it can perform fine scrolling by using
		// the fine_y component of the register. This means an increment in Y
		// first adjusts the fine offset, but may need to adjust the whole
		// row offset, since fine_y is a value 0 to 7, and a row is 8 pixels high

		// Ony if rendering is enabled
		if (mask.render_background || mask.render_sprites)
		{
			// If possible, just increment the fine y offset
			if (vram_addr.fine_y < 7)
			{
				vram_addr.fine_y++;
			}
			else
			{
				// If we have gone beyond the height of a row, we need to
				// increment the row, potentially wrapping into neighbouring
				// vertical nametables. Dont forget however, the bottom two rows
				// do not contain tile information. The coarse y offset is used
				// to identify which row of the nametable we want, and the fine
				// y offset is the specific "scanline"

				// Reset fine y offset
				vram_addr.fine_y = 0;

				// Check if we need to swap vertical nametable targets
				if (vram_addr.coarse_y == 29)
				{
					// We do, so reset coarse y offset
					vram_addr.coarse_y = 0;
					// And flip the target nametable bit
					vram_addr.name_table_y = ~vram_addr.name_table_y;
				}
				else if (vram_addr.coarse_y == 31)
				{
					// In case the pointer is in the attribute memory, we
					// just wrap around the current nametable
					vram_addr.coarse_y = 0;
				}
				else
				{
					// None of the above boundary/wrapping conditions apply
					// so just increment the coarse y offset
					vram_addr.coarse_y++;
				}
			}
		}
	};

	// ==============================================================================
	// Transfer the temporarily stored horizontal nametable access information
	// into the "pointer". Note that fine x scrolling is not part of the "pointer"
	// addressing mechanism
	auto TransferAddressX = [&]()
	{
		// Ony if rendering is enabled
		if (mask.render_background || mask.render_sprites)
		{
			vram_addr.name_table_x = tram_addr.name_table_x;
			vram_addr.coarse_x    = tram_addr.coarse_x;
		}
	};

	// ==============================================================================
	// Transfer the temporarily stored vertical nametable access information
	// into the "pointer". Note that fine y scrolling is part of the "pointer"
	// addressing mechanism
	auto TransferAddressY = [&]()
	{
		// Ony if rendering is enabled
		if (mask.render_background || mask.render_sprites)
		{
			vram_addr.fine_y      = tram_addr.fine_y;
			vram_addr.name_table_y = tram_addr.name_table_y;
			vram_addr.coarse_y    = tram_addr.coarse_y;
		}
	};


	// ==============================================================================
	// Prime the "in-effect" background tile shifters ready for outputting next
	// 8 pixels in scanline.
	auto LoadBackgroundShifters = [&]()
	{	
		// Each PPU update we calculate one pixel. These shifters shift 1 bit along
		// feeding the pixel compositor with the binary information it needs. Its
		// 16 bits wide, because the top 8 bits are the current 8 pixels being drawn
		// and the bottom 8 bits are the next 8 pixels to be drawn. Naturally this means
		// the required bit is always the MSB of the shifter. However, "fine x" scrolling
		// plays a part in this too, whcih is seen later, so in fact we can choose
		// any one of the top 8 bits.
		bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
		bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;

		// Attribute bits do not change per pixel, rather they change every 8 pixels
		// but are synchronised with the pattern shifters for convenience, so here
		// we take the bottom 2 bits of the attribute word which represent which 
		// palette is being used for the current 8 pixels and the next 8 pixels, and 
		// "inflate" them to 8 bit words.
		bg_shifter_attrib_lo  = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
		bg_shifter_attrib_hi  = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
	};


	// ==============================================================================
	// Every cycle the shifters storing pattern and attribute information shift
	// their contents by 1 bit. This is because every cycle, the output progresses
	// by 1 pixel. This means relatively, the state of the shifter is in sync
	// with the pixels being drawn for that 8 pixel section of the scanline.
	auto UpdateShifters = [&]()
	{
		if (mask.render_background)
		{
			// Shifting background tile pattern row
			bg_shifter_pattern_lo <<= 1;
			bg_shifter_pattern_hi <<= 1;

			// Shifting palette attributes by 1
			bg_shifter_attrib_lo <<= 1;
			bg_shifter_attrib_hi <<= 1;
		}
	};

	// All but 1 of the secanlines is visible to the user. The pre-render scanline
	// at -1, is used to configure the "shifters" for the first visible scanline, 0.
	if (scanline >= -1 && scanline < 240)
	{		
		if (scanline == 0 && cycle == 0)
		{
			// "Odd Frame" cycle skip
			cycle = 1;
		}

		if (scanline == -1 && cycle == 1)
		{
			// Effectively start of new frame, so clear vertical blank flag
			PPUSTATUS.vertical_blank = 0;
		}


		if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338))
		{
			UpdateShifters();
			
			
			// In these cycles we are collecting and working with visible data
			// The "shifters" have been preloaded by the end of the previous
			// scanline with the data for the start of this scanline. Once we
			// leave the visible region, we go dormant until the shifters are
			// preloaded for the next scanline.

			// Fortunately, for background rendering, we go through a fairly
			// repeatable sequence of events, every 2 clock cycles.
			switch ((cycle - 1) % 8)
			{
			case 0:
				// Load the current background tile pattern and attributes into the "shifter"
				LoadBackgroundShifters();

				// Fetch the next background tile ID
				// "(vram_addr.reg & 0x0FFF)" : Mask to 12 bits that are relevant
				// "| 0x2000"                 : Offset into nametable space on PPU address bus
				bg_next_tile_id = mapper->readVRAM((uint8_t*)(uintptr_t)(0x2000 | (vram_addr.value & 0x0FFF)));

				// Explanation:
				// The bottom 12 bits of the loopy register provide an index into
				// the 4 nametables, regardless of nametable mirroring configuration.
				// nametable_y(1) nametable_x(1) coarse_y(5) coarse_x(5)
				//
				// Consider a single nametable is a 32x32 array, and we have four of them
				//   0                1
				// 0 +----------------+----------------+
				//   |                |                |
				//   |                |                |
				//   |    (32x32)     |    (32x32)     |
				//   |                |                |
				//   |                |                |
				// 1 +----------------+----------------+
				//   |                |                |
				//   |                |                |
				//   |    (32x32)     |    (32x32)     |
				//   |                |                |
				//   |                |                |
				//   +----------------+----------------+
				//
				// This means there are 4096 potential locations in this array, which 
				// just so happens to be 2^12!
				break;
			case 2:
				// Fetch the next background tile attribute. OK, so this one is a bit
				// more involved :P

				// Recall that each nametable has two rows of cells that are not tile 
				// information, instead they represent the attribute information that
				// indicates which palettes are applied to which area on the screen.
				// Importantly (and frustratingly) there is not a 1 to 1 correspondance
				// between background tile and palette. Two rows of tile data holds
				// 64 attributes. Therfore we can assume that the attributes affect
				// 8x8 zones on the screen for that nametable. Given a working resolution
				// of 256x240, we can further assume that each zone is 32x32 pixels
				// in screen space, or 4x4 tiles. Four system palettes are allocated
				// to background rendering, so a palette can be specified using just
				// 2 bits. The attribute byte therefore can specify 4 distinct palettes.
				// Therefore we can even further assume that a single palette is
				// applied to a 2x2 tile combination of the 4x4 tile zone. The very fact
				// that background tiles "share" a palette locally is the reason why
				// in some games you see distortion in the colours at screen edges.

				// As before when choosing the tile ID, we can use the bottom 12 bits of
				// the loopy register, but we need to make the implementation "coarser"
				// because instead of a specific tile, we want the attribute byte for a 
				// group of 4x4 tiles, or in other words, we divide our 32x32 address
				// by 4 to give us an equivalent 8x8 address, and we offset this address
				// into the attribute section of the target nametable.

				// Reconstruct the 12 bit loopy address into an offset into the
				// attribute memory

				// "(vram_addr.coarse_x >> 2)"        : integer divide coarse x by 4, 
				//                                      from 5 bits to 3 bits
				// "((vram_addr.coarse_y >> 2) << 3)" : integer divide coarse y by 4, 
				//                                      from 5 bits to 3 bits,
				//                                      shift to make room for coarse x

				// Result so far: YX00 00yy yxxx

				// All attribute memory begins at 0x03C0 within a nametable, so OR with
				// result to select target nametable, and attribute byte offset. Finally
				// OR with 0x2000 to offset into nametable address space on PPU bus.				
				bg_next_tile_attrib = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                              0x23C0 | (vram_addr.name_table_y << 11) 
					                                 | (vram_addr.name_table_x << 10) 
					                                 | ((vram_addr.coarse_y >> 2) << 3) 
					                                 | (vram_addr.coarse_x >> 2)));
				
				// Right we've read the correct attribute byte for a specified address,
				// but the byte itself is broken down further into the 2x2 tile groups
				// in the 4x4 attribute zone.

				// The attribute byte is assembled thus: BR(76) BL(54) TR(32) TL(10)
				//
				// +----+----+			    +----+----+
				// | TL | TR |			    | ID | ID |
				// +----+----+ where TL =   +----+----+
				// | BL | BR |			    | ID | ID |
				// +----+----+			    +----+----+
				//
				// Since we know we can access a tile directly from the 12 bit address, we
				// can analyse the bottom bits of the coarse coordinates to provide us with
				// the correct offset into the 8-bit word, to yield the 2 bits we are
				// actually interested in which specifies the palette for the 2x2 group of
				// tiles. We know if "coarse y % 4" < 2 we are in the top half else bottom half.
				// Likewise if "coarse x % 4" < 2 we are in the left half else right half.
				// Ultimately we want the bottom two bits of our attribute word to be the
				// palette selected. So shift as required...				
				if (vram_addr.coarse_y & 0x02) bg_next_tile_attrib >>= 4;
				if (vram_addr.coarse_x & 0x02) bg_next_tile_attrib >>= 2;
				bg_next_tile_attrib &= 0x03;
				break;

				// Compared to the last two, the next two are the easy ones... :P

			case 4: 
				// Fetch the next background tile LSB bit plane from the pattern memory
				// The Tile ID has been read from the nametable. We will use this id to 
				// index into the pattern memory to find the correct sprite (assuming
				// the sprites lie on 8x8 pixel boundaries in that memory, which they do
				// even though 8x16 sprites exist, as background tiles are always 8x8).
				//
				// Since the sprites are effectively 1 bit deep, but 8 pixels wide, we 
				// can represent a whole sprite row as a single byte, so offsetting
				// into the pattern memory is easy. In total there is 8KB so we need a 
				// 13 bit address.

				// "(control.pattern_background << 12)"  : the pattern memory selector 
				//                                         from control register, either 0K
				//                                         or 4K offset
				// "((uint16_t)bg_next_tile_id << 4)"    : the tile id multiplied by 16, as
				//                                         2 lots of 8 rows of 8 bit pixels
				// "(vram_addr.fine_y)"                  : Offset into which row based on
				//                                         vertical scroll offset
				// "+ 0"                                 : Mental clarity for plane offset
				// Note: No PPU address bus offset required as it starts at 0x0000
				bg_next_tile_lsb = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                             (PPUCTRL.pattern_background << 12) 
					                       + ((uint16_t)bg_next_tile_id << 4) 
					                       + (vram_addr.fine_y) + 0));

				break;
			case 6:
				// Fetch the next background tile MSB bit plane from the pattern memory
				// This is the same as above, but has a +8 offset to select the next bit plane
				bg_next_tile_msb = mapper->readVRAM((uint8_t*)(uintptr_t)(
                                             (PPUCTRL.pattern_background << 12)
					                       + ((uint16_t)bg_next_tile_id << 4)
					                       + (vram_addr.fine_y) + 8));
				break;
			case 7:
				// Increment the background tile "pointer" to the next tile horizontally
				// in the nametable memory. Note this may cross nametable boundaries which
				// is a little complex, but essential to implement scrolling
				IncrementScrollX();
				break;
			}
		}

		// End of a visible scanline, so increment downwards...
		if (cycle == 256)
		{
			IncrementScrollY();
		}

		//...and reset the x position
		if (cycle == 257)
		{
			LoadBackgroundShifters();
			TransferAddressX();
		}

		// Superfluous reads of tile id at end of scanline
		if (cycle == 338 || cycle == 340)
		{
			bg_next_tile_id = mapper->readVRAM((uint8_t*)(uintptr_t)(0x2000 | (vram_addr.value & 0x0FFF)));
		}

		if (scanline == -1 && cycle >= 280 && cycle < 305)
		{
			// End of vertical blank period so reset the Y address ready for rendering
			TransferAddressY();
		}
	}

	if (scanline == 240)
	{
		// Post Render Scanline - Do Nothing!
	}

	if (scanline >= 241 && scanline < 261)
	{
		if (scanline == 241 && cycle == 1)
		{
			// Effectively end of frame, so set vertical blank flag
			PPUSTATUS.vertical_blank = 1;

			// If the control register tells us to emit a NMI when
			// entering vertical blanking period, do it! The CPU
			// will be informed that rendering is complete so it can
			// perform operations with the PPU knowing it wont
			// produce visible artefacts
			if (PPUCTRL.enable_nmi) 
				mapper->pullNMI();
		}
	}



	// Composition - We now have background pixel information for this cycle
	// At this point we are only interested in background

	uint8_t bg_pixel = 0x00;   // The 2-bit pixel to be rendered
	uint8_t bg_palette = 0x00; // The 3-bit index of the palette the pixel indexes

	// We only render backgrounds if the PPU is enabled to do so. Note if 
	// background rendering is disabled, the pixel and palette combine
	// to form 0x00. This will fall through the colour tables to yield
	// the current background colour in effect
	if (mask.render_background)
	{
		// Handle Pixel Selection by selecting the relevant bit
		// depending upon fine x scolling. This has the effect of
		// offsetting ALL background rendering by a set number
		// of pixels, permitting smooth scrolling
		uint16_t bit_mux = 0x8000 >> fine_x;

		// Select Plane pixels by extracting from the shifter 
		// at the required location. 
		uint8_t p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
		uint8_t p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;

		// Combine to form pixel index
		bg_pixel = (p1_pixel << 1) | p0_pixel;

		// Get palette
		uint8_t bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
		uint8_t bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
		bg_palette = (bg_pal1 << 1) | bg_pal0;
	}


	// Now we have a final pixel colour, and a palette for this cycle
	// of the current scanline. Let's at long last, draw that ^&%*er :P

	uint8_t color_index = ((bg_palette << 2) + bg_pixel );
	uint8_t palette_val = mapper->readVRAM((uint8_t*)(uintptr_t)(0x3F00) + color_index);
	setPixel(cycle-1, scanline, pal.getColor(palette_val ));

	// Advance renderer - it never stops, it's relentless
	cycle++;
	if (cycle >= 341)
	{
		cycle = 0;
		scanline++;
		if (scanline >= 261)
		{
			scanline = -1;
			frameReady = true;
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
    else if(reg==(uint8_t*)&mask){
        mask.reg = val;
    }
    else if(reg==&OAMADDR){
        OAMADDR = val;
    }
    else if(reg==&OAMDATA){
        if(blanking){
            OAM[OAMADDR++] = val;
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
    uint8_t b;

	if(reg == (uint8_t*)&PPUSTATUS){
		/* w: = 0 */
		w = false;

		/* Read register */
        uint8_t value = PPUSTATUS.reg;
        // Clear Vblank
        PPUSTATUS.vertical_blank = 0;
		return value;
    }
	if(reg == &OAMDATA){
		/* Read from OAM */
		return OAM[OAMADDR];
    }
	if(reg == &PPUDATA){
		/* Read from VRAM incrementing address accordingly */
		b = vram_buffer;
		vram_buffer = mapper->readVRAM((uint8_t*)(uintptr_t)vram_addr.value);
		vram_addr.value += PPUCTRL.increment_mode ? 32 : 1;
        // 0x3F00 = BG_PALETTE_START
		if (vram_addr.value >= 0x3F00)
			b = vram_buffer;
		return b;
    }
	return 0;
}
