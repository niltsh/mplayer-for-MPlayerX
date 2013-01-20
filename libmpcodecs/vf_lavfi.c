/*
 * Copyright (C) 2011 Nicolas George <nicolas.george@normalesup.org>
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

#include "vf.h"
#include "m_struct.h"
#include "fmt-conversion.h"
#include "mp_msg.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/avfiltergraph.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"

struct vf_priv_s {
    AVFilterGraph *graph;
    AVFilterContext *in;
    AVFilterContext *out;
    int in_w;
    int in_h;
    enum AVPixelFormat in_pixfmt;
    int in_imgfmt;
    AVRational in_sar;
    int out_w;
    int out_h;
    enum AVPixelFormat out_pixfmt;
    int out_imgfmt;
    AVRational out_sar;
    struct AVFilterBufferRef *in_buf;
    mp_image_t *in_mpi;
};

static void buf_mpi_free(AVFilterBuffer *buf)
{
    ((mp_image_t *)buf->priv)->usage_count--;
    av_free(buf);
}

static AVFilterBufferRef *mpi_to_bufref(mp_image_t *mpi, enum AVPixelFormat fmt,
                                        AVRational sar)
{
    AVFilterBufferRef *buf;
    int perms = AV_PERM_READ;

    if ((mpi->flags & MP_IMGFLAG_ALLOCATED))
        perms |= AV_PERM_REUSE2;
    if (!(mpi->flags & MP_IMGFLAG_PRESERVE))
        perms |= AV_PERM_WRITE;
    buf = avfilter_get_video_buffer_ref_from_arrays(mpi->planes, mpi->stride,
                                                    perms,
                                                    mpi->w, mpi->h,
                                                    fmt);
    buf->video->sample_aspect_ratio = sar;
    buf->buf->priv = mpi;
    buf->buf->free = buf_mpi_free;
    return buf;
}

static void bufref_to_mpi(AVFilterBufferRef *ref, mp_image_t *mpi)
{
    memcpy(mpi->planes, ref->data, sizeof(ref->data));
    memcpy(mpi->stride, ref->linesize, sizeof(ref->linesize));
}

struct mpsink_priv {
    struct vf_instance *vf;
};

static int mpsink_init(AVFilterContext *ctx,
                       av_unused const char *args, void *opaque)
{
    struct mpsink_priv *c = ctx->priv;
    c->vf = opaque;
    return 0;
}

static int mpsink_query_formats(AVFilterContext *ctx)
{
    struct mpsink_priv *c = ctx->priv;
    struct vf_instance *vf = c->vf;
    AVFilterFormats *all;
    enum AVPixelFormat *sup;
    unsigned i, nsup = 0;
    int ifmt;

    all = avfilter_all_formats(AVMEDIA_TYPE_VIDEO);
    sup = av_mallocz(sizeof(*sup) * (all->format_count + 1));
    if (!sup)
        return AVERROR(errno);
    for(i = 0; i < all->format_count; i++) {
        ifmt = pixfmt2imgfmt(all->formats[i]);
        if (vf->next->query_format(vf->next, ifmt) > 0)
            sup[nsup++] = all->formats[i];
    }
    sup[nsup++] = PIX_FMT_NONE;
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(sup));
    av_free(sup);
    return 0;
}

static AVFilterBufferRef *mpsink_get_video_buffer(AVFilterLink *link,
                                                  int perms, int w, int h)
{
    struct mpsink_priv *c = link->dst->priv;
    struct vf_instance *vf = c->vf;
    mp_image_t *dmpi;
    int type;
    int flags = 0;

    type = MP_IMGTYPE_NUMBERED | (-1 << 16);
    if ((perms & AV_PERM_PRESERVE))
        flags |= MP_IMGFLAG_PRESERVE;
    if ((perms & AV_PERM_READ))
        flags |= MP_IMGFLAG_READABLE;
    if ((perms & AV_PERM_NEG_LINESIZES))
        flags |= MP_IMGFLAG_ACCEPT_STRIDE;
    if (vf->priv->in_mpi) {
        type = vf->priv->in_mpi->type;
        vf->priv->in_mpi = NULL;
    }
    dmpi = vf_get_image(vf->next, vf->priv->out_imgfmt, type, flags, w, h);
    return mpi_to_bufref(dmpi, vf->priv->out_pixfmt, vf->priv->out_sar);
}

static void mpsink_end_frame(AVFilterLink *link)
{
    struct mpsink_priv *c = link->dst->priv;
    struct vf_instance *vf = c->vf;
    AVFilterBufferRef *buf = link->cur_buf;
    mp_image_t *mpi = buf->buf->priv;
    double pts;

    pts = buf->pts == (int64_t)AV_NOPTS_VALUE ? MP_NOPTS_VALUE :
          buf->pts * av_q2d(link->time_base);
    mpi->pict_type = buf->video->pict_type;
    mpi->fields = (buf->video->interlaced      ? MP_IMGFIELD_INTERLACED : 0) |
                  (buf->video->top_field_first ? MP_IMGFIELD_TOP_FIRST : 0);
    vf_next_put_image(vf, mpi, pts);
    avfilter_unref_buffer(link->cur_buf);
}

static AVFilter mpsink = {
    .name        = "mpsink",
    .description = "Video sink for mplayer interaction",
    .priv_size   = sizeof(struct mpsink_priv),

    .init          = mpsink_init,
    .query_formats = mpsink_query_formats,

    .inputs  = (AVFilterPad[]) {{ .name             = "default",
                                  .type             = AVMEDIA_TYPE_VIDEO,
                                  .end_frame        = mpsink_end_frame,
                                  .get_video_buffer = mpsink_get_video_buffer,
                                  .min_perms        = AV_PERM_READ, },
                                { .name = NULL }},
    .outputs = (AVFilterPad[]) {{ .name = NULL }},
};

struct mpsrc_priv {
    struct vf_instance *vf;
};

static int mpsrc_init(AVFilterContext *ctx,
                      av_unused const char *args, void *opaque)
{
    struct mpsrc_priv *c = ctx->priv;
    c->vf = opaque;
    return 0;
}

static int mpsrc_query_formats(AVFilterContext *ctx)
{
    struct mpsrc_priv *c = ctx->priv;
    enum AVPixelFormat pix_fmts[] = { c->vf->priv->in_pixfmt, PIX_FMT_NONE };
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

static int mpsrc_config_props(AVFilterLink *link)
{
    struct mpsrc_priv *c = link->src->priv;
    struct vf_instance *vf = c->vf;
    link->w = vf->priv->in_w;
    link->h = vf->priv->in_h;
    link->sample_aspect_ratio = vf->priv->in_sar;
    link->time_base = AV_TIME_BASE_Q;
    return 0;
}

static int mpsrc_request_frame(AVFilterLink *link)
{
    struct mpsrc_priv *c = link->src->priv;
    struct vf_instance *vf = c->vf;

    if (!vf->priv->in_buf)
        return AVERROR(EINVAL);
    avfilter_start_frame(link, avfilter_ref_buffer(vf->priv->in_buf, ~0));
    avfilter_draw_slice(link, 0, link->h, 1);
    avfilter_end_frame(link);
    vf->priv->in_buf = NULL;
    return 0;
}

static int mpsrc_poll_frame(AVFilterLink *link)
{
    struct mpsrc_priv *c = link->src->priv;
    struct vf_instance *vf = c->vf;
    return vf->priv->in_buf != NULL;
}

AVFilter mpsrc = {
    .name          = "mpsrc",
    .description   = "Video source for mplayer interaction",
    .priv_size     = sizeof(struct mpsrc_priv),
    .query_formats = mpsrc_query_formats,

    .init      = mpsrc_init,

    .inputs  = (AVFilterPad[]) {{ .name = NULL }},
    .outputs = (AVFilterPad[]) {{ .name           = "default",
                                  .type           = AVMEDIA_TYPE_VIDEO,
                                  .request_frame  = mpsrc_request_frame,
                                  .poll_frame     = mpsrc_poll_frame,
                                  .config_props   = mpsrc_config_props, },
                                { .name = NULL }},
};

static int config(struct vf_instance *vf, int w, int h, int dw, int dh,
                        unsigned flags, unsigned fmt)
{
    int ret;
    AVFilterLink *out;
    AVRational iar, dar;

    av_reduce(&iar.num, &iar.den, w, h, INT_MAX);
    av_reduce(&dar.num, &dar.den, dw, dh, INT_MAX);
    vf->priv->in_pixfmt = imgfmt2pixfmt(fmt);
    vf->priv->in_imgfmt = fmt;
    vf->priv->in_w = w;
    vf->priv->in_h = h;
    vf->priv->in_sar = av_div_q(dar, iar);
    ret = avfilter_graph_config(vf->priv->graph, NULL);
    if (ret < 0)
        return 0;
    out = vf->priv->out->inputs[0];
    vf->priv->out_w = out->w;
    vf->priv->out_h = out->h;
    vf->priv->out_pixfmt = out->format;
    vf->priv->out_imgfmt = pixfmt2imgfmt(out->format);
    vf->priv->out_sar = out->sample_aspect_ratio;
    if (vf->priv->out_sar.num != vf->priv->in_sar.num ||
        vf->priv->out_sar.den != vf->priv->in_sar.den ||
        out->w != w || out->h != h) {
        av_reduce(&iar.num, &iar.den, out->w, out->h, INT_MAX);
        dar = av_mul_q(iar, out->sample_aspect_ratio);
        if (av_cmp_q(dar, iar) >= 0) {
            dh = out->h;
            dw = av_rescale(dh, dar.num, dar.den);
        } else {
            dw = out->w;
            dh = av_rescale(dw, dar.den, dar.num);
        }
    }
    return vf_next_config(vf, out->w, out->h, dw, dh, flags, fmt);
}

static void get_image(struct vf_instance *vf, mp_image_t *mpi)
{
    AVFilterBufferRef *buf;
    unsigned perms = AV_PERM_WRITE | AV_PERM_REUSE2;

    avfilter_unref_buffer(mpi->priv);
    mpi->priv = NULL; /* for safety */
    if (mpi->flags & MP_IMGFLAG_READABLE)
        perms |= AV_PERM_READ;
    if (mpi->flags & MP_IMGFLAG_PRESERVE)
        perms |= AV_PERM_PRESERVE;
    vf->priv->in_mpi = mpi;
    buf = avfilter_get_video_buffer(vf->priv->in->outputs[0], perms,
                                    mpi->w, mpi->h);
    vf->priv->in_mpi = NULL;
    bufref_to_mpi(buf, mpi);
    mpi->flags |= MP_IMGFLAG_DIRECT;
    mpi->flags &= ~MP_IMGFLAG_ALLOCATED;
    mpi->priv = buf;
}

