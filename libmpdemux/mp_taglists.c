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

#include "config.h"

#include <stdint.h>
#include "mp_msg.h"
#include "mp_taglists.h"
#include "libavutil/common.h"
#include "libavformat/avformat.h"
// for AVCodecTag
#include "libavformat/internal.h"

static const struct AVCodecTag mp_wav_tags[] = {
    { AV_CODEC_ID_ADPCM_4XM,         MKTAG('4', 'X', 'M', 'A') },
    { AV_CODEC_ID_ADPCM_ADX,         MKTAG('S', 'a', 'd', 'x') },
#if LIBAVUTIL_VERSION_MICRO >= 100
    { AV_CODEC_ID_ADPCM_AFC,         MKTAG('A', 'F', 'C', ' ') },
#endif
    { AV_CODEC_ID_ADPCM_IMA_APC,     MKTAG('A', 'P', 'C', 'A') },
    { AV_CODEC_ID_ADPCM_EA,          MKTAG('A', 'D', 'E', 'A') },
    { AV_CODEC_ID_ADPCM_EA_R1,       MKTAG('E', 'A', 'R', '1') },
    { AV_CODEC_ID_ADPCM_EA_R2,       MKTAG('E', 'A', 'R', '2') },
    { AV_CODEC_ID_ADPCM_EA_R3,       MKTAG('E', 'A', 'R', '3') },
    { AV_CODEC_ID_ADPCM_EA_MAXIS_XA, MKTAG('A', 'D', 'X', 'A') },
    { AV_CODEC_ID_ADPCM_IMA_EA_EACS, MKTAG('E', 'A', 'C', 'S') },
    { AV_CODEC_ID_ADPCM_IMA_ISS,     MKTAG('A', 'I', 'S', 'S') },
    { AV_CODEC_ID_ADPCM_IMA_WS,      MKTAG('A', 'I', 'W', 'S') },
    { AV_CODEC_ID_ADPCM_THP,         MKTAG('T', 'H', 'P', 'A') },
    { AV_CODEC_ID_ADPCM_XA,          MKTAG('P', 'S', 'X', 'A') },
    { AV_CODEC_ID_AMR_NB,            MKTAG('n', 'b',   0,   0) },
    { AV_CODEC_ID_ATRAC1,            MKTAG('A', 'T', 'R', '1') },
    { AV_CODEC_ID_BINKAUDIO_DCT,     MKTAG('B', 'A', 'U', '1') },
    { AV_CODEC_ID_BINKAUDIO_RDFT,    MKTAG('B', 'A', 'U', '2') },
    { AV_CODEC_ID_BMV_AUDIO,         MKTAG('B', 'M', 'V', 'A') },
    { AV_CODEC_ID_COOK,              MKTAG('c', 'o', 'o', 'k') },
    { AV_CODEC_ID_DSICINAUDIO,       MKTAG('D', 'C', 'I', 'A') },
    { AV_CODEC_ID_EAC3,              MKTAG('E', 'A', 'C', '3') },
#if LIBAVUTIL_VERSION_MICRO >= 100
    { AV_CODEC_ID_FFWAVESYNTH,       MKTAG('F', 'F', 'W', 'S') },
#endif
    { AV_CODEC_ID_G723_1,            MKTAG('7', '2', '3', '1') },
    { AV_CODEC_ID_INTERPLAY_DPCM,    MKTAG('I', 'N', 'P', 'A') },
    { AV_CODEC_ID_ILBC,              MKTAG('i', 'l', 'b', 'c') },
    { AV_CODEC_ID_MLP,               MKTAG('M', 'L', 'P', ' ') },
    { AV_CODEC_ID_MP1,               0x50 },
    { AV_CODEC_ID_MP4ALS,            MKTAG('A', 'L', 'S', ' ') },
    { AV_CODEC_ID_MUSEPACK7,         MKTAG('M', 'P', 'C', ' ') },
    { AV_CODEC_ID_MUSEPACK8,         MKTAG('M', 'P', 'C', '8') },
    { AV_CODEC_ID_NELLYMOSER,        MKTAG('n', 'm', 'o', 's') },
    { AV_CODEC_ID_OPUS,              MKTAG('o', 'p', 'u', 's') },
    { AV_CODEC_ID_PCM_LXF,           MKTAG('P', 'L', 'X', 'F') },
    { AV_CODEC_ID_PCM_S16LE_PLANAR,  MKTAG('1', '6', 'P', 'L') },
    { AV_CODEC_ID_QCELP,             MKTAG('Q', 'c', 'l', 'p') },
    { AV_CODEC_ID_QDM2,              MKTAG('Q', 'D', 'M', '2') },
    { AV_CODEC_ID_RA_144,            MKTAG('1', '4', '_', '4') },
    { AV_CODEC_ID_RA_288,            MKTAG('2', '8', '_', '8') },
    { AV_CODEC_ID_ROQ_DPCM,          MKTAG('R', 'o', 'Q', 'A') },
    { AV_CODEC_ID_SHORTEN,           MKTAG('s', 'h', 'r', 'n') },
    { AV_CODEC_ID_SPEEX,             MKTAG('s', 'p', 'x', ' ') },
    { AV_CODEC_ID_TAK,               MKTAG('T', 'A', 'K', ' ') },
    { AV_CODEC_ID_TTA,               MKTAG('T', 'T', 'A', '1') },
    { AV_CODEC_ID_TWINVQ,            MKTAG('T', 'W', 'I', '2') },
#if LIBAVUTIL_VERSION_MICRO >= 100
    { AV_CODEC_ID_VIMA,              MKTAG('V', 'I', 'M', 'A') },
#endif
    { AV_CODEC_ID_VMDAUDIO,          MKTAG('V', 'M', 'D', 'A') },
    { AV_CODEC_ID_WAVPACK,           MKTAG('W', 'V', 'P', 'K') },
    { AV_CODEC_ID_WESTWOOD_SND1,     MKTAG('S', 'N', 'D', '1') },
    { AV_CODEC_ID_XAN_DPCM,          MKTAG('A', 'x', 'a', 'n') },
    { 0, 0 },
};

