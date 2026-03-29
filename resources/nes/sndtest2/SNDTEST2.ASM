; * NES sound testing program 2 - even testier
; * Updated by Controllerhead (Bill Moran)
; * Uh, i fixed some code, added featues, replaced font,
; * clarified things (hopefully), and it works on console. Enjoy!
; * 
; * Original text is as follows...

;
; NES sound testing program
; Written by SnowBro <kentmhan@online.no>
;
; Yep, I know there's a program similar to this already around, but I
; wanted to write one on my own. Partly to try and figure out more about
; how the NES sound registers work, and partly to get some experience
; with 6502 programming, which I'm pretty rusty at. I tried to keep the
; code small, but was a bit lazy here and there. Same goes for the lacking
; comments in some areas. The font was disrespectfully ripped from
; Castlevania 3.
;
; This code hasn't been tested on a real NES, so I don't know if it will
; work properly (sigh). If anyone has the chance to try it, please report
; back to me. It has been tested with most reliable NES emulators and
; should work fine on them.
;
; The program is simple: it lets you set individual bits of the NES sound
; registers for each channel and then test the output.
; The joypad buttons do the following:
;
;       Up/Down:    Select register of current channel
;       Right/Left: Select bit of current register
;       A:          Toggle current bit
;       B:          Toggle register update *
;       Select:     Select channel
;       Start:      Play channel
;
; The code was assembled with ASM6F, a great S/NES assembler by minus.
; Modify the code as you wish.
;
; Check out my webpage at http://home.sol.no/~kenhanse/nes/ for some
; of the other stuff I've written.
;
; Kent Hansen 01/01/99
; * Controllerhead 2/27/20

header:
; 		 N    E    S    _   1x16 1x8  Hmir       C    H    _    V    1    .    0    0 
.db 	$4E, $45, $53, $1A, $01, $01, $01, $00, $00, $00, $00, $00, $00, $00, $00, $00

.org $C000

;------------------------------[ Define stuff ]-------------------------------

RIGHT_BUTTON    EQU %00000001
LEFT_BUTTON     EQU %00000010
DOWN_BUTTON     EQU %00000100
UP_BUTTON       EQU %00001000
START_BUTTON    EQU %00010000
SELECT_BUTTON   EQU %00100000
B_BUTTON        EQU %01000000
A_BUTTON        EQU %10000000

; Zero page addresses for where to store data

SND_REGS        EQU     $00     ; data for 4 channels, 4 bytes each
CUR_CHN         EQU     $10     ; current channel (0...3)
CUR_REG         EQU     $11     ; current register of channel (0...3)
CUR_BIT         EQU     $12     ; current bit of register
CUR_CHN_OFFSET	EQU		$13		; current channel * 4
JOY_STAT        EQU     $FC     ; byte containing status of joypad
OLD_STAT        EQU     $FD     ; joypad status from previous refresh
ADDR_LO         EQU     $FE
ADDR_HI         EQU     $FF

UPDATE_REG		EQU     $20		; edit registers or other things


;---------------------------------[ Data ]------------------------------------

soundTest_text	.db "SOUND TEST"
chn_text        .db "CHANNEL"
                .db 0FFh
square_text     .db "SQUARE"
                .db 0FFh
triangle_text   .db "TRIANGLE"
                .db 0FFh
noise_text      .db "NOISE"
                .db 0FFh,0FFh,0FFh 			
inst_text0		.db "DPAD", 074h, 0ffh, 0ffh, 0ffh, "SELECT", 0ffh, "BIT"

inst_text1		.db "A", 074h, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, "TOGGLE", 0ffh, "BIT"

inst_text2		.db "B", 074h, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, "TOGGLE", 0ffh, "UPDATE"

inst_text3		.db "SELECT", 074h, 0ffh, "SWITCH", 0ffh, "CHANNEL"

