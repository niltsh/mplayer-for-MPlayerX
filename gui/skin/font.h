/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPLAYER_GUI_FONT_H
#define MPLAYER_GUI_FONT_H

#include "gui/bitmap.h"
#include "gui/app.h"

#define ASCII_CHRS 128   // number of ASCII characters
#define EXTRA_CHRS 128   // (arbitrary) number of non-ASCII characters
#define UTF8LENGTH 4     // length of an UTF-8 encoding according to RFC 3629

#define MAX_FONT_NAME 128
#define MAX_FONTS 25

#define fntAlignLeft   0
#define fntAlignCenter 1
#define fntAlignRight  2

typedef struct
{
 int x,y;   // location
 int sx,sy; // size
} fntChar;

typedef struct
{
 fntChar         Fnt[ASCII_CHRS + EXTRA_CHRS];
 unsigned char   nonASCIIidx[EXTRA_CHRS][UTF8LENGTH];
 txSample        Bitmap;
 char            name[MAX_FONT_NAME];
} bmpFont;

void fntFreeFont( void );
int  fntFindID( char * name );
int  fntTextWidth( int id, char * str );

int        fntRead( char * path, char * fname );
txSample * fntRender( wItem * item, int px, char * txt );

#endif /* MPLAYER_GUI_FONT_H */