static const struct AVCodecTag mp_codecid_override_tags[] = {
    { AV_CODEC_ID_8SVX_EXP,          MKTAG('8', 'e', 'x', 'p') },
    { AV_CODEC_ID_8SVX_FIB,          MKTAG('8', 'f', 'i', 'b') },
    { MKBETAG('8','S','V','X'),      MKTAG('8', 'r', 'a', 'w') },
    { AV_CODEC_ID_AAC,               MKTAG('M', 'P', '4', 'A') },
    { AV_CODEC_ID_AAC_LATM,          MKTAG('M', 'P', '4', 'L') },
    { AV_CODEC_ID_AC3,               0x2000 },
    { AV_CODEC_ID_ADPCM_IMA_EA_SEAD, MKTAG('S', 'E', 'A', 'D') },
    { AV_CODEC_ID_ADPCM_IMA_AMV,     MKTAG('A', 'M', 'V', 'A') },
    { AV_CODEC_ID_DTS,               0x2001 },
    { AV_CODEC_ID_DVVIDEO,           MKTAG('d', 'v', 's', 'd') },
    { AV_CODEC_ID_EAC3,              MKTAG('E', 'A', 'C', '3') },
    { AV_CODEC_ID_ESCAPE124,         MKTAG('E', '1', '2', '4') },
#if LIBAVUTIL_VERSION_MICRO >= 100
    { AV_CODEC_ID_ESCAPE130,         MKTAG('E', '1', '3', '0') },
#endif
    { AV_CODEC_ID_FLV1,              MKTAG('F', 'L', 'V', '1') },
    { AV_CODEC_ID_G729,              MKTAG('G', '7', '2', '9') },
    { AV_CODEC_ID_H264,              MKTAG('H', '2', '6', '4') },
    { AV_CODEC_ID_MP3,               0x55 },
    { AV_CODEC_ID_MPEG4,             MKTAG('M', 'P', '4', 'V') },
#if LIBAVUTIL_VERSION_MICRO >= 100
    { AV_CODEC_ID_PAF_AUDIO,         MKTAG('P', 'A', 'F', 'A') },
    { AV_CODEC_ID_PAF_VIDEO,         MKTAG('P', 'A', 'F', 'V') },
#endif
    { AV_CODEC_ID_PCM_BLURAY,        MKTAG('B', 'P', 'C', 'M') },
    { AV_CODEC_ID_PCM_S8,            MKTAG('t', 'w', 'o', 's') },
    { AV_CODEC_ID_PCM_U8,            1 },
    { AV_CODEC_ID_PCM_S16BE,         MKTAG('t', 'w', 'o', 's') },
    { AV_CODEC_ID_PCM_S16LE,         1 },
    { AV_CODEC_ID_PCM_S24BE,         MKTAG('i', 'n', '2', '4') },
    { AV_CODEC_ID_PCM_S24LE,         1 },
    { AV_CODEC_ID_PCM_S32BE,         MKTAG('i', 'n', '3', '2') },
    { AV_CODEC_ID_PCM_S32LE,         1 },
    { AV_CODEC_ID_MP2,               0x50 },
    { AV_CODEC_ID_MPEG2VIDEO,        MKTAG('M', 'P', 'G', '2') },
    { AV_CODEC_ID_TRUEHD,            MKTAG('T', 'R', 'H', 'D') },
    { 0, 0 },
};

