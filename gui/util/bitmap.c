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
#include <unistd.h>

#include "bitmap.h"

#include "help_mp.h"
#include "libavcodec/avcodec.h"
#include "libavutil/common.h"
#include "libavutil/intreadwrite.h"
#include "libvo/fastmemcpy.h"
#include "mp_msg.h"

static int pngRead(const char *fname, guiImage *img)
{
    FILE *file;
    long len;
    void *data;
    int decode_ok, bpl;
    AVCodecContext *avctx;
    AVFrame *frame;
    AVPacket pkt;

    file = fopen(fname, "rb");

    if (!file)
        return 2;

    fseek(file, 0, SEEK_END);
    len = ftell(file);

    if (len > 50 * 1024 * 1024) {
        fclose(file);
        return 3;
    }

    data = av_malloc(len + FF_INPUT_BUFFER_PADDING_SIZE);

    if (!data) {
        fclose(file);
        return 4;
    }

    fseek(file, 0, SEEK_SET);
    fread(data, len, 1, file);
    fclose(file);

    avctx = avcodec_alloc_context();
    frame = avcodec_alloc_frame();

    if (!(avctx && frame)) {
        av_free(frame);
        av_free(avctx);
        av_free(data);
        return 5;
    }

    avcodec_register_all();
    avcodec_open(avctx, avcodec_find_decoder(CODEC_ID_PNG));

    av_init_packet(&pkt);
    pkt.data = data;
    pkt.size = len;
    // HACK: make PNGs decode normally instead of as CorePNG delta frames
    pkt.flags = AV_PKT_FLAG_KEY;

    avcodec_decode_video2(avctx, frame, &decode_ok, &pkt);

    memset(img, 0, sizeof(*img));

    switch (avctx->pix_fmt) {
    case PIX_FMT_GRAY8:
        img->Bpp = 8;
        break;

    case PIX_FMT_GRAY16BE:
        img->Bpp = 16;
        break;

    case PIX_FMT_RGB24:
        img->Bpp = 24;
        break;

    case PIX_FMT_BGRA:
    case PIX_FMT_ARGB:
        img->Bpp = 32;
        break;

    default:
        img->Bpp = 0;
        break;
    }

    if (decode_ok && img->Bpp) {
        img->Width  = avctx->width;
        img->Height = avctx->height;
        bpl = img->Width * (img->Bpp / 8);
        img->ImageSize = bpl * img->Height;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] file: %s\n", fname);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap]  size: %lux%lu, color depth: %u\n", img->Width, img->Height, img->Bpp);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap]  image size: %lu\n", img->ImageSize);

        img->Image = malloc(img->ImageSize);

        if (img->Image)
            memcpy_pic(img->Image, frame->data[0], bpl, img->Height, bpl, frame->linesize[0]);
        else
            decode_ok = 0;
    }

    avcodec_close(avctx);
    av_free(frame);
    av_free(avctx);
    av_free(data);

    return !(decode_ok && img->Bpp);
}

static int Convert24to32(guiImage *img)
{
    char *orgImage;
    unsigned long i, c;

    if (img->Bpp == 24) {
        orgImage = img->Image;

        img->Bpp       = 32;
        img->ImageSize = img->Width * img->Height * 4;
        img->Image     = calloc(1, img->ImageSize);

        if (!img->Image) {
            free(orgImage);
            mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] not enough memory: %lu\n", img->ImageSize);
            return 0;
        }

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] 32 bpp conversion size: %lu\n", img->ImageSize);

        for (c = 0, i = 0; c < img->ImageSize; c += 4, i += 3)
            *(uint32_t *)&img->Image[c] = ALPHA_OPAQUE | AV_RB24(&orgImage[i]);

        free(orgImage);
    }

    return 1;
}

static const char *fExist(const char *fname)
{
    static const char ext[][4] = { "png", "PNG" };
    static char buf[512];
    unsigned int i;

    if (access(fname, R_OK) == 0)
        return fname;

    for (i = 0; i < FF_ARRAY_ELEMS(ext); i++) {
        snprintf(buf, sizeof(buf), "%s.%s", fname, ext[i]);

        if (access(buf, R_OK) == 0)
            return buf;
    }

    return NULL;
}

int bpRead(const char *fname, guiImage *img)
{
    int r;

    fname = fExist(fname);

    if (!fname)
        return -2;

    r = pngRead(fname, img);

    if (r != 0) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] read error #%d: %s\n", r, fname);
        return -5;
    }

    if (img->Bpp < 24) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] bpp too low: %u\n", img->Bpp);
        return -1;
    }

    if (!Convert24to32(img))
        return -8;

    return 0;
}

void bpFree(guiImage *img)
{
    free(img->Image);
    memset(img, 0, sizeof(*img));
}

int bpRenderMask(const guiImage *in, guiImage *out)
{
    uint32_t *buf;
    unsigned long i;
    int b = 0, c = 0;
    unsigned char tmp = 0;
    int shaped = 0;

    out->Width     = in->Width;
    out->Height    = in->Height;
    out->Bpp       = 1;
    out->ImageSize = (out->Width * out->Height + 7) / 8;
    out->Image     = calloc(1, out->ImageSize);

    if (!out->Image) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] not enough memory: %lu\n", out->ImageSize);
        return 0;
    }

    buf = (uint32_t *)in->Image;

    for (i = 0; i < out->Width * out->Height; i++) {
        tmp >>= 1;

        if (!IS_TRANSPARENT(buf[i]))
            tmp |= 0x80;
        else {
            buf[i] = 0;
            shaped = 1;
        }

        if (++b == 8) {
            out->Image[c++] = tmp;
            tmp = b = 0;
        }
    }

    if (b)
        out->Image[c] = tmp;

    if (!shaped)
        bpFree(out);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] 1 bpp conversion size: %lu\n", out->ImageSize);

    return 1;
}
