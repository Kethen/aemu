/*
 * This file is part of PRO ONLINE.

 * PRO ONLINE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO ONLINE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO ONLINE. If not, see <http://www.gnu.org/licenses/ .
 */

// Parts of this code were taken from PSPHUD and all credits go to darko79!

#include <string.h>
#include <pspdisplay.h>
#include "draw.h"

static u16 convert_8888_to_565(u32 color)
{
        int r, g, b;

        b = (color >> 19) & 0x1F;
        g = (color >> 10) & 0x3F;
        r = (color >> 3) & 0x1F;

        return r | (g << 5) | (b << 11);
}

static u16 convert_8888_to_5551(u32 color)
{
        int r, g, b, a;

        a = (color >> 24) ? 0x8000 : 0;
        b = (color >> 19) & 0x1F;
        g = (color >> 11) & 0x1F;
        r = (color >> 3) & 0x1F;

        return a | r | (g << 5) | (b << 10);
}

static u16 convert_8888_to_4444(u32 color)
{
        int r, g, b, a;

        a = (color >> 28) & 0xF; 
        b = (color >> 20) & 0xF;
        g = (color >> 12) & 0xF;
        r = (color >> 4) & 0xF;

        return (a << 12) | r | (g << 4) | (b << 8);
}

static u16 convert_u32_to_u16(u32 color, int pixelFormat)
{
	switch(pixelFormat) {
		case PSP_DISPLAY_PIXEL_FORMAT_565: return convert_8888_to_565(color);
		case PSP_DISPLAY_PIXEL_FORMAT_5551: return convert_8888_to_5551(color);
		case PSP_DISPLAY_PIXEL_FORMAT_4444: return convert_8888_to_4444(color);
	}
	return convert_8888_to_565(color);
}


int getCanvas(CANVAS *canvas)
{
	if(canvas != NULL) {
		int mode = 0;
		sceDisplayGetMode(&mode, &(canvas->width), &(canvas->height));
   		sceDisplayGetFrameBuf((void*)&(canvas->buffer), &(canvas->lineWidth), &(canvas->pixelFormat), 0);
		if(canvas->width > 0 && canvas->height > 0 && canvas->lineWidth > 0 && canvas->buffer != NULL) return 1;
	}
	return 0;
}

/**************************************
             SMALL FONT
 *************************************/

// x - WHITE
// - - BLACK
// R - RED
// G - GREEN
// B - BLUE
// Y - YELLOW

