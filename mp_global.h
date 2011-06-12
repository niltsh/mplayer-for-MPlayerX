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

/*
 * This header is meant for code that is needed practically everywhere:
 * Filters, demuxers, streams, mplayer.c etc.
 */
#ifndef MPLAYER_MP_GLOBAL_H
#define MPLAYER_MP_GLOBAL_H

//FIXME this should be in a common header, but i dunno which
#define MP_NOPTS_VALUE (-1LL<<63) //both int64_t and double should be able to represent this exactly

#endif /* MPLAYER_MP_GLOBAL_H */