static const struct AVCodecTag * const mp_codecid_override_taglists[] =
                        {mp_codecid_override_tags, 0};

static const struct AVCodecTag mp_bmp_tags[] = {
    { AV_CODEC_ID_AMV,               MKTAG('A', 'M', 'V', 'V') },
    { AV_CODEC_ID_ANM,               MKTAG('A', 'N', 'M', ' ') },
    { AV_CODEC_ID_ANSI,              MKTAG('T', 'X', 'T', '4') },
    { AV_CODEC_ID_AVS,               MKTAG('A', 'V', 'S', ' ') },
    { AV_CODEC_ID_BETHSOFTVID,       MKTAG('B', 'E', 'T', 'H') },
    { AV_CODEC_ID_BFI,               MKTAG('B', 'F', 'I', 'V') },
    { AV_CODEC_ID_BMP,               MKTAG('b', 'm', 'p', ' ') },
    { AV_CODEC_ID_BMV_VIDEO,         MKTAG('B', 'M', 'V', 'V') },
    { AV_CODEC_ID_C93,               MKTAG('C', '9', '3', 'V') },
    { AV_CODEC_ID_CDGRAPHICS,        MKTAG('C', 'D', 'G', 'R') },
    { AV_CODEC_ID_CDXL,              MKTAG('C', 'D', 'X', 'L') },
    { AV_CODEC_ID_CMV,               MKTAG('M', 'V', 'I', 'f') },
    { AV_CODEC_ID_DFA,               MKTAG('C', 'D', 'F', 'A') },
    { AV_CODEC_ID_DNXHD,             MKTAG('A', 'V', 'd', 'n') },
    { AV_CODEC_ID_DSICINVIDEO,       MKTAG('D', 'C', 'I', 'V') },
    { AV_CODEC_ID_DXA,               MKTAG('D', 'X', 'A', '1') },
    { AV_CODEC_ID_FLIC,              MKTAG('f', 'l', 'i', 'c') },
    { AV_CODEC_ID_IDCIN,             MKTAG('I', 'D', 'C', 'I') },
    { AV_CODEC_ID_INTERPLAY_VIDEO,   MKTAG('I', 'N', 'P', 'V') },
    { AV_CODEC_ID_JV,                MKTAG('F', 'F', 'J', 'V') },
    { AV_CODEC_ID_MAD,               MKTAG('M', 'A', 'D', 'k') },
    { AV_CODEC_ID_MDEC,              MKTAG('M', 'D', 'E', 'C') },
    { AV_CODEC_ID_MMVIDEO,           MKTAG('M', 'M', 'V', ' ') },
    { AV_CODEC_ID_MOTIONPIXELS,      MKTAG('M', 'V', 'I', '1') },
#if LIBAVUTIL_VERSION_MICRO >= 100
    { AV_CODEC_ID_MVC1,              MKTAG('m', 'v', 'c', '1') },
    { AV_CODEC_ID_MVC2,              MKTAG('m', 'v', 'c', '2') },
#endif
    { AV_CODEC_ID_MXPEG,             MKTAG('M', 'X', 'P', 'G') },
    { AV_CODEC_ID_NUV,               MKTAG('N', 'U', 'V', '1') },
    { AV_CODEC_ID_RL2,               MKTAG('R', 'L', '2', 'V') },
    { AV_CODEC_ID_ROQ,               MKTAG('R', 'o', 'Q', 'V') },
    { AV_CODEC_ID_RV10,              MKTAG('R', 'V', '1', '0') },
    { AV_CODEC_ID_RV20,              MKTAG('R', 'V', '2', '0') },
    { AV_CODEC_ID_RV30,              MKTAG('R', 'V', '3', '0') },
    { AV_CODEC_ID_RV40,              MKTAG('R', 'V', '4', '0') },
#if LIBAVUTIL_VERSION_MICRO >= 100
    { AV_CODEC_ID_SANM,              MKTAG('S', 'A', 'N', 'M') },
    { AV_CODEC_ID_SGIRLE,            MKTAG('r', 'l', 'e', '1') },
#endif
    { AV_CODEC_ID_SVQ3,              MKTAG('S', 'V', 'Q', '3') },
    { AV_CODEC_ID_TMV,               MKTAG('t', 'm', 'v', '8') },
    { AV_CODEC_ID_TGV,               MKTAG('f', 'V', 'G', 'T') },
    { AV_CODEC_ID_THP,               MKTAG('T', 'H', 'P', 'V') },
    { AV_CODEC_ID_TIERTEXSEQVIDEO,   MKTAG('T', 'S', 'E', 'Q') },
    { AV_CODEC_ID_TXD,               MKTAG('T', 'X', 'D', 'V') },
    { AV_CODEC_ID_VP6A,              MKTAG('V', 'P', '6', 'A') },
    { AV_CODEC_ID_VMDVIDEO,          MKTAG('V', 'M', 'D', 'V') },
    { AV_CODEC_ID_WS_VQA,            MKTAG('V', 'Q', 'A', 'V') },
    { AV_CODEC_ID_XAN_WC3,           MKTAG('W', 'C', '3', 'V') },
    { AV_CODEC_ID_YOP,               MKTAG('Y', 'O', 'P', '1') },
    { 0, 0 },
};

