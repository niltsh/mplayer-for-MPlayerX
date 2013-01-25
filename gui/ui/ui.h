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

extern int             uiLoadPlay;
extern int             uiMainRender;
extern int             uiPlaybarFade;

extern int             sx, sy;

void uiMainDraw( void );
void uiMainMouse( int Button, int X, int Y, int RX, int RY );
void uiMainKey( int KeyCode, int Type, int Key );
void uiMainDND(int num, char** files);

void uiVideoDraw( void );
void uiVideoMouse( int Button, int X, int Y, int RX, int RY );

void uiMenuInit( void );
void uiMenuHide( int mx, int my, int w );
void uiMenuShow( int mx, int my );
void uiMenuMouse( int RX, int RY );

void uiPlaybarInit( void );
void uiPlaybarShow( int y );

#endif /* MPLAYER_GUI_UI_H */