static char small_font[][10] = {
	// (00) 0x00
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (01) 0x01
	"       " ,
	"-------" ,
    "-xxxx-x" ,
    "-x----x" ,
    "-x----x" ,
	"-xxxx-x" ,
    "-------" ,
	// (02) 0x02
	"       " ,
	"-------" ,
    "xxxx-x-" ,
    "x--x-x-" ,
    "xxxx-x-" ,
	"x----xx" ,
    "-------" ,
	// (03) 0x03
	"       " ,
	"----   " ,
    "--x-   " ,
    "--x-   " ,
    "--x-   " ,
	"xxx-   " ,
    "----   " ,
	// (04) 0x04
	"       " ,
	"-------" ,
    "-xxx--x" ,
    "-xxxx-x" ,
    "-x--x-x" ,
	"-xxxx-x" ,
    "-------" ,
	// (05) 0x05
	"       " ,
	"-------" ,
    "x--x-xx" ,
    "x--x-xx" ,
    "x--x---" ,
	"xxxx-xx" ,
    "-------" ,
	// (06) 0x06
	"       " ,
	"----   " ,
    "xxx-   " ,
    "x---   " ,
    "-xx-   " ,
	"xxx-   " ,
    "----   " ,
	// (07) 0x07
	"       " ,
	"       " ,
    "-------" ,
    "-xxxxx-" ,
    "-x-x-x-" ,
	"-x-x-x-" ,
    "-------" ,
	// (08) 0x08
	"       " ,
	"       " ,
    "-------" ,
    "-x--x--" ,
    "-xxxx--" ,
	"-x--x--" ,
    "-------" ,
	// (09) 0x09
	"       " ,
	"       " ,
    "----   " ,
    "xxx-   " ,
    "-x--   " ,
	"xxx-   " ,
    "----   " ,
	// (10) 0x0A
	"       " ,
	"-------" ,
    "-xxx-xx" ,
    "-x---x-" ,
    "-xxx-xx" ,
	"-x---x-" ,
    "-------" ,
	// (11) 0x0B
	"       " ,
	"-------" ,
    "xx-xxx-" ,
    "-x-xxx-" ,
    "xx---x-" ,
	"---xxx-" ,
    "-------" ,
	// (12) 0x0C
	"       " ,
	"-------" ,
    "-xxx-xx" ,
    "-x-x-x-" ,
    "-xxx-x-" ,
	"-x---x-" ,
    "--- ---" ,
	// (13) 0x0D
	"       " ,
	"-----  " ,
    "xxxx-  " ,
    "-x-x-  " ,
    "-x-x-  " ,
	"-x-x-  " ,
    "-----  " ,
	// (14) 0x0E
	"       " ,
	"-------" ,
    "-xxx-xx" ,
    "-x-x-x-" ,
    "-xxx-x-" ,
	"-x-x-x-" ,
    "-------" ,
	// (15) 0x0F
	"       " ,
	"-----  " ,
    "xxxx-  " ,
    "-x-x-  " ,
    "-x-x-  " ,
	"-x-x-  " ,
    "-----  " ,
	// (16) 0x10
	"  ---  " ,
	"  -x-- " ,
    "----x--" ,
    "-xxxxx-" ,
    "----x--" ,
	"  -x-- " ,
    "  ---  " ,
	// (17) 0x11
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (18) 0x12
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (19) 0x13
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (20) 0x14
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (21) 0x15
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (22) 0x16
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (23) 0x17
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (24) 0x18
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (25) 0x19
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// (26) 0x1A
	" ----  " ,
	"--GG-- " ,
    "-GGGG- " ,
    "-GGGG- " ,
    "-GGGG- " ,
	"-GGGG- " ,
    "------ " ,
	// (27) 0x1B
	" ----  " ,
	"--YY-- " ,
    "-YYYY- " ,
    "-YYYY- " ,
    "-YYYY- " ,
    "-YYYY- " ,
    "------ " ,
	// (28) 0x1C
	" ----  " ,
	"--RR-- " ,
    "-RRRR- " ,
    "-RRRR- " ,
    "-RRRR- " ,
	"-RRRR- " ,
    "------ " ,
	// (29) 0x1D
	" ----  " ,
	"R-xx-- " ,
    "-R-xx- " ,
    "--R-x- " ,
    "-x-R-- " ,
	"-xx-R- " ,
    "-----R " ,
	// (30) 0x1E
	"  -----" ,
	" --xx-x" ,
    " -xxx-x" ,
    " -xxx--" ,
    " -xxxxx" ,
	" --xxxx" ,
    "  -----" ,
	// (31) 0x1F
	"----   " ,
	"xxx--  " ,
    "xxxx-  " ,
    "---x-  " ,
    "xxxx-  " ,
	"xxx--  " ,
    "----   " ,
	//  (32) 0x20
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// ! (33) 0x21
	"  ---  " ,
	"  -x-  " ,
    "  -x-  " ,
    "  -x-  " ,
    "  ---  " ,
	"  -x-  " ,
    "  ---  " ,
	// " (34) 0x22
	" ----- " ,
	" -x-x- " ,
    " ----- " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// # (35) 0x23
	" ----- " ,
	"--x-x--" ,
    "-xxxxx-" ,
    "--x-x--" ,
    "-xxxxx-" ,
	"--x-x--" ,
    " ----- " ,
	// $ (36) 0x24
	" --x---" ,
	"--xxxx-" ,
    "-x x---" ,
    "--xxx--" ,
    "---x-x-" ,
	"-xxxx--" ,
    "---x-- " ,
	// % (37) 0x25
	"       " ,
	"------ " ,
    "-x--x- " ,
    " --x-- " ,
    "--x--- " ,
	"-x--x- " ,
    "------ " ,
	// & (38) 0x26
	" ------" ,
	"--xxxx-" ,
    "--x----" ,
    "--xx---" ,
    "-x--xx-" ,
	"-xxx-x-" ,
    "-------" ,
	// ' (39) 0x27
	"  ---  " ,
	"  -x-  " ,
    "  ---  " ,
    "       " ,
    "       " ,
	"       " ,
    "       " ,
	// ( (40) 0x28
	"  ---  " ,
	" --x-  " ,
    " -x--  " ,
    " -x-   " ,
    " -x--  " ,
	" --x-  " ,
    "  ---  " ,
	// ) (41) 0x29
	"  ---  " ,
	"  -x-- " ,
    "  --x- " ,
    "   -x- " ,
    "  --x- " ,
	"  -x-- " ,
    "  ---  " ,
	// * (42) 0x2A
	"-------" ,
	"-x-x-x-" ,
    "--xxx--" ,
    "-xxxxx-" ,
    "--xxx--" ,
	"-x-x-x-" ,
    "-------" ,
	// + (43) 0x2B
	"  ---  " ,
	"  -x-  " ,
    "---x---" ,
    "-xxxxx-" ,
    "---x---" ,
	"  -x-  " ,
    "  ---  " ,
	// , (44) 0x2C
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "  ---  " ,
	" --x-  " ,
    " -x--  " ,
	// - (45) 0x2D
	"       " ,
	"       " ,
    "-------" ,
    "-xxxxx-" ,
    "-------" ,
	"       " ,
    "       " ,
	// . (46) 0x2E
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "  ---  " ,
	"  -x-  " ,
    "  ---  " ,
	// / (47) 0x2F
	"    ---" ,
	"   --x-" ,
    "  --x--" ,
    " --x-- " ,
    "--x--  " ,
	"-x--   " ,
    "---    " ,
	// 0 (48) 0x30
	"-------" ,
    "-xxxxx-" ,
    "-x---x-" ,
    "-x- -x-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// 1 (49) 0x31
	"    ---" ,
	"    -x-" ,
    "    -x-" ,
    "    -x-" ,
    "    -x-" ,
	"    -x-" ,
    "    ---" ,
	// 2 (50) 0x32
	"-------" ,
	"-xxxxx-" ,
    "-----x-" ,
    "-xxxxx-" ,
    "-x-----" ,
	"-xxxxx-" ,
    "-------" ,
	// 3 (51) 0x33
	"-------" ,
	"-xxxxx-" ,
    "-----x-" ,
    "-xxxxx-" ,
    "-----x-" ,
	"-xxxxx-" ,
    "-------" ,
	// 4 (52) 0x34
	"--- ---" ,
	"-x- -x-" ,
    "-x---x-" ,
    "-xxxxx-" ,
    "-----x-" ,
	"    -x-" ,
    "    ---" ,
	// 5 (53) 0x35
	"-------" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-xxxxx-" ,
    "-----x-" ,
	"-xxxxx-" ,
    "-------" ,
	// 6 (54) 0x36
	"-------" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-xxxxx-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// 7 (55) 0x37
	"-------" ,
	"-xxxxx-" ,
    "-----x-" ,
    "    -x-" ,
    "    -x-" ,
	"    -x-" ,
    "    ---" ,
	// 8 (56) 0x38
	"-------" ,
	"-xxxxx-" ,
    "-x---x-" ,
    "-xxxxx-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// 9 (57) 0x39
	"-------" ,
	"-xxxxx-" ,
    "-x---x-" ,
    "-xxxxx-" ,
    "-----x-" ,
	"-xxxxx-" ,
	"-------" ,
	// : (58) 0x3A
	"       " ,
	"  ---  " ,
    "  -x-  " ,
    "  ---  " ,
    "  -x-  " ,
	"  ---  " ,
    "       " ,
	// ; (59) 0x3B
	"       " ,
	"  ---  " ,
    "  -x-  " ,
    "  ---  " ,
    " --x-  " ,
	" -x--  " ,
    " ---   " ,
	// < (60) 0x3C
	"   --- " ,
	"  --x- " ,
    " --x-- " ,
    " -x--  " ,
    " --x-- " ,
	"  --x- " ,
    "   --- " ,
	// = (61) 0x3D
	"       " ,
	"-------" ,
    "-xxxxx-" ,
    "-------" ,
    "-xxxxx-" ,
	"-------" ,
    "       " ,
	// > (62) 0x3E
	" ---   " ,
	" -x--  " ,
    " --x-- " ,
    "  --x- " ,
    " --x-- " ,
	" -x--  " ,
    " ---   " ,
	// ? (63) 0x3F
	"-------" ,
	"-xxxxx-" ,
    "----xx-" ,
    "  -x---" ,
    "  ---  " ,
	"  -x-  " ,
    "  ---  " ,
	// @ (64) 0x40
	" ----- " ,
	"--xxx--" ,
    "-x-x-x-" ,
    "-x-xxx-" ,
    "-x-----" ,
	"--xxx--" ,
    " ----- " ,
	// A (65) 0x41
	"-------" ,
	"-xxxxx-" ,
    "-x---x-" ,
    "-xxxxx-" ,
    "-x---x-" ,
	"-x- -x-" ,
    "--- ---" ,
	// B (66) 0x42
	"------ " ,
	"-xxxx--" ,
    "-x---x-" ,
    "-xxxx--" ,
    "-x---x-" ,
	"-xxxx--" ,
    "------ " ,
	// C (67) 0x43
	"-------" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-x-    " ,
    "-x-----" ,
	"-xxxxx-" ,
    "-------" ,
	// D (68) 0x44
	"------ " ,
	"-xxxx--" ,
    "-x---x-" ,
    "-x- -x-" ,
    "-x---x-" ,
	"-xxxx--" ,
    "------ " ,
	// E (69) 0x45
	"-------" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-xxxxx-" ,
    "-x-----" ,
	"-xxxxx-" ,
    "-------" ,
	// F (70) 0x46
	"-------" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-xxxxx-" ,
    "-x-----" ,
	"-x-    " ,
    "---    " ,
	// G (71) 0x47
	"-------" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-x-xxx-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// H (72) 0x48
	"--- ---" ,
	"-x- -x-" ,
    "-x---x-" ,
    "-xxxxx-" ,
    "-x---x-" ,
	"-x- -x-" ,
    "--- ---" ,
	// I (73) 0x49
	"  ---  " ,
	"  -x-  " ,
    "  -x-  " ,
    "  -x-  " ,
    "  -x-  " ,
	"  -x-  " ,
    "  ---  " ,
	// J (74) 0x4A
	"-------" ,
	"-xxxxx-" ,
    "-----x-" ,
    "--- -x-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// K (75) 0x4B
	"--- ---" ,
	"-x---x-" ,
    "-x--x--" ,
    "-xxx-- " ,
    "-x--x--" ,
	"-x---x-" ,
    "--- ---" ,
	// L (76) 0x4C
	"---    " ,
	"-x-    " ,
    "-x-    " ,
    "-x-    " ,
    "-x-----" ,
	"-xxxxx-" ,
    "-------" ,
	// M (77) 0x4D
	"-------" ,
	"-xxxxx-" ,
    "-x-x-x-" ,
    "-x-x-x-" ,
    "-x-x-x-" ,
	"-x-x-x-" ,
    "-------" ,
	// N (78) 0x4E
	"--- ---" ,
	"-x---x-" ,
    "-xx--x-" ,
    "-x-x-x-" ,
    "-x--xx-" ,
	"-x---x-" ,
    "--- ---" ,
	// O (79) 0x4F
	"-------" ,
	"-xxxxx-" ,
    "-x---x-" ,
    "-x- -x-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// P (80) 0x50
	"-------" ,
	"-xxxxx-" ,
    "-x---x-" ,
    "-xxxxx-" ,
    "-x-----" ,
	"-x-    " ,
    "---    " ,
	// Q (81) 0x51
	"-------" ,
	"-xxxxx-" ,
    "-x---x-" ,
    "-x-x-x-" ,
    "-x--xx-" ,
	"-xxxxx-" ,
    "-------" ,
	// R (82) 0x52
	"------ " ,
	"-xxxx--" ,
    "-x---x-" ,
    "-xxxx--" ,
    "-x---x-" ,
	"-x- -x-" ,
    "--- ---" ,
	// S (83) 0x53
	"-------" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-xxxxx-" ,
    "-----x-" ,
	"-xxxxx-" ,
    "-------" ,
	// T (84) 0x54
	"-------" ,
	"-xxxxx-" ,
    "---x---" ,
    "  -x-  " ,
    "  -x-  " ,
	"  -x-  " ,
    "  ---  " ,
	// U (85) 0x55
	"--- ---" ,
	"-x- -x-" ,
    "-x- -x-" ,
    "-x- -x-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// V (86) 0x56
	"--- ---" ,
	"-x- -x-" ,
    "-x- -x-" ,
    "-x---x-" ,
    "--x-x--" ,
	" --x-- " ,
    "  ---  " ,
	// W (87) 0x57
	"-------" ,
	"-x-x-x-" ,
    "-x-x-x-" ,
    "-x-x-x-" ,
    "-x-x-x-" ,
	"-xxxxx-" ,
    "-------" ,
	// X (88) 0x58
	"--- ---" ,
	"-x- -x-" ,
    "-x---x-" ,
    "--xxx--" ,
    "-x---x-" ,
	"-x- -x-" ,
    "--- ---" ,
	// Y (89) 0x59
	"--- ---" ,
	"-x- -x-" ,
    "-x---x-" ,
    "--xxx--" ,
    " --x-- " ,
	"  -x-  " ,
    "  ---  " ,
	// Z (90) 0x5A
	"-------" ,
	"-xxxxx-" ,
    "----x--" ,
    " --x-- " ,
    "--x----" ,
	"-xxxxx-" ,
    "-------" ,
	// [ (91) 0x5B
	" ----- " ,
	" -xxx- " ,
    " -x--- " ,
    " -x-   " ,
    " -x--- " ,
	" -xxx- " ,
    " ----- " ,
	// \ (92) 0x5C
	"---    " ,
	"-x--   " ,
    "--x--  " ,
    " --x-- " ,
    "  --x--" ,
	"   --x-" ,
    "    ---" ,
	// ] (93) 0x5D
	" ----- " ,
	" -xxx- " ,
    " ---x- " ,
    "  --x- " ,
    " ---x- " ,
	" -xxx- " ,
    " ----- " ,
	// ^ (94) 0x5E
	"  ---  " ,
	" --x-- " ,
    " -x-x- " ,
    " ----- " ,
    "       " ,
	"       " ,
    "       " ,
	// _ (95) 0x5F
	"       " ,
	"       " ,
    "       " ,
    "       " ,
    "-------" ,
	"-xxxxx-" ,
    "-------" ,

	// (96) 0x60
	"--x-x--" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-x-    " ,
    "-x-----" ,
	"-xxxxx-" ,
    "-------" ,
	// (97) 0x61
	"----x--" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-x-    " ,
    "-x-----" ,
	"-xxxxx-" ,
    "-------" ,
	// (98) 0x62
	"------ " ,
	"-xxxx--" ,
    "-x---x-" ,
    "xxx--x-" ,
    "-x---x-" ,
	"-xxxx--" ,
    "------ " ,
	// (99) 0x63
	"--x-x--" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-xxxxx-" ,
    "-----x-" ,
	"-xxxxx-" ,
    "-------" ,
	// (100) 0x64
	"--x-x--" ,
	"-xxxxx-" ,
    "----x--" ,
    " --x-- " ,
    "--x----" ,
	"-xxxxx-" ,
    "-------" ,
	// (101) 0x65
	"--xxx--" ,
	"-xxxxx-" ,
    "-x---x-" ,
    "-xxxxx-" ,
    "-x---x-" ,
	"-x- -x-" ,
    "--- ---" ,
	// (102) 0x66
	"----x--" ,
	"-xxxxx-" ,
    "-x-----" ,
    "-xxxxx-" ,
    "-x-----" ,
	"-xxxxx-" ,
    "-------" ,
	// (103) 0x67
	"----x- " ,
	"-xxxx--" ,
    "-x---x-" ,
    "-xxxx--" ,
    "-x---x-" ,
	"-x- -x-" ,
    "--- ---" ,
	// (104) 0x68
	"----x--" ,
	"-x-x-x-" ,
    "-xx--x-" ,
    "-x-x-x-" ,
    "-x--xx-" ,
	"-x---x-" ,
    "--- ---" ,
	// (105) 0x69
	"----x--" ,
	"-x-x-x-" ,
    "-x- -x-" ,
    "-x- -x-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// (106) 0x6A
	"----x--" ,
	"-xxxxx-" ,
    "-x- -x-" ,
    "-x- -x-" ,
    "-x---x-" ,
	"-xxxxx-" ,
    "-------" ,
	// (107) 0x6B
	"------ " ,
	"-xxx---" ,
    "-x--x--" ,
    "-x-xx--" ,
    "-x---x-" ,
	"-x-xx--" ,
    "-x---- " ,
    	// (108) 0x6C <3 Symbol
    	"       ",
    	" -- -- ",
    	"-RR-RR-",
    	"-RRRRR-",
    	" -RRR- ",
    	"  -R-  ",
    	"   -   ",
	};

