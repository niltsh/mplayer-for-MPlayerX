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
#include <stdarg.h>
#include <assert.h>

#include "config.h"
#include "mp_msg.h"
#include "help_mp.h"

#include "vd_internal.h"

#include "libavutil/intreadwrite.h"

static const vd_info_t info = {
    "Theora/VP3",
    "theora",
    "David Kuehling",
    "www.theora.org",
    "Theora project's VP3 codec"
};

LIBVD_EXTERN(theora)

#include <theora/theoradec.h>

#define THEORA_NUM_HEADER_PACKETS 3

typedef struct theora_struct_st {
    th_setup_info  *tsi;
    th_dec_ctx     *tctx;
    th_comment      tc;
    th_info         ti;
    th_ycbcr_buffer ycbcrbuf;
} theora_struct_t;

/** Convert Theora pixelformat to the corresponding IMGFMT_ */
static uint32_t theora_pixelformat2imgfmt(th_pixel_fmt fmt)
{
    switch (fmt) {
    case TH_PF_420: return IMGFMT_YV12;
    case TH_PF_422: return IMGFMT_422P;
    case TH_PF_444: return IMGFMT_444P;
    }
    return 0;
}

// to set/get/query special features/parameters
static int control(sh_video_t *sh, int cmd, void *arg, ...)
{
    theora_struct_t *context = sh->context;
    switch (cmd) {
    case VDCTRL_QUERY_FORMAT:
        if (*(int *)arg == theora_pixelformat2imgfmt(context->ti.pixel_fmt))
            return CONTROL_TRUE;
        return CONTROL_FALSE;
    }

    return CONTROL_UNKNOWN;
}

/*
 * init driver
 */
static int init(sh_video_t *sh)
{
    theora_struct_t *context = NULL;
    uint8_t *extradata       = (uint8_t *)(sh->bih + 1);
    int      extradata_size  = sh->bih->biSize - sizeof(*sh->bih);
    int      errorCode       = 0;
    ogg_packet op;
    int i;

    context     = calloc(sizeof(theora_struct_t), 1);
    sh->context = context;
    if (!context)
        goto err_out;

    th_info_init(&context->ti);
    th_comment_init(&context->tc);
    context->tsi = NULL;

    /* Read all header packets, pass them to theora_decode_header. */
    for (i = 0; i < THEORA_NUM_HEADER_PACKETS; i++) {
        if (extradata_size > 2) {
            op.bytes  = AV_RB16(extradata);
            op.packet = extradata + 2;
            op.b_o_s  = 1;
            if (extradata_size < op.bytes + 2) {
                mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Theora header too small\n");
                goto err_out;
            }
            extradata      += op.bytes + 2;
            extradata_size -= op.bytes + 2;
        } else {
            op.bytes = ds_get_packet(sh->ds, &op.packet);
            op.b_o_s = 1;
        }

        if ((errorCode = th_decode_headerin(&context->ti, &context->tc, &context->tsi, &op)) < 0) {
            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Broken Theora header; errorCode=%i!\n", errorCode);
            goto err_out;
        }
    }

    /* now init codec */
    context->tctx = th_decode_alloc(&context->ti, context->tsi);
    if (!context->tctx) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Theora decode init failed\n");
        goto err_out;
    }
    /* free memory used for decoder setup information */
    th_setup_free(context->tsi);

    if (sh->aspect == 0.0 && context->ti.aspect_denominator != 0) {
        sh->aspect = ((double)context->ti.aspect_numerator * context->ti.frame_width) /
                     ((double)context->ti.aspect_denominator * context->ti.frame_height);
    }

    mp_msg(MSGT_DECVIDEO, MSGL_V, "INFO: Theora video init ok!\n");
    mp_msg(MSGT_DECVIDEO, MSGL_INFO, "Frame %dx%d, Picture %dx%d, Offset [%d,%d]\n", context->ti.frame_width, context->ti.frame_height, context->ti.pic_width, context->ti.pic_height, context->ti.pic_x, context->ti.pic_y);

    return mpcodecs_config_vo(sh, context->ti.frame_width, context->ti.frame_height, theora_pixelformat2imgfmt(context->ti.pixel_fmt));

err_out:
    free(context);
    sh->context = NULL;
    return 0;
}

/*
 * uninit driver
 */
static void uninit(sh_video_t *sh)
{
    theora_struct_t *context = sh->context;

    if (context) {
        th_info_clear(&context->ti);
        th_comment_clear(&context->tc);
        th_decode_free(context->tctx);
        free(context);
    }
}

/*
 * decode frame
 */
static mp_image_t *decode(sh_video_t *sh, void *data, int len, int flags)
{
    theora_struct_t *context = sh->context;
    int errorCode = 0;
    ogg_packet op;
    mp_image_t *mpi;

    // no delayed frames
    if (!data || !len)
        return NULL;

    memset(&op, 0, sizeof(op));
    op.bytes      = len;
    op.packet     = data;
    op.granulepos = -1;

    errorCode = th_decode_packetin(context->tctx, &op, NULL);
    if (errorCode < 0) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Theora decode packetin failed: %i \n",
               errorCode);
        return NULL;
    }

    if (errorCode != TH_DUPFRAME) {
        errorCode = th_decode_ycbcr_out(context->tctx, context->ycbcrbuf);
        if (errorCode != 0) {
            mp_msg(MSGT_DECVIDEO, MSGL_ERR,
                   "Theora decode YUVout failed: %i \n", errorCode);
            return NULL;
        }
    }

    mpi = mpcodecs_get_image(sh, MP_IMGTYPE_EXPORT, 0,
                             context->ycbcrbuf[0].width,
                             context->ycbcrbuf[0].height);
    if (!mpi)
        return NULL;

    mpi->planes[0] = context->ycbcrbuf[0].data;
    mpi->stride[0] = context->ycbcrbuf[0].stride;
    mpi->planes[1] = context->ycbcrbuf[1].data;
    mpi->stride[1] = context->ycbcrbuf[1].stride;
    mpi->planes[2] = context->ycbcrbuf[2].data;
    mpi->stride[2] = context->ycbcrbuf[2].stride;

    return mpi;
}
