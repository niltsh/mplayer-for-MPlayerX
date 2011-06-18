/*
 * MPlayer output to MNG file
 *
 * Copyright (C) 2011 Stefan Schuermans <stefan blinkenarea org>
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <zlib.h>

#define MNG_INCLUDE_WRITE_PROCS
#define MNG_ACCESS_CHUNKS
#define MNG_SUPPORT_READ
#define MNG_SUPPORT_DISPLAY
#define MNG_SUPPORT_WRITE
#include <libmng.h>

#include "video_out.h"
#include "video_out_internal.h"
#include "mp_msg.h"
#include "subopt-helper.h"

#define VOMNG_DEFAULT_DELAY_MS (100) /* default delay of a frame */

static vo_info_t info = {
    .name       = "MNG file",
    .short_name = "mng",
    .author     = "Stefan Schuermans <stefan blinkenarea org>"
};

LIBVO_EXTERN(mng)

/* a frame to be written to the MNG file */
struct vomng_frame {
    mng_ptr data;             /**< deflate compressed data, malloc-ed */
    mng_uint32 len;           /**< length of compressed data */
    unsigned int time_ms;     /**< timestamp of frame (in ms) */
    struct vomng_frame *next; /**< next frame */
};

/* properties of MNG output */
struct vomng_properties {
    char               *out_file_name; /**< name of output file, malloc-ed */
    unsigned int       width, height;  /**< dimensions */
    unsigned char      *canvas;        /**< canvas for frame,
                                            canvas := row ... row,
                                            row    := filter_id pix ... pix,
                                            pix    := red green blue */
    struct vomng_frame *frame_first;   /**< list of frames */
    struct vomng_frame *frame_last;
    int                is_init;        /**< if initialized */
};

/* private data of MNG vo module */
static struct vomng_properties vomng;

/**
 * @brief libmng callback: allocate memory
 * @param[in] size size of requested memory block
 * @return pointer to memory block, which is initialized to zero
 */
static mng_ptr vomng_alloc(mng_size_t size)
{
    return calloc(1, size);
}

/**
 * @brief libmng callback: free memory
 * @param[in] pointer to memory block
 * @param[in] size size of requested memory block
 */
static void vomng_free(mng_ptr ptr, mng_size_t size)
{
    free(ptr);
}

/**
 * @brief libmng callback: open stream
 * @param[in] mng libmng handle
 * @return if stream could be opened
 */
static mng_bool vomng_openstream(mng_handle mng)
{
    return MNG_TRUE; /* stream is always open wen we get here,
                        tell libmng that everything is okay */
}

/**
 * @brief libmng callback: stream should be closed
 * @param[in] mng libmng handle
 * @return if stream could be closed
 */
static mng_bool vomng_closestream(mng_handle mng)
{
    return MNG_TRUE; /* stream will be closed later,
                        tell libmng that everything is okay */
}

/**
 * @brief libmng callback: write libmng data to the open stream
 * @param[in] mng libmng handle
 * @param[in] *buf pointer to data to write
 * @param[in] size size of data to write
 * @param[out] *written size of data written
 * @return if data was written successfully
 */
static mng_bool vomng_writedata(mng_handle mng, mng_ptr buf,
                                mng_uint32 size, mng_uint32 *written)
{
    FILE *file = mng_get_userdata(mng);
    *written = fwrite(buf, 1, size, file);
    /* according to the example in libmng documentation, true is always
       returned here, short writes can be detected by libmng via *written */
    return MNG_TRUE;
}

/**
 * @brief compress frame data
 * @param[in] width width of canvas
 * @param[in] height height of canvas
 * @param[in] *canvas data on canvas (including MNG filter IDs)
 * @param[out] *out_ptr pointer to compressed data, malloc-ed
 * @param[out] *out_len length of compressed data
 */
static void vomng_canvas_to_compressed(unsigned int width, unsigned int height,
                                       const unsigned char *canvas,
                                       mng_ptr *out_ptr, mng_uint32 *out_len)
{
    mng_uint32 raw_len;
    unsigned char *ptr;
    unsigned long len;

    /* default: no output */
    *out_ptr = NULL;
    *out_len = 0;

    /* raw_len := length of input data
        - it will be significantly shorter than 32 bit
        - the "1 +" is needed because each row starts with the filter ID */
    raw_len = height * (1 + width * 3);

    /* compress data
        - compress2 output size will be smaller than raw_len * 1.001 + 12 (see
          man page), so calculate the next larger integer value in len and
          allocate abuffer of this size
        - len will still contain a value shorter than 32 bit */
    len = raw_len + (raw_len + 999) / 1000 + 12;
    ptr = malloc(len);
    if (!ptr)
        return;
    compress2(ptr, &len, canvas, raw_len, Z_BEST_COMPRESSION);

    /* return allocated compressed data
        - we have to convert the output length to a shorter data type as
          libmng does not accept an unsigned long as length
        - convert here, because we can see here that the conversion is safe
           - see comments about raw_len and len above
           - compress2 never increases value of len */
    *out_ptr = ptr;
    *out_len = len;
}

