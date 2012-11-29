/*
 * Copyright (C) 2012 Xidorn Quan
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

#include <stdlib.h>

#include "config.h"
#include "mp_msg.h"

#include "vd_internal.h"

static const vd_info_t info = {
    "black video generator",
    "black",
    "",
    "Xidorn Quan",
    "no decoding, always outputs black frames"
};

LIBVD_EXTERN(black)

typedef struct {
    unsigned char *planes[MP_MAX_PLANES];
    int stride[MP_MAX_PLANES];
    int buffer_filled;
    int w, h;
} vd_black_ctx;

static const unsigned int fmt_list[] = {
    IMGFMT_YV12,
    IMGFMT_IYUV,
    IMGFMT_I420,
    IMGFMT_UYVY,
    IMGFMT_YUY2,
    IMGFMT_BGR24,
    IMGFMT_RGB24,
    0
};

// to set/get/query special features/parameters
static int control(sh_video_t *sh, int cmd, void *arg, ...)
{
    unsigned int format;
    const unsigned int *p;

    switch (cmd) {
    case VDCTRL_QUERY_FORMAT:
        format = *(unsigned int *)arg;
        for (p = fmt_list; *p; ++p)
            if (format == *p)
                return CONTROL_TRUE;
        return CONTROL_FALSE;
    }

    return CONTROL_UNKNOWN;
}

// init driver
static int init(sh_video_t *sh)
{
    unsigned int format;
    vd_black_ctx *ctx;
    int w, h;
    int i;

    ctx = sh->context = calloc(1, sizeof(*ctx));
    if (!ctx)
        return 0;

    ctx->w = w = sh->disp_w;
    ctx->h = h = sh->disp_h;
    if (!mpcodecs_config_vo(sh, w, h, IMGFMT_BGR24))
        goto error;
    format = sh->codec->outfmt[sh->outfmtidx];

    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "[vd_black] format: %s\n", vo_format_name(format));
    switch (format) {
        case IMGFMT_RGB24:
        case IMGFMT_BGR24:
            ctx->planes[0] = malloc(w * h * 3);
            if (!ctx->planes[0])
                goto error;
            ctx->stride[0] = w * 3;
            memset(ctx->planes[0], 0, w * h * 3);
            break;
        case IMGFMT_UYVY:
        case IMGFMT_YUY2:
            ctx->planes[0] = malloc(w * h * 2);
            ctx->stride[0] = w * 2;
            if (format == IMGFMT_UYVY) {
                for (i = 0; i < w * h * 2; i += 4) {
                    ctx->planes[0][i + 0] = 128;
                    ctx->planes[0][i + 1] = 16;
                    ctx->planes[0][i + 2] = 128;
                    ctx->planes[0][i + 3] = 16;
                }
            } else {
                for (i = 0; i < w * h * 2; i += 4) {
                    ctx->planes[0][i + 0] = 16;
                    ctx->planes[0][i + 1] = 128;
                    ctx->planes[0][i + 2] = 16;
                    ctx->planes[0][i + 3] = 128;
                }
            }

            break;
        case IMGFMT_YV12:
        case IMGFMT_IYUV:
        case IMGFMT_I420:
            ctx->planes[0] = malloc(w * h);
            ctx->planes[1] = malloc(w * h / 4);
            ctx->planes[2] = malloc(w * h / 4);
            if (!ctx->planes[0] || !ctx->planes[1] || !ctx->planes[2])
                goto error;
            ctx->stride[0] = w;
            ctx->stride[1] = w / 2;
            ctx->stride[2] = w / 2;
            memset(ctx->planes[0], 16, w * h);
            memset(ctx->planes[1], 128, w * h / 4);
            memset(ctx->planes[2], 128, w * h / 4);
            break;
        default:
            goto error;
    }

    return 1;

error:
    uninit(sh);
    return 0;
}

// uninit driver
static void uninit(sh_video_t *sh)
{
    int i;
    vd_black_ctx *ctx = sh->context;

    for (i = 0; i < MP_MAX_PLANES; ++i) {
        free(ctx->planes[i]);
        ctx->planes[i] = NULL;
    }

    free(ctx);
}

// decode a frame
static mp_image_t* decode(sh_video_t *sh, void *data, int len, int flags)
{
    mp_image_t *mpi = NULL;
    vd_black_ctx *ctx = sh->context;
    int i;

    if (sh->num_buffered_pts == FF_ARRAY_ELEMS(sh->buffered_pts) - 1)
        ctx->buffer_filled = 1;
    if (!ctx->buffer_filled || !sh->num_buffered_pts)
        return NULL;

    mpi = mpcodecs_get_image(sh, MP_IMGTYPE_EXPORT, MP_IMGFLAG_PRESERVE,
            ctx->w, ctx->h);
    if (!mpi)
        return NULL;
    for (i = 0; i < MP_MAX_PLANES; ++i) {
        mpi->planes[i] = ctx->planes[i];
        mpi->stride[i] = ctx->stride[i];
    }
    return mpi;
}