static int put_image(struct vf_instance *vf, mp_image_t *mpi, double pts)
{
    AVFilterBufferRef *buf;
    mp_image_t *cmpi = NULL;

    if (!(mpi->flags & MP_IMGFLAG_DIRECT)) {
        cmpi = vf_get_image(vf, mpi->imgfmt, MP_IMGTYPE_TEMP,
                            MP_IMGFLAG_PREFER_ALIGNED_STRIDE,
                            mpi->w, mpi->h);
        copy_mpi(cmpi, mpi);
        buf = cmpi->priv;
    } else {
        buf = mpi->priv;
    }
    buf->video->key_frame = mpi->pict_type == 1;
    buf->video->pict_type = mpi->pict_type; /* seems to be the same code */
    buf->video->interlaced = !!(mpi->fields & MP_IMGFIELD_INTERLACED);
    buf->video->top_field_first = !!(mpi->fields & MP_IMGFIELD_TOP_FIRST);
    vf->priv->in_buf = buf;
    if (pts != MP_NOPTS_VALUE)
        buf->pts = pts * AV_TIME_BASE;
    while (avfilter_poll_frame(vf->priv->out->inputs[0])) {
        if (avfilter_request_frame(vf->priv->out->inputs[0]))
            break;
    }
    return 1;
}