static void get_taglists(const struct AVCodecTag *dst[3], int audio)
{
    dst[0] = audio ? mp_wav_tags : mp_bmp_tags;
    dst[1] = audio ? avformat_get_riff_audio_tags() : avformat_get_riff_video_tags();
    dst[2] = NULL;
}

enum CodecID mp_tag2codec_id(uint32_t tag, int audio)
{
    const struct AVCodecTag *taglists[3];
    get_taglists(taglists, audio);
    return av_codec_get_id(taglists, tag);
}

uint32_t mp_codec_id2tag(enum CodecID codec_id, uint32_t old_tag, int audio)
{
    const struct AVCodecTag *taglists[3];
    // For some formats (like PCM) always trust AV_CODEC_ID_* more than codec_tag
    uint32_t tag = av_codec_get_tag(mp_codecid_override_taglists, codec_id);
    if (tag)
        return tag;

    // mp4a tag is used for all mp4 files no matter what they actually contain
    // mp4v is sometimes also used for files containing e.g. mjpeg
    if (audio  && old_tag != MKTAG('m', 'p', '4', 'a') ||
        !audio && old_tag != MKTAG('m', 'p', '4', 'v'))
        tag = old_tag;
    if (tag)
        return tag;

    get_taglists(taglists, audio);
    return av_codec_get_tag(taglists, codec_id);
}
