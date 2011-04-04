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

#include "bitmap.h"

#include "help_mp.h"
#include "libavcodec/avcodec.h"
#include "libavutil/intreadwrite.h"
#include "libvo/fastmemcpy.h"
#include "mp_msg.h"

static int pngRead(unsigned char *fname, txSample *bf)
{
    FILE *fp;
    int decode_ok;
    void *data;
    int len;
    AVCodecContext *avctx;
    AVFrame *frame;
    AVPacket pkt;

    fp = fopen(fname, "rb");

    if (!fp) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] open error: %s\n", fname);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);

    if (len > 50 * 1024 * 1024) {
        fclose(fp);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] file too big: %s\n", fname);
        return 2;
    }

    data = av_malloc(len + FF_INPUT_BUFFER_PADDING_SIZE);

    fseek(fp, 0, SEEK_SET);
    fread(data, len, 1, fp);
    fclose(fp);

    avctx = avcodec_alloc_context();
    frame = avcodec_alloc_frame();
    avcodec_register_all();
    avcodec_open(avctx, avcodec_find_decoder(CODEC_ID_PNG));
    av_init_packet(&pkt);
    pkt.data = data;
    pkt.size = len;
    // HACK: make PNGs decode normally instead of as CorePNG delta frames
    pkt.flags = AV_PKT_FLAG_KEY;
    avcodec_decode_video2(avctx, frame, &decode_ok, &pkt);
    memset(bf, 0, sizeof(*bf));

    switch (avctx->pix_fmt) {
    case PIX_FMT_GRAY8:
        bf->BPP = 8;
        break;

    case PIX_FMT_GRAY16BE:
        bf->BPP = 16;
        break;

    case PIX_FMT_RGB24:
        bf->BPP = 24;
        break;

    case PIX_FMT_BGRA:
    case PIX_FMT_ARGB:
        bf->BPP = 32;
        break;

    default:
        bf->BPP = 0;
        break;
    }

    if (decode_ok && bf->BPP) {
        int bpl;

        bf->Width  = avctx->width;
        bf->Height = avctx->height;
        bpl = bf->Width * (bf->BPP / 8);
        bf->ImageSize = bpl * bf->Height;
        bf->Image     = malloc(bf->ImageSize);
        memcpy_pic(bf->Image, frame->data[0], bpl, bf->Height, bpl, frame->linesize[0]);
    }

    avcodec_close(avctx);
    av_freep(&frame);
    av_freep(&avctx);
    av_freep(&data);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] file: %s\n", fname);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap]  size: %lux%lu, color depth: %u\n", bf->Width, bf->Height, bf->BPP);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap]  image size: %lu\n", bf->ImageSize);

    return !(decode_ok && bf->BPP);
}

static int Convert24to32(txSample *bf)
{
    unsigned char *tmpImage;
    unsigned int i, c;

    if (bf->BPP == 24) {
        tmpImage      = bf->Image;
        bf->BPP       = 32;
        bf->ImageSize = bf->Width * bf->Height * 4;
        bf->Image     = calloc(1, bf->ImageSize);

        if (!bf->Image) {
            free(tmpImage);
            mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] not enough memory: %lu\n", bf->ImageSize);
            return 0;
        }

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] 32 bpp conversion size: %lu\n", bf->ImageSize);

        for (c = 0, i = 0; c < bf->ImageSize; c += 4, i += 3)
            *(uint32_t *)&bf->Image[c] = AV_RB24(&tmpImage[i]);

        free(tmpImage);
    }

    return 1;
}

static void Normalize(txSample *bf)
{
    int i;

    for (i = 0; i < (int)bf->ImageSize; i += 4)
#if !HAVE_BIGENDIAN
        bf->Image[i + 3] = 0;
#else
        bf->Image[i] = 0;
#endif
}

static unsigned char *fExist(unsigned char *fname)
{
    static unsigned char tmp[512];
    FILE *fl;
    unsigned char ext[][6] = { ".png\0", ".PNG\0" };
    int i;

    fl = fopen(fname, "rb");

    if (fl != NULL) {
        fclose(fl);
        return fname;
    }

    for (i = 0; i < 2; i++) {
        snprintf(tmp, sizeof(tmp), "%s%s", fname, ext[i]);
        fl = fopen(tmp, "rb");

        if (fl != NULL) {
            fclose(fl);
            return tmp;
        }
    }

    return NULL;
}

int bpRead(char *fname, txSample *bf)
{
    fname = fExist(fname);

    if (fname == NULL)
        return -2;

    if (pngRead(fname, bf)) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] read error: %s\n", fname);
        return -5;
    }

    if (bf->BPP < 24) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] bpp too low: %u\n", bf->BPP);
        return -1;
    }

    if (!Convert24to32(bf))
        return -8;

    Normalize(bf);

    return 0;
}

void bpFree(txSample *bf)
{
    free(bf->Image);
    memset(bf, 0, sizeof(*bf));
}

int Convert32to1(txSample *in, txSample *out, uint32_t transparent)
{
    uint32_t *buf;
    unsigned long i;
    int b = 0, c = 0;
    unsigned char tmp = 0;
    int shaped = 0;

    out->Width     = in->Width;
    out->Height    = in->Height;
    out->BPP       = 1;
    out->ImageSize = (out->Width * out->Height + 7) / 8;
    out->Image     = calloc(1, out->ImageSize);

    if (out->Image == NULL) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] not enough memory: %lu\n", out->ImageSize);
        return 0;
    } else {
        buf = (uint32_t *)in->Image;

        for (i = 0; i < out->Width * out->Height; i++) {
            tmp >>= 1;

            if (buf[i] != transparent)
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

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[bitmap] 1 bpp conversion size: %lu\n", out->ImageSize);

        if (b)
            out->Image[c] = tmp;

        if (!shaped)
            bpFree(out);
    }

    return 1;
}
