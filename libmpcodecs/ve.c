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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "mp_msg.h"

#include "img_format.h"
#include "mp_image.h"
#include "vf.h"
#include "ve.h"

extern const vf_info_t ve_info_lavc;
extern const vf_info_t ve_info_vfw;
extern const vf_info_t ve_info_raw;
extern const vf_info_t ve_info_libdv;
extern const vf_info_t ve_info_xvid;
extern const vf_info_t ve_info_qtvideo;
extern const vf_info_t ve_info_nuv;
extern const vf_info_t ve_info_x264;

/* Please do not add any new encoders here. If you want to implement a new
 * encoder, add it to libavcodec, except for wrappers around external
 * libraries and encoders requiring binary support. */

static const vf_info_t * const encoder_list[] = {
#ifdef CONFIG_FFMPEG
    &ve_info_lavc,
#endif
#ifdef CONFIG_WIN32DLL
    &ve_info_vfw,
#ifdef CONFIG_QTX_CODECS_WIN32
    &ve_info_qtvideo,
#endif
#endif
#ifdef CONFIG_LIBDV095
    &ve_info_libdv,
#endif
    &ve_info_raw,
#ifdef CONFIG_XVID4
    &ve_info_xvid,
#endif
#ifdef CONFIG_LIBLZO
    &ve_info_nuv,
#endif
#ifdef CONFIG_X264
    &ve_info_x264,
#endif
    /* Please do not add any new encoders here. If you want to implement a new
     * encoder, add it to libavcodec, except for wrappers around external
     * libraries and encoders requiring binary support. */
    NULL
};

vf_instance_t* vf_open_encoder(vf_instance_t* next, const char *name, char *args){
    char* vf_args[] = { "_oldargs_", args, NULL };
    return vf_open_plugin(encoder_list,next,name,vf_args);
}

static double *forced_key_frames_ts;
static int forced_key_frames_number;
static int forced_key_frames_idx;

int parse_forced_key_frames(const m_option_t *opt, const char *arg)
{
    double ts;
    const char *p;
    int nts = 1, idx = 0, len;

    for (p = arg; *p; p++)
        nts += *p == ',';
    free(forced_key_frames_ts);
    forced_key_frames_ts = calloc(sizeof(*forced_key_frames_ts), nts);
    p = arg;
    while (1) {
        len = parse_timestring(p, &ts, ',');
        if (!len) {
            mp_msg(MSGT_CFGPARSER, MSGL_ERR,
                   "Option force-key-frames: invalid time: '%s'\n", p);
            return M_OPT_INVALID;
        }
        forced_key_frames_ts[idx++] = ts;
        if (!p[len])
            break;
        p += len + 1;
    }
    forced_key_frames_number = idx;
    forced_key_frames_idx = 0;
    for (idx = 1; idx < forced_key_frames_number; idx++) {
        if (forced_key_frames_ts[idx - 1] >= forced_key_frames_ts[idx]) {
            mp_msg(MSGT_CFGPARSER, MSGL_ERR, "Option force-key-frames: "
                   "timestamps are not in ascending order\n");
            return M_OPT_INVALID;
        }
    }
    return 0;
}

int is_forced_key_frame(double pts)
{
    if (forced_key_frames_idx < forced_key_frames_number &&
        pts >= forced_key_frames_ts[forced_key_frames_idx]) {
        forced_key_frames_idx++;
        if (forced_key_frames_idx >= forced_key_frames_number) {
            free(forced_key_frames_ts);
            forced_key_frames_number = 0;
        }
        return 1;
    }
    return 0;
}