/**
 * @brief write frame to MNG file
 * @param[in] *frame the frame to write to MNG file
 * @param[in] mng libmng handle
 * @param[in] width width of canvas
 * @param[in] height height of canvas
 * @param[in] first_frame if the frame is the first one in the file
 * @return 0 on success, 1 on error
 */
static int vomng_write_frame(struct vomng_frame *frame, mng_handle mng,
                             unsigned int width, unsigned int height,
                             int first_frame)
{
    unsigned int delay_ms;

    /* determine delay */
    if (frame->next)
        delay_ms = frame->next->time_ms - frame->time_ms;
    else
        delay_ms = VOMNG_DEFAULT_DELAY_MS; /* default delay for last frame */

    /* write frame headers to MNG file */
    if (mng_putchunk_seek(mng, 0, MNG_NULL)) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing SEEK chunk failed\n");
        return 1;
    }
    if (mng_putchunk_fram(mng, MNG_FALSE,
                          /* keep canvas if not 1st frame */
                          first_frame ? MNG_FRAMINGMODE_1
                                      : MNG_FRAMINGMODE_NOCHANGE,
                          0, MNG_NULL,              /* no frame name */
                          MNG_CHANGEDELAY_DEFAULT,  /* change only delay */
                          MNG_CHANGETIMOUT_NO,
                          MNG_CHANGECLIPPING_NO,
                          MNG_CHANGESYNCID_NO,
                          delay_ms,                 /* new delay */
                          0,                        /* no new timeout */
                          0, 0, 0, 0, 0,            /* no new boundary */
                          0, 0)) {                  /* no count, no IDs */
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing FRAM chunk failed\n");
        return 1;
    }
    if (mng_putchunk_defi(mng, 0,                   /* no ID */
                          MNG_DONOTSHOW_VISIBLE,
                          MNG_ABSTRACT,
                          MNG_TRUE, 0, 0,           /* top left location */
                          MNG_FALSE, 0, 0, 0, 0)) { /* no clipping */
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing DEFI chunk failed\n");
        return 1;
    }
    if (mng_putchunk_ihdr(mng, width, height,       /* dimensions */
                          8, MNG_COLORTYPE_RGB,     /* RBG */
                          MNG_COMPRESSION_DEFLATE,
                          MNG_FILTER_ADAPTIVE,
                          MNG_INTERLACE_NONE)) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing IHDR chunk failed\n");
        return 1;
    }

    /* write frame data */
    if (mng_putchunk_idat(mng, frame->len, frame->data)) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing IDAT chunk failed\n");
        return 1;
    }

    /* write frame footers to MNG file */
    if (mng_putchunk_iend(mng)) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing IEND chunk failed\n");
        return 1;
    }

    return 0;
}

/**
 * @brief write buffered frames to MNG file
 * @return 0 on success, 1 on error
 */