inst_text4		.db "START", 074h, 0ffh, 0ffh, "UPDATE", 0ffh, "REGISTERS"


palette:        .db 0Fh,11h,0Fh,20h		; 	text
                .db 0Fh,01h,06h,2Ah
                .db 0Fh,0Fh,0Fh,1Ah
                .db 0Fh,0Fh,0Fh,0Ah

                .db 0Fh,07h,0Fh,20h
                .db 0Fh,0Fh,0Fh,20h
                .db 0Fh,0Fh,0Fh,20h
                .db 0Fh,0Fh,0Fh,20h

;---------------------------------[ Code ]------------------------------------

reset:
cld             ; clear decimal mode
sei             ; disable interrupts
lda     #$00
sta     $2000   ; disable various stuff
sta     $2001   ; ditto
ldx     #$FF
txs             ; set stack pointer

jsr     clear_ram
jsr     wait_vblank
jsr     clear_nametable
jsr     set_palette
jsr		set_attribute
jsr		setup_screen
jsr		print_instructions
jsr     print_reg_display ; writes some text info to the name table
jsr		print_regs
jsr		print_channel

jsr 	set_cursors


lda     #%00011110
sta     $2001   ; enable bg & sprites

lda     #%10000000
sta     $2000   ; enable NMI

forever:
jmp forever

;	I am doing sound test things! Fun!
;	I should make it print text based on what it does. 
;	You know, with a text printing system.
;	Like a text printing system i need to make for, you know, the thing...

main_loop:
;jsr     wait_vblank
jsr		sprite_dma

; the next piece of code handles button presses

lda     #RIGHT_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT ; If this AND results in a non-zero result, it means
bne     +        ;  the button was pressed last refresh too, so do nothing.
jsr     right_pressed       ; Do appropriate action for this button
+       ; Repeat for the other 7 buttons...
lda     #LEFT_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT
bne     +
jsr     left_pressed
+
lda     #DOWN_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT
bne     +
jsr     down_pressed
+
lda     #UP_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT
bne     +
jsr     up_pressed
+
lda     #START_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT
bne     +
jsr     start_pressed
+
lda     #SELECT_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT
bne     +
jsr     select_pressed
+
lda     #A_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT
bne     +
jsr     A_pressed
+
lda     #B_BUTTON
and     JOY_STAT
beq     +
and     OLD_STAT
bne     +
jsr     B_pressed
+

lda     #$00
sta     $2005   ; hscroll = 0
sta     $2005   ; vscroll = 0

lda     JOY_STAT
sta     OLD_STAT
jsr     read_joypad

rti

;-----------------------------------------------------------------------------

right_pressed:
dec     CUR_BIT
lda     CUR_BIT
and     #$07
sta     CUR_BIT

jsr     set_cursors     ; show the ChannelCursor and BitCursor

rts

left_pressed:
inc     CUR_BIT
lda     CUR_BIT
and     #$07
sta     CUR_BIT

jsr     set_cursors     ; show the ChannelCursor and BitCursor

rts

down_pressed:
inc     CUR_REG
lda     CUR_REG
and     #$03
sta     CUR_REG

jsr     set_cursors     ; show the ChannelCursor and BitCursor

rts

up_pressed:
dec     CUR_REG
lda     CUR_REG
and     #$03
sta     CUR_REG

jsr     set_cursors     ; show the ChannelCursor and BitCursor

rts

start_pressed:
lda     #$01
ldx     CUR_CHN
beq     +
-
asl
dex
bne     -
+
lda 	#%00001111		;	enable all channels
sta     $4015   		; 	enable sound channel

ldy     #$10
ldx     #$00

playChannels:
lda		UPDATE_REG,x
beq		skipRegister
lda     SND_REGS,x
sta     $4000,x
skipRegister:
inx
dey
bne     playChannels
rts

select_pressed:
inc     CUR_CHN
lda     CUR_CHN
and     #$03
sta     CUR_CHN
asl
asl
sta		CUR_CHN_OFFSET

