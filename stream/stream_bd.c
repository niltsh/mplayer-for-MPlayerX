/*
 * Bluray stream playback
 * by cRTrn13 <crtrn13-at-gmail.com> 2009
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

#include <stdio.h>
#include <limits.h>
#include "libavutil/common.h"
#include "libavutil/aes.h"
#include "libavutil/sha.h"
#include "libmpdemux/demuxer.h"
#include "libavutil/intreadwrite.h"
#include "m_struct.h"
#include "m_option.h"
#include "stream.h"

static const int   BD_UNIT_SIZE = 6144;
static const char BD_UKF_PATH[]  = "%s/AACS/Unit_Key_RO.inf";
static const char BD_M2TS_PATH[] = "%s/BDMV/STREAM/%05d.m2ts";

static const char DEFAULT_BD_DEVICE[] = "/mnt/bd";

static const struct stream_priv_s {
    int   title;
    char *device;
} stream_priv_dflts = {
    0,
    NULL
};

// Format: bd://[title][</mntlocation>]
// --> e.g.: bd://117/media/THE_MUMMY/
// --> or   bd://152
// instead of directly, mount location can also be gotten through -dvd-device
#define ST_OFF(f) M_ST_OFF(struct stream_priv_s,f)
static const m_option_t stream_opts_fields[] = {
    { "hostname", ST_OFF(title),  CONF_TYPE_INT, M_OPT_RANGE, 0, 99999, NULL},
    { "filename", ST_OFF(device), CONF_TYPE_STRING, 0, 0 ,0, NULL},
    { NULL, NULL, 0, 0, 0, 0,  NULL }
};

static const struct m_struct_st stream_opts = {
    "bd",
    sizeof(struct stream_priv_s),
    &stream_priv_dflts,
    stream_opts_fields
};

typedef union {
    uint64_t u64[2];
    uint8_t  u8[16];
} key;

static const key BD_CBC_IV = {
    .u8 = {
    0x0b, 0xa0, 0xf8, 0xdd, 0xfe, 0xa6, 0x1f, 0xb3,
    0xd8, 0xdf, 0x9f, 0x56, 0x6a, 0x05, 0x0f, 0x78
    }
};

struct uks {
    int  count;
    key *keys;
};

struct bd_priv {
    key           vuk;
    key           iv;
    int           title;
    const char   *device;
    stream_t     *title_file;
    struct AVAES *aescbc;
    struct AVAES *aeseed;
    off_t         pos;
    struct uks    uks;
};

static void bd_stream_close(stream_t *s)
{
    struct bd_priv *bd = s->priv;
    free_stream(bd->title_file);
    av_free(bd->aescbc);
    av_free(bd->aeseed);
    free(bd->uks.keys);
    free(bd);
}

static int bd_stream_seek(stream_t *s, off_t pos)
{
    struct bd_priv *bd = s->priv;

    // must seek to start of unit
    pos -= pos % BD_UNIT_SIZE;
    if (!stream_seek(bd->title_file, pos)) {
        s->eof = 1;
        return 0;
    }

    bd->pos = pos;
    s->pos  = pos;

    return 1;
}

static int bd_get_uks(struct bd_priv *bd)
{
    unsigned char *buf;
    size_t file_size;
    int pos;
    int i, j;
    struct AVAES *a;
    struct AVSHA *asha;
    stream_t *file;
    char filename[PATH_MAX];
    uint8_t discid[20];
    char *home;
    int vukfound = 0;

    snprintf(filename, sizeof(filename), BD_UKF_PATH, bd->device);
    file = open_stream(filename, NULL, NULL);
    if (!file) {
        mp_msg(MSGT_OPEN, MSGL_ERR,
               "Cannot open file %s to get UK and DiscID\n", filename);
        return 0;
    }
    file_size = file->end_pos;
    if (file_size <= 0 || file_size > 10 * 1024* 1024) {
        mp_msg(MSGT_OPEN, MSGL_ERR, "File %s too large\n", filename);
        free_stream(file);
        return 0;
    }
    buf = av_malloc(file_size);
    stream_read(file, buf, file_size);
    free_stream(file);

    // get discid from file
    asha = av_malloc(av_sha_size);
    av_sha_init(asha, 160);
    av_sha_update(asha, buf, file_size);
    av_sha_final(asha, discid);
    av_free(asha);

    // look up discid in KEYDB.cfg to get VUK
    home = getenv("HOME");
    snprintf(filename, sizeof(filename), "%s/.dvdcss/KEYDB.cfg", home);
    file = open_stream(filename, NULL, NULL);
    if (!file) {
        mp_msg(MSGT_OPEN,MSGL_ERR,
               "Cannot open VUK database file %s\n", filename);
        av_free(buf);
        return 0;
    }
    while (!stream_eof(file)) {
        char line[1024];
        uint8_t id[20];
        char d[200];
        char *vst;
        unsigned int byte;

        stream_read_line(file, line, sizeof(line), 0);
        // file is built up this way:
        // DISCID = title | V | VUK
        // or
        // DISCID = title | key-pair
        // key-pair = V | VUK
        // or         D | Date
        // or         M | M-key???
        // or         I | I-Key
        // can be followed by ; and comment

        //This means: first string up to whitespace is discid
        sscanf(line, "%40s", d);
        for (i = 0; i < 20; ++i) {
            if (sscanf(&d[i*2], "%2x", &byte) != 1)
                break;
            id[i] = byte;
        }
        if (memcmp(id, discid, 20) != 0)
            continue;
        mp_msg(MSGT_OPEN, MSGL_V, "KeyDB found Entry for DiscID:\n%s\n", line);

        vst = strstr(line, "| V |");
        if (vst == 0)
            break;
        sscanf(&vst[6], "%32s", d);
        for (i = 0; i < 16; i++) {
            if (sscanf(&d[i*2], "%2x", &byte) != 1)
                break;
            bd->vuk.u8[i] = byte;
        }
        vukfound = 1;
    }
    free_stream(file);
    if (!vukfound) {
        mp_msg(MSGT_OPEN, MSGL_ERR,
               "No Volume Unique Key (VUK) found for this Disc: ");
        for (j = 0; j < 20; j++) mp_msg(MSGT_OPEN, MSGL_ERR, "%02x", discid[j]);
        mp_msg(MSGT_OPEN, MSGL_ERR, "\n");
        return 0;
    }

    pos = AV_RB32(buf);
    if (pos < file_size) {
        int key_pos = pos + 48;
        int max_count = (file_size - key_pos - 16) / 48;
        bd->uks.count = AV_RB16(&buf[pos]);
        if (max_count < bd->uks.count) {
            mp_msg(MSGT_OPEN, MSGL_ERR,
                   "File to small for key count %i, using %i\n",
                   bd->uks.count, max_count);
            bd->uks.count = max_count;
        }
        bd->uks.keys  = calloc(bd->uks.count, sizeof(key));

        a = av_malloc(av_aes_size);
        j = av_aes_init(a, bd->vuk.u8, 128, 1);

        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_BD_DISCID=");
        for (j = 0; j < 20; j++)
            mp_msg(MSGT_IDENTIFY, MSGL_INFO, "%02x", discid[j]);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "\n");
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_BD_VUK=");
        for (j = 0; j < 20; j++)
            mp_msg(MSGT_IDENTIFY, MSGL_INFO, "%02x", discid[j]);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "\n");

        for (i = 0; i < bd->uks.count; i++) {
            av_aes_crypt(a, bd->uks.keys[i].u8, &buf[key_pos], 1, NULL, 1); // decrypt unit key
            key_pos += 48;
        }

        av_free(a);
    }

    av_free(buf);
    return 1;
}

// NOTE: we assume buf is sufficiently aligned to 64 bit read/writes
static off_t bd_read(struct bd_priv *bd, uint8_t *buf, int len)
{
    int read_len;
    len &= ~15;
    if (!len)
        return 0;

    read_len = stream_read(bd->title_file, buf, len);
    if (read_len != len)
        return -1;

    if (bd->pos % BD_UNIT_SIZE) {
        // decrypt in place
        av_aes_crypt(bd->aescbc, buf, buf, len / 16, bd->iv.u8, 1);
    } else {
        // reset aes context as at start of unit
        key enc_seed;
        bd->iv = BD_CBC_IV;

        // perform encryption of first 16 bytes of unit (seed)
        av_aes_crypt(bd->aeseed, enc_seed.u8, buf, 1, NULL, 0);

        // perform xor
        enc_seed.u64[0] ^= AV_RN64A(buf);
        enc_seed.u64[1] ^= AV_RN64A(buf + 8);

        // set uk AES-CBC key from enc_seed and BD_CBC_IV
        av_aes_init(bd->aescbc, enc_seed.u8, 128, 1);

        // decrypt
        av_aes_crypt(bd->aescbc, &buf[16], &buf[16],
                     (len - 16) / 16, bd->iv.u8, 1);
    }

    bd->pos += read_len;

    return read_len;
}

static int bd_stream_fill_buffer(stream_t *s, char *buf, int len)
{
    int read_len;
    struct bd_priv *bd = s->priv;

    read_len = bd_read(bd, buf, len);

    s->pos = bd->pos;

    return read_len;
}

static int bd_stream_open(stream_t *s, int mode, void* opts, int* file_format)
{
    char filename[PATH_MAX];

    struct stream_priv_s* p = opts;
    struct bd_priv *bd = calloc(1, sizeof(*bd));

    if (p->device)
        bd->device = p->device;
    else if (dvd_device)
        bd->device = dvd_device;
    else
        bd->device = DEFAULT_BD_DEVICE;

    s->sector_size = BD_UNIT_SIZE;
    s->flags       = STREAM_READ | MP_STREAM_SEEK;
    s->fill_buffer = bd_stream_fill_buffer;
    s->seek        = bd_stream_seek;
    s->close       = bd_stream_close;
    s->start_pos   = 0;
    s->priv        = bd;
    s->type        = STREAMTYPE_BD;
    s->url         = strdup("bd://");

    bd->pos   = 0;
    bd->title = p->title;

    // get and decrypt unit keys
    if (!bd_get_uks(bd))
        return STREAM_ERROR;

    bd->aescbc = av_malloc(av_aes_size);
    bd->aeseed = av_malloc(av_aes_size);

    // set up AES key from uk
    av_aes_init(bd->aeseed, bd->uks.keys[0].u8, 128, 0);

    snprintf(filename, sizeof(filename), BD_M2TS_PATH, bd->device, bd->title);
    mp_msg(MSGT_OPEN, MSGL_STATUS, "Opening %s\n", filename);
    bd->title_file = open_stream(filename, NULL, NULL);
    if (!bd->title_file)
        return STREAM_ERROR;
    s->end_pos = bd->title_file->end_pos;

    return STREAM_OK;
}

const stream_info_t stream_info_bd = {
    "Bluray",
    "bd",
    "cRTrn13",
    "",
    bd_stream_open,
    { "bd", NULL },
    &stream_opts,
    1
};
