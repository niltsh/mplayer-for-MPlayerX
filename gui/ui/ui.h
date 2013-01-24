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

#ifndef MPLAYER_GUI_UI_H
#define MPLAYER_GUI_UI_H

extern unsigned char * mainDrawBuffer;
extern unsigned char * menuDrawBuffer;
extern int             mainVisible;

extern int             uiMainAutoPlay;
extern int             uiPlaybarFade;

extern int             sx, sy;

void uiInit( void * disp );

void uiMainDraw( void );
void uiEventHandling( int msg, float param );
void uiMainMouseHandle( int Button, int X, int Y, int RX, int RY );
void uiMainKeyHandle( int KeyCode, int Type, int Key );
void uiDandDHandler(int num, char** files);

void uiVideoDraw( void );
void uiVideoMouseHandle( int Button, int X, int Y, int RX, int RY );

void uiMenuInit( void );
void uiHideMenu( int mx, int my, int w );
void uiShowMenu( int mx, int my );
void uiMenuMouseHandle( int RX, int RY );

void uiPlaybarInit( void );
void uiPlaybarShow( int y );

#endif /* MPLAYER_GUI_UI_H */