jsr		print_channel
jsr     print_reg_display
jsr     print_regs      ; print the regs in binary format

rts

A_pressed:
lda     #$01
ldx     CUR_BIT
beq     +
clc
-
asl
dex
bne     -
+
pha
lda     CUR_CHN
asl
asl
clc
adc     CUR_REG
tax
pla
eor     SND_REGS,x
sta     SND_REGS,x

;	Reprint REGS
jsr     print_regs      ; print the regs in binary format
rts

B_pressed:

lda     CUR_CHN
asl
asl
clc
adc		CUR_REG
tax
lda 	UPDATE_REG, x
eor		#%00000001
sta		UPDATE_REG, x

jsr		print_regs     ; print the regs in binary format

rts

;-----------------------------------------------------------------------------

print_regs:
lda     #$20
sta     ADDR_HI
lda     #$CA
sta     ADDR_LO

lda     CUR_CHN
asl
asl
tax

print_one_reg:
lda     ADDR_HI
sta     $2006
lda     ADDR_LO
sta     $2006
lda     SND_REGS,x
ldy     #$08
-
asl
pha
lda     #$00
rol
ora     #$20
sta     $2007
pla
dey
bne     -

print_byte:


print_update_register:
lda		#$00
sta		$2007

ldy		#$33
lda		UPDATE_REG,x
beq		registerDisabled
dey	
registerDisabled:
sty		$2007

lda     ADDR_LO
clc
adc     #$40
sta     ADDR_LO
bcc     +
inc     ADDR_HI
+
inx
txa
and     #$03
bne     print_one_reg
rts

print_channel:
lda     #$20
sta     $2006
lda     #$62
sta     $2006

ldx 	#$00

lda		CUR_CHN
cmp		#$03
beq		print_noise
cmp		#$02
beq		print_triangle
; else just print square and number

print_square:
lda		square_text,x
sbc		#$3F
sta		$2007
inx
cpx		#$06
bne		print_square:

lda		#$00
sta		$2007

lda     CUR_CHN			;	print square number
ora     #$20
;adc		#$20
sta     $2007

rts

print_triangle:
clc
lda		triangle_text,x
sbc		#$3F
sta		$2007
inx
cpx		#$08
bne		print_triangle:

rts

print_noise:
clc
lda		noise_text,x
sbc		#$3F
sta		$2007
inx
cpx		#$08
bne		print_noise:

rts


set_cursors:
;	Do DMA instead of this crap
;lda     #$00
;sta     $2003
lda     CUR_REG
asl
asl
asl
asl
clc
adc     #$2F
sta     $300
lda     #$30
sta     $301
lda     #$00
sta     $302
lda     #$47
sta     $303

lda     CUR_REG
asl
asl
asl
asl
clc
adc     #$36
sta     $304
lda     #$31
sta     $305
lda     #$00
sta     $306
lda     CUR_BIT
eor     #$07
asl
asl
asl
clc
adc     #$51
sta     $307

rts

sprite_dma:
lda 	#$00
sta		$2003
lda		#$03
sta		$4014

rts

setup_screen:

;	turn on blip
inc 	$03
inc		$23

lda     #$20
sta     $2006
lda     #$42
sta     $2006
ldx     #$00
-
lda     chn_text,x
cmp     #$FF
beq     end_text
sbc		#$3F
sta     $2007
inx
bne     -
end_text:

print_reg_display:

lda     #$20
sta     ADDR_HI
lda     #$C2
sta     ADDR_LO
ldy     #$20
-
lda     ADDR_HI
sta     $2006
lda     ADDR_LO
sta     $2006
lda     #$2B    ; "$"
sta     $2007
lda     #$24    ; "4"
sta     $2007
lda     #$20    ; "0"
sta     $2007
sta     $2007