/**************************************
             SMALL FONT
 *************************************/

char convertChar(unsigned char c)
{
	if(c >= 96 && c <= 127) return c - 32;
	if(c == 0xE8 || c == 0xC8) return 0x60;
	if(c == 0xE6 || c == 0xC6) return 0x61;
	if(c == 0xF0 || c == 0xD0) return 0x62;
	if(c == 0x9A || c == 0x8A) return 0x63;
	if(c == 0x9E || c == 0x8E) return 0x64;
    if(c == 0xE2 || c == 0xC2) return 0x65;
    if(c == 0xE9 || c == 0xC9) return 0x66;
    if(c == 0xE0 || c == 0xC0) return 0x67;
    if(c == 0xF1 || c == 0xD1) return 0x68;
    if(c == 0xDA || c == 0xDA) return 0x69;
    if(c == 0xD3 || c == 0xD3) return 0x6A;
    if(c == 0xDF || c == 0xDF) return 0x6B;
    
    // Heart Symbol
    if(c == 0x80) return 0x6C;
	return c;
}



u32 WHITE_8888  = RGB_8888(255, 255, 255);
u32 BLACK_8888  = RGB_8888(0, 0, 0);
u32 RED_8888    = RGB_8888(255, 0, 0);
u32 GREEN_8888  = RGB_8888(0, 255, 0);
u32 BLUE_8888   = RGB_8888(0, 0, 255);
u32 YELLOW_8888 = RGB_8888(255, 255, 0);

