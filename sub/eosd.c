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
#include "sub.h"
#include "ass_mp.h"
#include "eosd.h"

static struct mp_eosd_source *sources;
static struct mp_eosd_settings settings;
static struct mp_eosd_image *image_pool;

void eosd_init(vf_instance_t *vf)
{
    vf->control(vf, VFCTRL_INIT_EOSD, NULL);
}

void eosd_register(struct mp_eosd_source *src)
{
    struct mp_eosd_source *p, **prev = &sources;
    for (p = sources; p && p->z_index < src->z_index; p = p->priv_next)
        prev = &p->priv_next;
    src->priv_next = p;
    *prev          = src;
    src->initialized = 0;
}

int eosd_registered(struct mp_eosd_source *source)
{
    struct mp_eosd_source *p;
    for (p = sources; p; p = p->priv_next)
        if (p == source)
            return 1;
    return 0;
}

void eosd_configure(struct mp_eosd_settings *res)
{
    if (res->w        != settings.w        ||
        res->h        != settings.h        ||
        res->srcw     != settings.srcw     ||
        res->srch     != settings.srch     ||
        res->mt       != settings.mt       ||
        res->mt       != settings.mb       ||
        res->mt       != settings.ml       ||
        res->mt       != settings.mr       ||
        res->unscaled != settings.unscaled) {
        settings         = *res;
        settings.changed = 1;
    }
}

void eosd_render_frame(double ts, struct mp_eosd_image_list *images)
{
    struct mp_eosd_source *src;
    int changed = 0;
    for (src = sources; src; src = src->priv_next) {
        if (src->update)
            src->update(src, &settings, ts);
        changed |= src->changed;
        src->changed = 0;
    }
    settings.changed = 0;
    images->first_source = sources;
    images->changed      = changed;
}

void eosd_uninit(void)
{
    struct mp_eosd_source *src;
    for (src = sources; src; src = src->priv_next) {
        // TODO: maybe only call if src->initialized is set.
        if (src->uninit)
            src->uninit(src);
        src->initialized = 0;
    }
}

struct mp_eosd_image *eosd_image_alloc(void)
{
    struct mp_eosd_image *r;
    if (!image_pool) {
        const unsigned n_alloc = 127; /* arbitrary */
        unsigned i;
        image_pool = calloc(n_alloc, sizeof(*image_pool));
        for (i = 0; i < n_alloc - 1; i++)
            image_pool[i].next = image_pool + i + 1;
        image_pool[i].next = NULL;
    }
    r          = image_pool;
    image_pool = image_pool->next;
    return r;
}

void eosd_image_free(struct mp_eosd_image *image)
{
    image->next = image_pool;
    image_pool  = image;
}

void eosd_image_append(struct mp_eosd_source *source,
                       struct mp_eosd_image *image)
{
    image->next          = NULL;
    *source->images_tail = image;
    source->images_tail  = &image->next;
}

void eosd_image_remove(struct mp_eosd_source *source,
                       struct mp_eosd_image *image,
                       struct mp_eosd_image **prev)
{
    *prev = image->next;
    if (!*prev)
        source->images_tail = prev;
    eosd_image_free(image);
}

void eosd_image_remove_all(struct mp_eosd_source *source)
{
    struct mp_eosd_image *image;

    while (source->images) {
        image          = source->images;
        source->images = source->images->next;
        eosd_image_free(image);
    }
    source->images_tail = &source->images;
}

static void next_image_in_sources(struct mp_eosd_image_list *images,
                                  struct mp_eosd_source *src)
{
    images->source = src;
    while (images->source && !images->source->images)
        images->source = images->source->priv_next;
    images->image = images->source ? images->source->images : NULL;
}

struct mp_eosd_image *eosd_image_first(struct mp_eosd_image_list *images)
{
    next_image_in_sources(images, images->first_source);
    return images->image;
}

struct mp_eosd_image *eosd_image_next(struct mp_eosd_image_list *images)
{
    images->image = images->image->next;
    if (!images->image)
        next_image_in_sources(images, images->source->priv_next);
    return images->image;
}