tya
clc
adc		CUR_CHN_OFFSET
cmp		#$2A
bmi +	
sbc		#$29		
+
sta     $2007
lda     ADDR_LO
clc
adc     #$40
sta     ADDR_LO
bcc     +
inc     ADDR_HI
+
iny
cpy     #$24
bne     -
rts

print_instructions:
lda     #$22
sta     $2006
lda     #$22
sta     $2006

clc
ldx 	#$00
print_inst_text0:
lda		inst_text0,x
sbc		#$3F
sta		$2007
inx
cpx		#$12
bne		print_inst_text0:


lda     #$22
sta     $2006
lda     #$62
sta     $2006

clc
ldx 	#$00
print_inst_text1:
lda		inst_text1,x
sbc		#$3F
sta		$2007
inx
cpx		#$12
bne		print_inst_text1:


lda     #$22
sta     $2006
lda     #$A2
sta     $2006

clc
ldx 	#$00
print_inst_text2:
lda		inst_text2,x
sbc		#$3F
sta		$2007
inx
cpx		#$15
bne		print_inst_text2:

lda     #$22
sta     $2006
lda     #$E2
sta     $2006

clc
ldx 	#$00
print_inst_text3:
lda		inst_text3,x
sbc		#$3F
sta		$2007
inx
cpx		#$16
bne		print_inst_text3:

lda     #$23
sta     $2006
lda     #$22
sta     $2006

clc
ldx 	#$00
print_inst_text4:
lda		inst_text4,x
sbc		#$3F
sta		$2007
inx
cpx		#$18
bne		print_inst_text4:

clc
rts


wait_vblank:
lda     $2002
bpl     wait_vblank
rts

; Clear RAM at $0000-$07FF
; ========================

clear_ram:
lda     #$07    ; high byte of last RAM page
sta     ADDR_HI
lda     #$00    ; low byte of last RAM page
sta     ADDR_LO
clear_it:
ldy     ADDR_HI ; load high byte of address
cpy     #$01    ; if it equals 1...
beq     +       ; ... skip (don't mess with stack at $0100-$01FF)
ldy     #$00
-
sta     (ADDR_LO),y
iny
bne     -
+
dec     ADDR_HI ; decrement high byte of address
bpl     clear_it
rts

; Read joypad
; ===========
; Returns: JOY_STAT = status of all buttons

read_joypad:
ldy     #$01
sty     $4016   ; reset strobe
dey
sty     $4016   ; clear strobe
;sty     JOY_STAT ; JOY_STAT = 0 (clear all button bits)
ldy     #$08    ; do all 8 buttons

read_button:
lda     $4016   ; load button status
and     #$01    ; only keep lowest bit
lsr     a       ; transfer to carry flag
rol     JOY_STAT
dey
bne     read_button
rts

; Clear nametable
; ===============
; A     : Select nametable (range 0...3)

clear_nametable:
lda     #$20
sta     $2006   ; write high byte of name table address
lda     #$00
sta     $2006   ; write low byte of name table address

ldx     #$04
ldy     #$00
lda     #$00
-
sta     $2007
dey
bne     -
dex
bne     -
rts

; Set attribute
; ===========
set_attribute:
lda		#%01000100	; Green or red

ldx     #$23
stx     $2006
ldx     #$CC
stx     $2006
sta		$2007

ldx     #$23
stx     $2006
ldx     #$D4
stx     $2006
sta		$2007

ldx     #$23
stx     $2006
ldx     #$DC
stx     $2006
sta		$2007

rts

; Set palette
; ===========

set_palette:
lda     #$3F
sta     $2006
lda     #$00
sta     $2006
ldx     #$00
ldy     #$20
-
lda     palette,x
sta     $2007
inx
dey
bne     -
rts

nmi:
jmp main_loop	; This is fine...


.org $FFFA

.dw nmi,reset,reset

.org $10000
.incbin "SNDTEST2.CHR"

;.end