u16 WHITE_565  = RGB_565(255, 255, 255);
u16 BLACK_565  = RGB_565(0, 0, 0);
u16 RED_565    = RGB_565(255, 0, 0);
u16 GREEN_565  = RGB_565(0, 255, 0);
u16 BLUE_565   = RGB_565(0, 0, 255);
u16 YELLOW_565 = RGB_565(255, 255, 0);

u16 WHITE_5551  = RGB_5551(255, 255, 255);
u16 BLACK_5551  = RGB_5551(0, 0, 0);
u16 RED_5551    = RGB_5551(255, 0, 0);
u16 GREEN_5551  = RGB_5551(0, 255, 0);
u16 BLUE_5551   = RGB_5551(0, 0, 255);
u16 YELLOW_5551 = RGB_5551(255, 255, 0);

u16 WHITE_4444  = RGB_4444(255, 255, 255);
u16 BLACK_4444  = RGB_4444(0, 0, 0);
u16 RED_4444    = RGB_4444(255, 0, 0);
u16 GREEN_4444  = RGB_4444(0, 255, 0);
u16 BLUE_4444   = RGB_4444(0, 0, 255);
u16 YELLOW_4444 = RGB_4444(255, 255, 0);

int draw_min_x = -1;
int draw_max_x = -1;