static int vomng_write_file(void)
{
    FILE *file;
    mng_handle mng;
    struct vomng_frame *frame;
    unsigned int frames, duration_ms;
    int first;

    /* refuse to create empty MNG file */
    if (!vomng.frame_first || !vomng.frame_last) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: not creating empty file\n");
        return 1;
    }

    /* create output file */
    file = fopen(vomng.out_file_name, "wb");
    if (!file) {
        mp_msg(MSGT_VO, MSGL_ERR,
               "vomng: could not open output file \"%s\": %s\n",
               vomng.out_file_name, strerror(errno));
        return 1;
    }

    /* inititalize MNG library */
    mng = mng_initialize(file, vomng_alloc, vomng_free, MNG_NULL);
    if (!mng) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: could not initialize libmng\n");
        fclose(file);
        return 1;
    }
    if (mng_setcb_openstream (mng, vomng_openstream ) ||
        mng_setcb_closestream(mng, vomng_closestream) ||
        mng_setcb_writedata  (mng, vomng_writedata  )) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: cannot set callbacks for libmng\n");
        mng_cleanup(&mng);
        fclose(file);
        return 1;
    }

    /* create new MNG image in memory */
    if (mng_create(mng)) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: cannot create MNG image in memory\n");
        mng_cleanup(&mng);
        fclose(file);
        return 1;
    }

    /* determine number of frames and total duration */
    frames = 0;
    for (frame = vomng.frame_first; frame; frame = frame->next)
        frames++;
    duration_ms = vomng.frame_last->time_ms - vomng.frame_first->time_ms;

    /* write MNG header chunks */
    if (mng_putchunk_mhdr(mng,
                          vomng.width,          /* dimensions */
                          vomng.height,
                          1000, 0,              /* ticks per second, layer */
                          frames,               /* number of frames */
                          duration_ms,          /* total duration */
                          MNG_SIMPLICITY_VALID |
                          MNG_SIMPLICITY_SIMPLEFEATURES |
                          MNG_SIMPLICITY_COMPLEXFEATURES) ||
        mng_putchunk_save(mng,
                          MNG_TRUE, 0, 0) ||    /* empty save chunk */
        mng_putchunk_term(mng,
                          MNG_TERMACTION_CLEAR, /* show last frame forever */
                          MNG_ITERACTION_CLEAR,
                          0, 0)) {
        mp_msg(MSGT_VO, MSGL_ERR,
               "vomng: writing MHDR/SAVE/TERM chunks failed\n");
        mng_write(mng); /* write out buffered chunks before cleanup */
        mng_cleanup(&mng);
        fclose(file);
        return 1;
    }

    /* write frames */
    first = 1;
    for (frame = vomng.frame_first; frame; frame = frame->next) {
        if (vomng_write_frame(frame, mng, vomng.width, vomng.height, first))
            break;
        first = 0;
    }
    if (frame) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing frames failed\n");
        mng_write(mng); /* write out buffered chunks before cleanup */
        mng_cleanup(&mng);
        fclose(file);
        return 1;
    }

    /* write MNG end chunk */
    if (mng_putchunk_mend(mng)) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: writing end chunk failed\n");
        mng_write(mng); /* write out buffered chunks before cleanup */
        mng_cleanup(&mng);
        fclose(file);
        return 1;
    }

    /* finish and cleanup */
    mng_write(mng); /* write out buffered chunks before cleanup */
    mng_cleanup(&mng);
    fclose(file);

    return 0;
}

/**
 * @brief close all files and free all memory of MNG vo module
 */
static void vomng_prop_reset(void)
{
    struct vomng_frame *frame, *next;

    /* we are initialized properly */
    if (vomng.is_init) {
        /* write buffered frames to MNG file */
        if (vomng_write_file())
            mp_msg(MSGT_VO, MSGL_ERR,
                   "vomng: writing output file failed\n");
    }

    /* reset state */
    vomng.is_init = 0;
    if (vomng.frame_first) {
        frame = vomng.frame_first;
        while (frame) {
            next = frame->next;
            free(frame->data);
            free(frame);
            frame = next;
        }
        vomng.frame_first = NULL;
        vomng.frame_last  = NULL;
    }
    free(vomng.canvas);
    vomng.canvas = NULL;
    vomng.width  = 0;
    vomng.height = 0;
}

/**
 * @brief close files, free memory and delete private data of MNG von module
 */
static void vomng_prop_cleanup(void)
{
    vomng_prop_reset();
    free(vomng.out_file_name);
}

/**
 * @brief configure MNG vo module
 * @param[in] width video width
 * @param[in] height video height
 * @param[in] d_width (unused)
 * @param[in] d_height (unused)
 * @param[in] flags (unused)
 * @param[in] title (unused)
 * @param[in] format video frame format
 * @return 0 on success, 1 on error
 */
static int config(uint32_t width, uint32_t height,
                  uint32_t d_width, uint32_t d_height,
                  uint32_t flags, char *title, uint32_t format)
{
    uint32_t row_stride, y;

    /* reset state */
    vomng_prop_reset();

    /* check format */
    if (format != IMGFMT_RGB24) {
        mp_msg(MSGT_VO, MSGL_ERR,
               "vomng: config with invalid format (!= IMGFMT_RGB24)\n");
        return 1;
    }

    /* allocate canvas */
    vomng.width  = width;
    vomng.height = height;
    row_stride   = 1 + width * 3; /* rows contain filter IDs */
    vomng.canvas = calloc(height * row_stride, 1);
    if (!vomng.canvas) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: out of memory\n");
        return 1;
    }
    /* fill in filter IDs for rows */
    for (y = 0; y < height; y++)
        *(vomng.canvas + row_stride * y) = MNG_FILTER_NONE;

    /* we are initialized */
    vomng.is_init = 1;

    return 0;
}

