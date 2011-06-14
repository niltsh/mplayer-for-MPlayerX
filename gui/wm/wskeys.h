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

#ifndef MPLAYER_GUI_WSKEYS_H
#define MPLAYER_GUI_WSKEYS_H

#define wsEnter         0x0d + 256
#define wsEscape        0x1b + 256
#define wsGrayMul       0xaa + 256
#define wsGrayDiv       0xaf + 256

//Keys for multimedia keyboard

#define wsXF86LowerVolume 0x11 + 256
#define wsXF86RaiseVolume 0x13 + 256
#define wsXF86Mute        0x12 + 256
#define wsXF86Play        0x14 + 256
#define wsXF86Stop        0x15 + 256
#define wsXF86Prev        0x16 + 256
#define wsXF86Next        0x17 + 256
#define wsXF86Media       0x32 + 256

// NOTE TO MYSELF: This is only for the Acer AirKey V keyboard.
#define wsXFMMPrev        0x98
#define wsXFMMStop        0xb1
#define wsXFMMPlay	  0x95
#define wsXFMMNext        0x93
#define wsXFMMVolUp       0xad
#define wsXFMMVolDown     0xa6
#define wsXFMMMute        0x99

#endif /* MPLAYER_GUI_WSKEYS_H */