void drawSmallFontVertical(CANVAS *canvas, const char *text, int x, int y, u32 fillcolor)
{
	// Iterate Characters
	int i = 0; for(; i < strlen(text); i++)
	{
		// Wrapper String
		char string[2]; string[0] = text[i]; string[1] = 0;
		
		// Print Character
		drawSmallFont(canvas, string, x, y + i * 6, fillcolor);
	}
}

void drawSmallFont(CANVAS *canvas, const char *text, int x, int y, u32 fillcolor)
{
	draw_min_x = -1;
	draw_max_x = -1;
	if(canvas != NULL && text != NULL && canvas->width > 0 && canvas->height > 0 && canvas->lineWidth > 0 && canvas->buffer != NULL) {
		draw_min_x = x;
		u32 *vram32, *vram32_ptr;
		u16 *vram16, *vram16_ptr;
		u16 fillcolor16 = convert_u32_to_u16(fillcolor, canvas->pixelFormat);
		int c, i, j, len;
		char *font;
		len = strlen(text);
		for(c = 0; c < len; c++) {
			if(x < 0 || x + 8 > canvas->width || y < 0 || y + 8 > canvas->height) break;
			int ch = (int)text[c];
			if(ch >= 1 && ch <= 108) {
				switch(canvas->pixelFormat) {

					case PSP_DISPLAY_PIXEL_FORMAT_8888:
						vram32 = canvas->buffer + x + y * canvas->lineWidth;				
						for(i = 0; i < 7; i++) {
							vram32_ptr = vram32;
							font = small_font[((int)ch) * 7 + i];
							for (j = 0; j < 7; j++) {
								switch(font[j]) {
									case 'x': *vram32_ptr = fillcolor; break;
									case '-': *vram32_ptr = BLACK_8888; break;
									case 'R': *vram32_ptr = RED_8888; break;
									case 'G': *vram32_ptr = GREEN_8888; break;
									case 'B': *vram32_ptr = BLUE_8888; break;
									case 'Y': *vram32_ptr = YELLOW_8888; break;
								}
								vram32_ptr++;
							}
							vram32 += canvas->lineWidth;
						}
						break;

					case PSP_DISPLAY_PIXEL_FORMAT_565:
						vram16 = (u16*)canvas->buffer + x * canvas->scale + y * canvas->lineWidth * canvas->scale;
						for(i = 0; i < 7 * canvas->scale; i++) {
							vram16_ptr = vram16;
							font = small_font[((int)ch) * 7 + i / canvas->scale];
							for (j = 0; j < 7 * canvas->scale; j++) {
								switch(font[j / canvas->scale]) {
									case 'x': *vram16_ptr = fillcolor16; break;
									case '-': *vram16_ptr = BLACK_565; break;
									case 'R': *vram16_ptr = RED_565; break;
									case 'G': *vram16_ptr = GREEN_565; break;
									case 'B': *vram16_ptr = BLUE_565; break;
									case 'Y': *vram16_ptr = YELLOW_565; break;
								}
								vram16_ptr++;
							}
							vram16 += canvas->lineWidth;
						}
						break;

					case PSP_DISPLAY_PIXEL_FORMAT_5551:
						vram16 = (u16*)canvas->buffer + x + y * canvas->lineWidth;				
						for(i = 0; i < 7; i++) {
							vram16_ptr = vram16;
							font = small_font[((int)ch) * 7 + i];
							for (j = 0; j < 7; j++) {
								switch(font[j]) {
									case 'x': *vram16_ptr = fillcolor16; break;
									case '-': *vram16_ptr = BLACK_5551; break;
									case 'R': *vram16_ptr = RED_5551; break;
									case 'G': *vram16_ptr = GREEN_5551; break;
									case 'B': *vram16_ptr = BLUE_5551; break;
									case 'Y': *vram16_ptr = YELLOW_5551; break;
								}
								vram16_ptr++;
							}
							vram16 += canvas->lineWidth;
						}
						break;

					case PSP_DISPLAY_PIXEL_FORMAT_4444:
						vram16 = (u16*)canvas->buffer + x + y * canvas->lineWidth;				
						for(i = 0; i < 7; i++) {
							vram16_ptr = vram16;
							font = small_font[((int)ch) * 7 + i];
							for (j = 0; j < 7; j++) {
								switch(font[j]) {
									case 'x': *vram16_ptr = fillcolor16; break;
									case '-': *vram16_ptr = BLACK_4444; break;
									case 'R': *vram16_ptr = RED_4444; break;
									case 'G': *vram16_ptr = GREEN_4444; break;
									case 'B': *vram16_ptr = BLUE_4444; break;
									case 'Y': *vram16_ptr = YELLOW_4444; break;
								}
								vram16_ptr++;
							}
							vram16 += canvas->lineWidth;
						}
						break;
				}
			}
			x += 6;
		}
		draw_max_x = x;
	}
}

