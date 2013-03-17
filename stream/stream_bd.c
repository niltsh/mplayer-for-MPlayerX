/*
 * Blu-ray stream playback
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
#include <ctype.h>
#include <strings.h>

#include "libavutil/common.h"
#include "libavutil/aes.h"
#include "libavutil/sha.h"
#include "libavutil/avstring.h"
#include "libmpdemux/demuxer.h"
#include "libavutil/intreadwrite.h"
#include "m_struct.h"
#include "m_option.h"
#include "mp_msg.h"
#include "stream.h"
#include "stream_bd.h"

static const int   BD_UNIT_SIZE = 6144;
static const char BD_UKF_PATH[]  = "%s/AACS/Unit_Key_RO.inf";
static const char BD_M2TS_PATH[] = "%s/BDMV/STREAM/%05d.m2ts";
static const char BD_CLIPINF_PATH[] = "%s/BDMV/CLIPINF/%05d.clpi";

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

// This is just a guess
#define LANG_NAME_LEN 20

struct lang_map {
    int id;
    int type;
    char lang_name[LANG_NAME_LEN + 1];
};

struct bd_priv {
    key           vuk;
    key           iv;
    int           title;
    const char   *device;
    stream_t     *title_file;
    struct AVAES *aescbc;
    struct AVAES *aeseed;
    int64_t       pos;
    struct uks    uks;
    int           nr_lang_maps;
    struct lang_map *lang_maps;
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

static int bd_stream_seek(stream_t *s, int64_t pos)
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

#define ID_STR_LEN 41
static void id2str(const uint8_t *id, int idlen, char dst[ID_STR_LEN])
{
    int i;
    idlen = FFMIN(idlen, 20);
    for (i = 0; i < idlen; i++)
        snprintf(dst + 2*i, 3, "%02"PRIX8, id[i]);
}

static int find_vuk(struct bd_priv *bd, const uint8_t discid[20])
{
    char line[1024];
    char filename[PATH_MAX];
    const char *home;
    int vukfound = 0;
    stream_t *file;
    char idstr[ID_STR_LEN];

    // look up discid in KEYDB.cfg to get VUK
    home = getenv("HOME");
    snprintf(filename, sizeof(filename), "%s/.dvdcss/KEYDB.cfg", home);
    file = open_stream(filename, NULL, NULL);
    if (!file) {
        mp_msg(MSGT_OPEN,MSGL_ERR,
               "Cannot open VUK database file %s\n", filename);
        return 0;
    }
    id2str(discid, 20, idstr);
    while (stream_read_line(file, line, sizeof(line), 0)) {
        char *vst;

        // file is built up this way:
        // DISCID = title | V | VUK
        // or
        // DISCID = title | key-pair
        // key-pair = V | VUK
        // or         D | Date
        // or         M | M-key???
        // or         I | I-Key
        // can be followed by ; and comment

        if (strncasecmp(line, idstr, 40))
            continue;
        mp_msg(MSGT_OPEN, MSGL_V, "KeyDB found Entry for DiscID:\n%s\n", line);

        vst = strstr(line, "| V |");
        if (!vst)
            break;
        vst += 6;
        while (isspace(*vst)) vst++;
        if (sscanf(vst,      "%16"SCNx64, &bd->vuk.u64[0]) != 1)
            continue;
        if (sscanf(vst + 16, "%16"SCNx64, &bd->vuk.u64[1]) != 1)
            continue;
        bd->vuk.u64[0] = av_be2ne64(bd->vuk.u64[0]);
        bd->vuk.u64[1] = av_be2ne64(bd->vuk.u64[1]);
        vukfound = 1;
    }
    free_stream(file);
    return vukfound;
}

static int bd_get_uks(struct bd_priv *bd)
{
    unsigned char *buf;
    size_t file_size;
    int pos;
    int i;
    struct AVAES *a;
    struct AVSHA *asha;
    stream_t *file;
    char filename[PATH_MAX];
    uint8_t discid[20];
    char idstr[ID_STR_LEN];

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

    if (!find_vuk(bd, discid)) {
        id2str(discid, 20, idstr);
        mp_msg(MSGT_OPEN, MSGL_ERR,
               "No Volume Unique Key (VUK) found for this Disc: %s\n", idstr);
        av_free(buf);
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
        av_aes_init(a, bd->vuk.u8, 128, 1);

        id2str(discid, 20, idstr);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_BD_DISCID=%s\n", idstr);
        id2str(bd->vuk.u8, 16, idstr);
        mp_msg(MSGT_IDENTIFY, MSGL_INFO, "ID_BD_VUK=%s\n", idstr);

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
static int64_t bd_read(struct bd_priv *bd, uint8_t *buf, int len)
{
    int read_len;
    int unit_offset = bd->pos % BD_UNIT_SIZE;
    len &= ~15;
    if (!len)
        return 0;

    read_len = stream_read(bd->title_file, buf, len);
    if (read_len != len)
        return -1;

    if (unit_offset) {
        int decrypt_len = FFMIN(len, BD_UNIT_SIZE - unit_offset);
        av_aes_crypt(bd->aescbc, buf, buf, decrypt_len / 16, bd->iv.u8, 1);
        buf += decrypt_len;
        len -= decrypt_len;
    }
    while (len) {
        int decrypt_len = FFMIN(len, BD_UNIT_SIZE);
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
                     (decrypt_len - 16) / 16, bd->iv.u8, 1);
        buf += decrypt_len;
        len -= decrypt_len;
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

static int is_video_type(int type)
{
    switch (type) {
    case 1:
    case 2:
    case 0x10:
    case 0x1b:
    case 0xD1:
    case 0xEA:
        return 1;
    }
    return 0;
}

static int is_audio_type(int type)
{
    switch (type) {
    case 3:
    case 4:
    case 0x0f:
    case 0x11:
    case 0x81:
    case 0x8A:
    case 0x82:
    case 0x85:
    case 0x86:
        return 1;
    }
    return 0;
}

static int is_sub_type(int type)
{
    switch (type) {
    case 0x90:
        return 1;
    }
    return 0;
}

static const char *bd_lang_from_id(stream_t *s, int id)
{
    struct bd_priv *bd = s->priv;
    int i;
    for (i = 0; i < bd->nr_lang_maps; i++) {
        if (bd->lang_maps[i].id == id)
            return bd->lang_maps[i].lang_name;
    }
    return NULL;
}

int bd_aid_from_lang(stream_t *s, const char *lang)
{
    struct bd_priv *bd = s->priv;
    int i;
    for (i = 0; i < bd->nr_lang_maps; i++) {
        if (is_audio_type(bd->lang_maps[i].type) &&
            strstr(bd->lang_maps[i].lang_name, lang))
            return bd->lang_maps[i].id;
    }
    return -1;
}

int bd_sid_from_lang(stream_t *s, const char *lang)
{
    struct bd_priv *bd = s->priv;
    int i;
    for (i = 0; i < bd->nr_lang_maps; i++) {
        if (is_sub_type(bd->lang_maps[i].type) &&
            strstr(bd->lang_maps[i].lang_name, lang))
            return bd->lang_maps[i].id;
    }
    return -1;
}

static void get_clipinf(struct bd_priv *bd)
{
    int i;
    int langmap_offset, index_offset, end_offset;
    char filename[PATH_MAX];
    stream_t *file;

    snprintf(filename, sizeof(filename), BD_CLIPINF_PATH, bd->device, bd->title);
    file = open_stream(filename, NULL, NULL);
    if (!file) {
        mp_msg(MSGT_OPEN, MSGL_ERR, "Cannot open clipinf %s\n", filename);
        return;
    }
    if (stream_read_qword(file) != AV_RB64("HDMV0200")) {
        mp_msg(MSGT_OPEN, MSGL_ERR, "Unknown clipinf format\n");
        return;
    }
    stream_read_dword(file); // unknown offset
    langmap_offset = stream_read_dword(file);
    index_offset = stream_read_dword(file);
    end_offset = stream_read_dword(file);

    // read language <-> stream id mappings
    stream_seek(file, langmap_offset);
    stream_read_dword(file); // size
    stream_skip(file, 8); // unknown
    bd->nr_lang_maps = stream_read_char(file); // number of entries
    stream_read_char(file); // unknown

    bd->lang_maps = calloc(bd->nr_lang_maps, sizeof(*bd->lang_maps));
    for (i = 0; i < bd->nr_lang_maps; i++) {
        int type;
        bd->lang_maps[i].id = stream_read_word(file);
        stream_read_char(file); // unknown
        type = stream_read_char(file);
        if (!is_video_type(type) && !is_audio_type(type) && !is_sub_type(type))
            mp_msg(MSGT_OPEN, MSGL_WARN, "Unknown type %x in clipinf\n", type);
        bd->lang_maps[i].type = type;
        stream_read(file, bd->lang_maps[i].lang_name, LANG_NAME_LEN);
    }

    free_stream(file);
}

static int bd_stream_control(stream_t *s, int cmd, void *arg)
{
    switch (cmd) {
    case STREAM_CTRL_GET_LANG:
    {
        struct stream_lang_req *req = arg;
        const char *lang = bd_lang_from_id(s, req->id);
        if (!lang)
            return STREAM_ERROR;
        av_strlcpy(req->buf, lang, sizeof(req->buf));
        return STREAM_OK;
    }
    }
    return STREAM_UNSUPPORTED;
}

static int bd_stream_open(stream_t *s, int mode, void* opts, int* file_format)
{
    char filename[PATH_MAX];

    struct stream_priv_s* p = opts;
    struct bd_priv *bd = calloc(1, sizeof(*bd));

    if (p->device)
        bd->device = p->device;
    else if (bluray_device)
        bd->device = bluray_device;
    else
        bd->device = DEFAULT_BD_DEVICE;

    s->sector_size = BD_UNIT_SIZE;
    s->read_chunk  = 32 * BD_UNIT_SIZE;
    s->flags       = STREAM_READ | MP_STREAM_SEEK;
    s->fill_buffer = bd_stream_fill_buffer;
    s->seek        = bd_stream_seek;
    s->control     = bd_stream_control;
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

    get_clipinf(bd);

    return STREAM_OK;
}

const stream_info_t stream_info_bd = {
    "Blu-ray",
    "bd",
    "cRTrn13",
    "",
    bd_stream_open,
    { "bd", NULL },
    &stream_opts,
    1
};
