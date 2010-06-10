/*****************************************************************************
 *
 * - H.264 encoder for mencoder using x264 -
 *
 * Copyright (C) 2004 LINUX4MEDIA GmbH
 * Copyright (C) 2004 Ark Linux
 *
 * Written by Bernhard Rosenkraenzer <bero@arklinux.org>
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
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "mp_msg.h"

#include "m_option.h"
#include "codec-cfg.h"
#include "stream/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"

#include "stream/stream.h"
#include "libmpdemux/muxer.h"

#include "img_format.h"
#include "mp_image.h"
#include "vf.h"
#include "ve_x264.h"

#include <x264.h>

typedef struct h264_module_t {
    muxer_stream_t *mux;
    x264_t *    x264;
    x264_picture_t  pic;
} h264_module_t;

extern char* passtmpfile;
static x264_param_t param;
static int parse_error = 0;

static int put_image(struct vf_instance *vf, mp_image_t *mpi, double pts);
static int encode_frame(struct vf_instance *vf, x264_picture_t *pic_in);

void x264enc_set_param(const m_option_t* opt, char* arg)
{
    static int initialized = 0;
    int slow_firstpass = 0;
    char *preset = NULL, *tune = NULL, *profile = NULL;
    char *p, *copy, *name;

    if (!initialized) {
        x264_param_default(&param);
        initialized = 1;
    }
    if (!arg) {
        mp_msg(MSGT_CFGPARSER, MSGL_ERR, "Option x264encopts: no options provided\n");
        parse_error = 1;
        return;
    } else if (!*arg)
        /* Empty arguments, just doing initialization of default parameters. */
        return;

    /* Step 1: look for initial preset/tune. */
    copy = p = strdup(arg);
    while ((name = strsep(&copy, ":"))) {
        char *value = strpbrk(name, "=:");
        if (!value)
            continue;
        *value++ = 0;
        if (!strcasecmp(name, "preset"))
            preset = value;
        else if (!strcasecmp(name, "tune"))
            tune = value;
    }
    if (x264_param_default_preset(&param, preset, tune) < 0) {
        mp_msg(MSGT_CFGPARSER, MSGL_ERR, "Option x264encopts: Invalid preset or tune.\n");
        parse_error = 1;
    }
    free(p);

    /* Step 2: explicit user overrides */
    while ((name = strsep(&arg, ":")) && *name) {
        int ret = 0;
        char *value = strpbrk(name, "=:");

        if (value)
            *value++ = 0;
        if (!strcasecmp(name, "profile"))
            profile = value;
        else if (!strcasecmp(name, "turbo")) {
            mp_msg(MSGT_CFGPARSER, MSGL_WARN, "Option x264encopts: turbo option is deprecated; "
                                              "use slow_firstpass to disable turbo\n");
            if (value && *value == '0')
                slow_firstpass = 1;
        } else if (!strcasecmp(name, "slow_firstpass"))
            slow_firstpass = 1;
        else if (strcasecmp(name, "preset") && strcasecmp(name, "tune")) {
            ret = x264_param_parse(&param, name, value);
            if (ret == X264_PARAM_BAD_NAME)
                mp_msg(MSGT_CFGPARSER, MSGL_ERR, "Option x264encopts: Unknown suboption %s\n", name);
            if (ret == X264_PARAM_BAD_VALUE)
                mp_msg(MSGT_CFGPARSER, MSGL_ERR, "Option x264encopts: Bad argument %s=%s\n",
                       name, value ? value : "(null)");

        }
        /* mark this option as done, so it's not reparsed if there's another -x264encopts */
        *name = 0;

        parse_error |= ret;
    }

    /* Step 3: Apply fast first pass (turbo) options. */
    if (!slow_firstpass)
        x264_param_apply_fastfirstpass(&param);

    /* Step 4: enforce profile */
    if (profile && x264_param_apply_profile(&param, profile) < 0)
        parse_error = 1;
}