void fillRectangle(CANVAS *canvas, int x, int y, int width, int height, u32 fill, u32 border)
{
	if(canvas != NULL && canvas->width > 2 && canvas->height > 2 && canvas->lineWidth > 0 && canvas->buffer != NULL && width > 0 && height > 0) {
		u32 *vram32, *vram32_ptr;
		u16 *vram16, *vram16_ptr;
		int i, j;
		u16 fill16 = convert_u32_to_u16(fill, canvas->pixelFormat);
		u16 border16 = convert_u32_to_u16(border, canvas->pixelFormat);
		
		switch(canvas->pixelFormat) {

			case PSP_DISPLAY_PIXEL_FORMAT_8888:
				vram32 = canvas->buffer + x + y * canvas->lineWidth;
				vram32_ptr = vram32;
				for(i = 0; i < width; i++, vram32_ptr++) *vram32_ptr = border;
				vram32 += canvas->lineWidth;
				for(j = 0; j < (height - 2); j++) {
					vram32_ptr = vram32;
					*vram32_ptr = border; vram32_ptr++;
					for(i = 0; i < (width - 2); i++, vram32_ptr++) *vram32_ptr = fill;
					*vram32_ptr = border;
					vram32 += canvas->lineWidth;
				}
				vram32_ptr = vram32;
				for(i = 0; i < width; i++, vram32_ptr++) *vram32_ptr = border;
				break;

			case PSP_DISPLAY_PIXEL_FORMAT_565:
			case PSP_DISPLAY_PIXEL_FORMAT_5551:
			case PSP_DISPLAY_PIXEL_FORMAT_4444:

				vram16 = (u16*)canvas->buffer + x * canvas->scale + y * canvas->lineWidth * canvas->scale;
				// top border
				for (int k = 0;k < canvas->scale;k++){
					vram16_ptr = vram16;
					for(i = 0; i < width * canvas->scale; i++, vram16_ptr++) *vram16_ptr = border16;
					vram16 += canvas->lineWidth;
				}
				for(j = 0; j < (height - 2) * canvas->scale; j++) {
					vram16_ptr = vram16;
					// left border
					for(int k = 0;k < canvas->scale;k++){
						*vram16_ptr = border16; vram16_ptr++;
					}
					// fill
					for(i = 0; i < (width - 2) * canvas->scale; i++, vram16_ptr++) *vram16_ptr = fill16;
					// right border
					for(int k = 0;k < canvas->scale;k++){
						*vram16_ptr = border16; vram16_ptr++;
					}
					vram16 += canvas->lineWidth;
				}
				// bottom border
				for (int k = 0;k < canvas->scale;k++){
					vram16_ptr = vram16;
					for(i = 0; i < width * canvas->scale; i++, vram16_ptr++) *vram16_ptr = border16;
					vram16 += canvas->lineWidth;
				}
				break;
		}

	}
}

