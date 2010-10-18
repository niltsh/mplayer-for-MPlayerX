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

#include <stdint.h>
#include "libmpcodecs/vf.h"

enum {
    EOSD_CHANGED_LAYOUT = 0x1,
    EOSD_CHANGED_BITMAP = 0x2,
};

struct mp_eosd_settings {
    int w, h;           ///< screen dimensions, including black borders
    int srcw, srch;     ///< unscaled source dimensions
    int mt, mb, ml, mr; ///< borders (top, bottom, left, right)
    int unscaled;       ///< EOSD objects are rendered at native resolution
    int changed;        ///< settings have changed since last update
};

struct mp_eosd_image {
    struct mp_eosd_image *next; ///< Next image, or NULL
    uint8_t *bitmap;            ///< 1bpp stride*h alpha buffer
    void *opaque;               ///< Arbitrary value for the client's use
    int w, h;                   ///< Bitmap width, height
    int stride;                 ///< Bitmap stride
    uint32_t color;             ///< Bitmap color and transparency, RGBT
                                ///  T is the complement of A (alpha=opacity).
    int dst_x, dst_y;           ///< Bitmap placement inside the video frame
};

struct mp_eosd_source {

    /**
     * Linked list of images element.
     * The client is responsible for initializing and maintaining this list.
     * It can alter it at any time in the main MPlayer thread.
     */
    struct mp_eosd_image *images;

    /**
     * Pointer to the next field of the last image, or to images if the list
     * is empty.
     * The client is not required to handle this field, but list
     * manipulation functions (see below) use it.
     */
    struct mp_eosd_image **images_tail;

    /**
     * Callback to update the images. Can be NULL.
     */
    void (*update)(struct mp_eosd_source *, const struct mp_eosd_settings *,
                   double);

    /**
     * Callback to uninit the source. Can be NULL.
     */
    void (*uninit)(struct mp_eosd_source *);

    /**
     * Changed flags of the images.
     * The client must set it to a combination of EOSD_CHANGED_* whenever
     * the images are altered.
     * The core EOSD system resets it.
     */
    int changed;

    /**
     * Z-index of the images.
     * Images with a higher Z-index are rendered on top.
     */
    int z_index;

    /**
     * Initialized flag of the source.
     * Set by the source, automatically cleared when a source is added,
     * removed or reinitialized.
     */
    int initialized;

    struct mp_eosd_source *priv_next;
};

struct mp_eosd_image_list {
    struct mp_eosd_source *first_source;
    struct mp_eosd_source *source;
    struct mp_eosd_image *image;
    int changed;
};

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
 */
void eosd_configure(struct mp_eosd_settings *res);

/**
 * Renders the EOSD elements for the current frame.
 * Should be called by the rendering engine when it is about to do or
 * prepare the rendering.
 *
 * @param[in]  ts      presentation timestamp of the frame.
 * @param[out] images  list of images to render.
 *                     The list and list elements are only valid until any
 *                     client alter them.
 *                     The renderer should therefore not call anything that
 *                     may alter the EOSD elements.
 */
void eosd_render_frame(double ts, struct mp_eosd_image_list *images);

/**
 * Shut down the EOSD subsystem and free the associated resources.
 */
void eosd_uninit(void);

/**
 * Register a source of EOSD images.
 */
void eosd_register(struct mp_eosd_source *source);

/**
 * Test whether a source has already been registered.
 */
int eosd_registered(struct mp_eosd_source *source);

/**
 * Allocate a structure for an EOSD image.
 */
struct mp_eosd_image *eosd_image_alloc(void);

/**
 * Free a previously allocated structure.
 */
void eosd_image_free(struct mp_eosd_image *image);

/**
 * Append an image to the list of images associated to a source.
 * This function requires that the images_tail pointer is correctly set.
 */
void eosd_image_append(struct mp_eosd_source *source,
                       struct mp_eosd_image *image);

/**
 * Remove an image from the list of images associated to a source.
 * The image structure is freed using eosd_image_free.
 *
 * @param source  source where the image is.
 * @param image   image to remove.
 * @param prev    pointeur to the prev field of the previous image,
 *                or to source->images if this is the first image.
 */
void eosd_image_remove(struct mp_eosd_source *source,
                       struct mp_eosd_image *image,
                       struct mp_eosd_image **prev);

/**
 * Remove all images associated to a source and free the corresponding
 * structures.
 * This function also resets the images_tail pointer.
 */
void eosd_image_remove_all(struct mp_eosd_source *source);

/**
 * Reset the cursor of an image list and get the first image.
 */
struct mp_eosd_image *eosd_image_first(struct mp_eosd_image_list *images);

/**
 * Get the next image in an image list.
 * The renderer must NOT use the next field in the image structure.
 */
struct mp_eosd_image *eosd_image_next(struct mp_eosd_image_list *images);

#endif /* MPLAYER_EOSD_H */