/**
 * @brief draw on screen display (unsupported for MNG vo module)
 */
static void draw_osd(void)
{
}

/**
 * @brief display data currently on canvas
 */
static void flip_page(void)
{
    unsigned int last_ms;
    struct vomng_frame *frame;

    /* get time of last frame in ms
       (intensive testing showed that the time obtained from vo_pts
       is the time of the previous frame) */
    last_ms = (unsigned int)(vo_pts / 90.0 + 0.5);

    /* set time of last frame */
    if (vomng.frame_last)
        vomng.frame_last->time_ms = last_ms;

    /* create new frame */
    frame = calloc(1, sizeof(*frame));
    if (!frame) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: out of memory\n");
        return;
    }
    /* time of frame is not yet known (see comment about vo_pts about 20
       lines above), approximate time using time of last frame and the
       default frame delay */
    frame->time_ms = last_ms + VOMNG_DEFAULT_DELAY_MS;
    frame->next    = NULL;

    /* compress canvas data */
    vomng_canvas_to_compressed(vomng.width, vomng.height, vomng.canvas,
                               &frame->data, &frame->len);
    if (!frame->data) {
        mp_msg(MSGT_VO, MSGL_ERR, "vomng: compressing frame failed\n");
        free(frame);
        return;
    }

    /* add frame to list */
    if (!vomng.frame_first || !vomng.frame_last) {
        vomng.frame_first = frame;
        vomng.frame_last  = frame;
    } else {
        vomng.frame_last->next = frame;
        vomng.frame_last       = frame;
    }
}

/**
 * @brief put frame data onto canvas (not supported)
 * @return always 1 to indicate error
 */
static int draw_frame(uint8_t *src[])
{
    /* draw_frame() not supported
     * VFCAP_ACCEPT_STRIDE is set for format
     * so draw_slice() will be called instead of this function */
    return 1;
}

/**
 * @brief deinitialize MNG vo module
 */
static void uninit(void)
{
    vomng_prop_cleanup();
}

/**
 * @brief deal with events (not supported)
 */
static void check_events(void)
{
}

/**
 * @brief put a slice of frame data onto canvas
 * @param[in] srcimg pointer to data
 * @param[in] stride line stride in data
 * @param[in] wf frame slice width
 * @param[in] hf frame slice height
 * @param[in] xf leftmost x coordinate of frame slice
 * @param[in] yf topmost y coordinate of frame slice
 * @return always 0 to indicate success
 */
static int draw_slice(uint8_t *srcimg[], int stride[],
                      int wf, int hf, int xf, int yf)
{
    uint8_t *line_ptr;
    int line_len, row_stride, y;

    /* put pixel data from slice to canvas */
    line_ptr   = srcimg[0];
    line_len   = stride[0];
    row_stride = 1 + vomng.width * 3; /* rows contain filter IDs */
    for (y = 0; y < hf; y++)
        memcpy(vomng.canvas + (yf + y) * row_stride + 1 + xf * 3,
               line_ptr + y * line_len, wf * 3);

    return 0;
}

/** list of suboptions */
static const opt_t subopts[] = {
    {"output", OPT_ARG_MSTRZ, &vomng.out_file_name, NULL},
    {NULL,     0,             NULL,                 NULL}
};

/**
 * @brief pre-initialize MNG vo module
 * @param[in] *arg arguments passed to MNG vo module (output file name)
 * @return 0 on success, 1 on error
 */
static int preinit(const char *arg)
{
    if (subopt_parse(arg, subopts)) {
        mp_msg(MSGT_VO, MSGL_ERR,
               "\n-vo mng command line help:\n"
               "Example: mplayer -vo mng:output=file.mng\n"
               "\nOptions:\n"
               "  output=<filename>\n"
               "    Specify the output file.  The default is out.mng.\n"
               "\n");
        vomng_prop_cleanup();
        return 1;
    }
    if (!vomng.out_file_name)
        vomng.out_file_name = strdup("out.mng");

    return 0;
}

/**
 * @brief get supported formats
 * @param[in] format format to check support for
 * @return acceptance flags
 */
static int query_format(uint32_t format)
{
    if (format == IMGFMT_RGB24)
        return VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW |
            VFCAP_ACCEPT_STRIDE;
    return 0;
}

/**
 * @brief handle control stuff
 * @param[in] request control request
 * @param[in] *data data (dependent on control request)
 * @return response to control request
 */
static int control(uint32_t request, void *data)
{
    switch (request) {
    case VOCTRL_QUERY_FORMAT:
        return query_format(*((uint32_t *)data));
    }
    return VO_NOTIMPL;
}

