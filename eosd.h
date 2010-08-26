/*
 * Extended On Screen Display
 * Copyright (C) 2010 Nicolas George
 *
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

#ifndef MPLAYER_EOSD_H
#define MPLAYER_EOSD_H

void eosd_init(vf_instance_t *);

void eosd_configure(mp_eosd_res_t *, int);
struct ass_image *eosd_render_frame(double, int *);
void eosd_uninit(void);

void eosd_ass_init(struct ass_library *);

#endif /* MPLAYER_EOSD_H */