static int config(struct vf_instance *vf, int width, int height, int d_width, int d_height, unsigned int flags, unsigned int outfmt) {
    h264_module_t *mod=(h264_module_t*)vf->priv;

    if(parse_error)
        return 0;

    mod->mux->bih->biWidth = width;
    mod->mux->bih->biHeight = height;
    mod->mux->bih->biSizeImage = width * height * 3;
    mod->mux->aspect = (float)d_width/d_height;

    // make sure param is initialized
    x264enc_set_param(NULL, "");
    param.i_width = width;
    param.i_height = height;
    param.i_fps_num = mod->mux->h.dwRate;
    param.i_fps_den = mod->mux->h.dwScale;
    param.b_vfr_input = 0;
    param.vui.i_sar_width = d_width*height;
    param.vui.i_sar_height = d_height*width;

    x264_param_parse(&param, "stats", passtmpfile);

    switch(outfmt) {
    case IMGFMT_I420:
        param.i_csp = X264_CSP_I420;
        break;
    case IMGFMT_YV12:
        param.i_csp = X264_CSP_YV12;
        break;
    default:
        mp_msg(MSGT_MENCODER, MSGL_ERR, "Wrong colorspace.\n");
        return 0;
    }

    mod->x264 = x264_encoder_open(&param);
    if(!mod->x264) {
        mp_msg(MSGT_MENCODER, MSGL_ERR, "x264_encoder_open failed.\n");
        return 0;
    }

    if(!param.b_repeat_headers){
        x264_nal_t *nal;
        int extradata_size, nnal;

        extradata_size = x264_encoder_headers(mod->x264, &nal, &nnal);

        mod->mux->bih= realloc(mod->mux->bih, sizeof(BITMAPINFOHEADER) + extradata_size);
        memcpy(mod->mux->bih + 1, nal->p_payload, extradata_size);
        mod->mux->bih->biSize= sizeof(BITMAPINFOHEADER) + extradata_size;
    }

    if (param.i_bframe > 1 && param.i_bframe_pyramid)
        mod->mux->decoder_delay = 2;
    else
        mod->mux->decoder_delay = param.i_bframe ? 1 : 0;

    return 1;
}

static int control(struct vf_instance *vf, int request, void *data)
{
    h264_module_t *mod=(h264_module_t*)vf->priv;
    switch(request){
        case VFCTRL_FLUSH_FRAMES:
            while (x264_encoder_delayed_frames(mod->x264) > 0)
                encode_frame(vf, NULL);
            return CONTROL_TRUE;
        default:
            return CONTROL_UNKNOWN;
    }
}

static int query_format(struct vf_instance *vf, unsigned int fmt)
{
    switch(fmt) {
    case IMGFMT_I420:
        return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW;
    case IMGFMT_YV12:
    case IMGFMT_422P:
    case IMGFMT_444P:
    case IMGFMT_YVYU:
    case IMGFMT_RGB:
    case IMGFMT_BGR:
    case IMGFMT_BGR32:
        /* These colorspaces are supported, but they'll just have
         * to be converted to I420 internally */
        return 0; /* VFCAP_CSP_SUPPORTED */
    }
    return 0;
}

static int put_image(struct vf_instance *vf, mp_image_t *mpi, double pts)
{
    h264_module_t *mod=(h264_module_t*)vf->priv;
    int i;

    memset(&mod->pic, 0, sizeof(x264_picture_t));
    mod->pic.img.i_csp=param.i_csp;
    mod->pic.img.i_plane=3;
    for(i=0; i<4; i++) {
        mod->pic.img.plane[i] = mpi->planes[i];
        mod->pic.img.i_stride[i] = mpi->stride[i];
    }

    mod->pic.i_type = X264_TYPE_AUTO;

    return encode_frame(vf, &mod->pic) >= 0;
}

static int encode_frame(struct vf_instance *vf, x264_picture_t *pic_in)
{
    h264_module_t *mod=(h264_module_t*)vf->priv;
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal;
    int i_size;

    i_size = x264_encoder_encode(mod->x264, &nal, &i_nal, pic_in, &pic_out);

    if(i_size<0) {
        mp_msg(MSGT_MENCODER, MSGL_ERR, "x264_encoder_encode failed\n");
        return -1;
    }
    if(i_size>0) {
        int keyframe = pic_out.b_keyframe;
        memcpy(mod->mux->buffer, nal->p_payload, i_size);
        muxer_write_chunk(mod->mux, i_size, keyframe?AVIIF_KEYFRAME:0, MP_NOPTS_VALUE, MP_NOPTS_VALUE);
    }
    else
        ++mod->mux->encoder_delay;

    return i_size;
}

static void uninit(struct vf_instance *vf)
{
    h264_module_t *mod=(h264_module_t*)vf->priv;
    if (mod->x264)
    x264_encoder_close(mod->x264);
}

static int vf_open(vf_instance_t *vf, char *args) {
    h264_module_t *mod;

    vf->config = config;
    vf->default_caps = VFCAP_CONSTANT;
    vf->control = control;
    vf->query_format = query_format;
    vf->put_image = put_image;
    vf->uninit = uninit;
    vf->priv = malloc(sizeof(h264_module_t));

    mod=(h264_module_t*)vf->priv;
    mod->mux = (muxer_stream_t*)args;
    mod->mux->bih = malloc(sizeof(BITMAPINFOHEADER));
    memset(mod->mux->bih, 0, sizeof(BITMAPINFOHEADER));
    mod->mux->bih->biSize = sizeof(BITMAPINFOHEADER);
    mod->mux->bih->biPlanes = 1;
    mod->mux->bih->biBitCount = 24;
    mod->mux->bih->biCompression = mmioFOURCC('h', '2', '6', '4');

    return 1;
}

vf_info_t ve_info_x264 = {
    "H.264 encoder",
    "x264",
    "Bernhard Rosenkraenzer <bero@arklinux.org>",
    "(C) 2004 LINUX4MEDIA GmbH; (C) 2004 Ark Linux",
    vf_open
};