void canvasCopy(CANVAS *src, int sx, int sy, int width, int height, CANVAS *dest, int dx, int dy, u32 key)
{
	if(src != NULL && dest != NULL && src->buffer != NULL && dest->buffer != NULL && src->lineWidth > 0 && dest->lineWidth > 0 && src->pixelFormat == dest->pixelFormat) {
		u32 *vram32_src, *vram32_dest, *vram32_ptr_src, *vram32_ptr_dest;
		//u16 *vram16_src, *vram16_dest, *vram16_ptr_src, *vram16_ptr_dest;
		int x, y;
		
		if(src->pixelFormat == PSP_DISPLAY_PIXEL_FORMAT_8888) {
			vram32_src = src->buffer + sx + sy * src->lineWidth;
			vram32_dest = dest->buffer + dx + dy * dest->lineWidth;
			for(y = 0; y < height; y++) {
				vram32_ptr_src = vram32_src;
				vram32_ptr_dest = vram32_dest;
				for(x = 0; x < width; x++, vram32_ptr_src++, vram32_ptr_dest++)
					if(*vram32_ptr_src != key) *vram32_ptr_dest = *vram32_ptr_src;
				vram32_src += src->lineWidth;
				vram32_dest += dest->lineWidth;

			}
		}

	}
}

