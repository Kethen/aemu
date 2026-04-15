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

#ifndef __DRAW_H__
#define __DRAW_H__
#include <psptypes.h>

#define RGB(r, g, b) ((r)|((g)<<8)|((b)<<16))
#define RGB_8888(r, g, b) (u32)((r)|((g)<<8)|((b)<<16))
#define RGB_565(r, g, b) (u16)((r)|((g)<<5)|((b)<<11))
#define RGB_5551(r, g, b) (u16)((r)|((g)<<5)|((b)<<10))
#define RGB_4444(r, g, b) (u16)((r)|((g)<<4)|((b)<<8))

char convertChar(unsigned char c);

typedef struct CANVAS {
	int width;
	int height;
	int lineWidth;
	int pixelFormat;
	int scale;
	u32 *buffer;
} CANVAS;

extern int draw_min_x;
extern int draw_max_x;

//void drawText(CANVAS *canvas, const char *text, int x, int y, u32 color);

int getCanvas(CANVAS *canvas);
void drawSmallFont(CANVAS *canvas, const char *text, int x, int y, u32 fillcolor);
void drawSmallFontVertical(CANVAS *canvas, const char *text, int x, int y, u32 fillcolor);
void fillRectangle(CANVAS *canvas, int x, int y, int width, int height, u32 fill, u32 border);
void canvasCopy(CANVAS *src, int sx, int sy, int width, int height, CANVAS *dest, int dx, int dy, u32 key);
char convertChar(unsigned char c);

#endif