static void uninit(struct vf_instance *vf)
{
    unsigned i;

#define FREE_MPI_ARRAY(field) \
    for (i = 0; i < FF_ARRAY_ELEMS(vf->imgctx.field); i++) \
        if (vf->imgctx.field[i]) \
            avfilter_unref_buffer(vf->imgctx.field[i]->priv);
    FREE_MPI_ARRAY(static_images);
    FREE_MPI_ARRAY(temp_images);
    FREE_MPI_ARRAY(export_images);
    FREE_MPI_ARRAY(numbered_images);
    avfilter_graph_free(&vf->priv->graph);
    av_free(vf->priv);
}

static int lavfi_open(struct vf_instance *vf, char *args)
{
    AVFilterInOut *outputs;
    AVFilterInOut *inputs;
    int ret;

    avfilter_register_all();
    if (!args) {
        mp_msg(MSGT_VFILTER, MSGL_ERR, "lavfi: filtergraph needed\n");
        goto fail;
    }
    if (args[0] == '$') {
        char *e = getenv(args + 1);
        if (!e) {
            mp_msg(MSGT_VFILTER, MSGL_ERR, "lavfi: %s not defined\n", args);
            goto fail;
        }
        args = e;
    }
    vf->priv = av_mallocz(sizeof(struct vf_priv_s));
    if (!vf->priv)
        return 0;

    vf->priv->graph = avfilter_graph_alloc();
    if (!vf->priv->graph)
        goto fail;
    ret = avfilter_graph_create_filter(&vf->priv->in, &mpsrc, "in",
                                       NULL, vf, vf->priv->graph);
    if (ret < 0)
        goto fail;
    ret = avfilter_graph_create_filter(&vf->priv->out, &mpsink, "out",
                                       NULL, vf, vf->priv->graph);
    if (ret < 0)
        return 0;
    outputs = avfilter_inout_alloc();
    inputs  = avfilter_inout_alloc();
    if (!outputs || !inputs)
        goto fail;
    outputs->name = av_strdup("in");
    outputs->filter_ctx = vf->priv->in;
    outputs->pad_idx = 0;
    outputs->next = NULL;
    inputs->name = av_strdup("out");
    inputs->filter_ctx = vf->priv->out;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    ret = avfilter_graph_parse(vf->priv->graph, args, &inputs, &outputs, NULL);
    if (ret < 0)
        goto fail;

    vf->config    = config;
    vf->uninit    = uninit;
    vf->put_image = put_image;
    vf->get_image = get_image;
    return 1;

fail:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    avfilter_graph_free(&vf->priv->graph);
    av_free(vf->priv);
    return 0;
}

static const m_option_t vf_opts_fields[] = { { .name = NULL } };

const vf_info_t vf_info_lavfi = {
    "libavfilter wrapper",
    "lavfi",
    "Nicolas George",
    "",
    lavfi_open,
    NULL,
};
