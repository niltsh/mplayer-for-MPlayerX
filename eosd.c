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

#include "mpcommon.h"
#include "libmpcodecs/vf.h"
#include "libvo/video_out.h"
#include "libvo/sub.h"
#include "ass_mp.h"
#include "eosd.h"

#ifdef CONFIG_ASS
static ASS_Renderer *ass_renderer;
int prev_visibility;

void eosd_ass_init(ASS_Library *ass_library)
{
    ass_renderer = ass_renderer_init(ass_library);
    if (!ass_renderer)
        return;
    ass_configure_fonts(ass_renderer);
}
#endif

void eosd_init(vf_instance_t *vf)
{
    vf->control(vf, VFCTRL_INIT_EOSD, NULL);
}

void eosd_configure(mp_eosd_res_t *res, int hinting)
{
#ifdef CONFIG_ASS
    double dar = (double) (res->w - res->ml - res->mr) / (res->h - res->mt - res->mb);
    if (ass_renderer) {
        ass_configure(ass_renderer, res->w, res->h, hinting);
        ass_set_margins(ass_renderer, res->mt, res->mb, res->ml, res->mr);
        ass_set_aspect_ratio(ass_renderer, dar, (double)res->srcw/res->srch);
    }
#endif
}

ASS_Image *eosd_render_frame(double ts, int *changed)
{
    ASS_Image *r = NULL;
#ifdef CONFIG_ASS
    if (sub_visibility && ass_renderer && ass_track && ts != MP_NOPTS_VALUE) {
        r = ass_mp_render_frame(ass_renderer, ass_track, (ts+sub_delay) * 1000 + .5, changed);
        if (!prev_visibility && changed)
            *changed = 2;
    }
    prev_visibility = sub_visibility;
#endif
    return r;
}

void eosd_uninit(void)
{
#ifdef CONFIG_ASS
    if (ass_renderer)
        ass_renderer_done(ass_renderer);
#endif
}
