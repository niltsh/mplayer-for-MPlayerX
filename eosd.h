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

/**
 * Initialize the EOSD subsystem.
 *
 * @param vf  the video filter chain where the rendering will take place.
 */
void eosd_init(vf_instance_t *vf);

/**
 * Configure the resolution for EOSD rendering.
 * Should be called by the rendering engine whenever the resolution or
 * settings change.
 *
 * @param res      resolution and margins of the rendering area.
 * @param hinting  nonzero if hinting is useful.
 */
void eosd_configure(mp_eosd_res_t *res, int hinting);

/**
 * Renders the EOSD elements for the current frame.
 * Should be called by the rendering engine when it is about to do or
 * prepare the rendering.
 *
 * @param ts       presentation timestamp of the frame.
 * @param changed  if not NULL, will be set to 0 if the elements are
 *                 identical since the last call, 1 if they have changed
 *                 only in coordinates, and 2 if they have really changed.
 * @return         a linked list of EOSD elements.
 */
struct ass_image *eosd_render_frame(double ts, int *changed);

/**
 * Shut down the EOSD subsystem and free the associated resources.
 */
void eosd_uninit(void);

/**
 * Initialize the use of EOSD for ASS subtitles rendering.
 */
void eosd_ass_init(struct ass_library *);

#endif /* MPLAYER_EOSD_H */
