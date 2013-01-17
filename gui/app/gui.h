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

/**
 * @file
 * @brief Internal declarations and definitions
 */

#ifndef MPLAYER_GUI_GUI_H
#define MPLAYER_GUI_GUI_H

//@{
/// boolean symbolic constant
#if !defined(True) && !defined(False)
#define True  1
#define False 0
#endif
//@}

/// a pseudo stream type indicating not to change the current one
#define SAME_STREAMTYPE (STREAMTYPE_DUMMY - 1)

/// StreamTypes that are permitted in a playlist
#define isPlaylistStreamtype (guiInfo.StreamType == STREAMTYPE_FILE || guiInfo.StreamType == STREAMTYPE_STREAM)

/// check whether x/y is inside the rectangle given by top x/y and bottom x/y
#define isInside(x, y, tx, ty, bx, by) ((x) > (tx) && (y) > (ty) && (x) < (bx) && (y) < (by))

#endif /* MPLAYER_GUI_GUI_H */
