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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"
#include "gui/interface.h"
#include "gui/skin/font.h"
#include "gui/util/string.h"

#include "access_mpcontext.h"
#include "libavutil/avstring.h"
#include "mixer.h"
#include "osdep/timer.h"
#include "stream/stream.h"

#define DLABEL_DELAY 2500   // in milliseconds

static char *image_buffer;
static int image_width;

static char *Translate(char *str)
{
    static char trbuf[512];
    char tmp[512];
    unsigned int i, c;
    int t;
    mixer_t *mixer = NULL;

    *trbuf = 0;

    for (c = 0, i = 0; i < strlen(str); i++) {
        if (str[i] != '$') {
            if (c + 1 < sizeof(trbuf)) {
                trbuf[c++] = str[i];
                trbuf[c]   = 0;
            }
        } else {
            switch (str[++i]) {
            case 't':
                snprintf(tmp, sizeof(tmp), "%02d", guiInfo.Track);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'o':
                TranslateFilename(0, tmp, sizeof(tmp));
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'f':
                TranslateFilename(1, tmp, sizeof(tmp));
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'F':
                TranslateFilename(2, tmp, sizeof(tmp));
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case '6':
                t = guiInfo.RunningTime;
                goto calclengthhhmmss;

            case '1':
                t = guiInfo.ElapsedTime;
calclengthhhmmss:
                snprintf(tmp, sizeof(tmp), "%02d:%02d:%02d", t / 3600, t / 60 % 60, t % 60);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case '7':
                t = guiInfo.RunningTime;
                goto calclengthmmmmss;

            case '2':
                t = guiInfo.ElapsedTime;
calclengthmmmmss:
                snprintf(tmp, sizeof(tmp), "%04d:%02d", t / 60, t % 60);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case '3':
                snprintf(tmp, sizeof(tmp), "%02d", guiInfo.ElapsedTime / 3600);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case '4':
                snprintf(tmp, sizeof(tmp), "%02d", (guiInfo.ElapsedTime / 60) % 60);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case '5':
                snprintf(tmp, sizeof(tmp), "%02d", guiInfo.ElapsedTime % 60);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case '8':
                snprintf(tmp, sizeof(tmp), "%01d:%02d:%02d", guiInfo.ElapsedTime / 3600, (guiInfo.ElapsedTime / 60) % 60, guiInfo.ElapsedTime % 60);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'v':
                snprintf(tmp, sizeof(tmp), "%3.2f%%", guiInfo.Volume);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'V':
                snprintf(tmp, sizeof(tmp), "%3.1f", guiInfo.Volume);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'b':
                snprintf(tmp, sizeof(tmp), "%3.2f%%", guiInfo.Balance);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'B':
                snprintf(tmp, sizeof(tmp), "%3.1f", guiInfo.Balance);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'x':
                snprintf(tmp, sizeof(tmp), "%d", guiInfo.VideoWidth);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'y':
                snprintf(tmp, sizeof(tmp), "%d", guiInfo.VideoHeight);
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 'C':
                snprintf(tmp, sizeof(tmp), "%s", guiInfo.CodecName ? guiInfo.CodecName : "");
                av_strlcat(trbuf, tmp, sizeof(trbuf));
                break;

            case 's':
                if (guiInfo.Playing == GUI_STOP)
                    av_strlcat(trbuf, "s", sizeof(trbuf));
                break;

            case 'l': // legacy
            case 'p':
                if (guiInfo.Playing == GUI_PLAY)
                    av_strlcat(trbuf, "p", sizeof(trbuf));
                break;

            case 'e':
                if (guiInfo.Playing == GUI_PAUSE)
                    av_strlcat(trbuf, "e", sizeof(trbuf));
                break;

            case 'a':

                if (guiInfo.mpcontext)
                    mixer = mpctx_get_mixer(guiInfo.mpcontext);

                if (mixer && mixer->muted) {
                    av_strlcat(trbuf, "n", sizeof(trbuf));
                    break;
                }

                switch (guiInfo.AudioChannels) {
                case 0:
                    av_strlcat(trbuf, "n", sizeof(trbuf));
                    break;

                case 1:
                    av_strlcat(trbuf, "m", sizeof(trbuf));
                    break;

                case 2:
                    av_strlcat(trbuf, "t", sizeof(trbuf));
                    break;
                }

                break;

            case 'T':
                switch (guiInfo.StreamType) {
                case STREAMTYPE_FILE:
                    av_strlcat(trbuf, "f", sizeof(trbuf));
                    break;

                case STREAMTYPE_STREAM:
                    av_strlcat(trbuf, "u", sizeof(trbuf));
                    break;

                case STREAMTYPE_CDDA:
                    av_strlcat(trbuf, "a", sizeof(trbuf));
                    break;

                case STREAMTYPE_VCD:
                    av_strlcat(trbuf, "v", sizeof(trbuf));
                    break;

                case STREAMTYPE_DVD:
                    av_strlcat(trbuf, "d", sizeof(trbuf));
                    break;

                default:
                    av_strlcat(trbuf, " ", sizeof(trbuf));
                    break;
                }
                break;

            case '$':
                av_strlcat(trbuf, "$", sizeof(trbuf));
                break;

            default:
                continue;
            }

            c = strlen(trbuf);
        }
    }

    return trbuf;
}

static void PutImage(guiImage *bf, int x, int y, int max, int ofs)
{
    int i = 0, ix, iy;
    uint32_t *buf = NULL;
    uint32_t *drw = NULL;
    register uint32_t tmp;

    /* register uint32_t yc; */

    if (!bf || (bf->Image == NULL))
        return;

    i   = bf->Width * (bf->Height / max) * ofs;
    buf = (uint32_t *)image_buffer;
    drw = (uint32_t *)bf->Image;

#if 1
    for (iy = y; iy < (int)(y + bf->Height / max); iy++)
        for (ix = x; ix < (int)(x + bf->Width); ix++) {
            tmp = drw[i++];

            if (!IS_TRANSPARENT(tmp))
                buf[iy * image_width + ix] = tmp;
        }
#else
    yc = y * image_width;

    for (iy = y; iy < (int)(y + bf->Height / max); iy++) {
        for (ix = x; ix < (int)(x + bf->Width); ix++) {
            tmp = drw[i++];

            if (!IS_TRANSPARENT(tmp))
                buf[yc + ix] = tmp;
        }

        yc += image_width;
    }
#endif
}

static void SimplePotmeterPutImage(guiImage *bf, int x, int y, float frac)
{
    int i = 0, w, r, ix, iy;
    uint32_t *buf = NULL;
    uint32_t *drw = NULL;
    register uint32_t tmp;

    if (!bf || (bf->Image == NULL))
        return;

    buf = (uint32_t *)image_buffer;
    drw = (uint32_t *)bf->Image;
    w   = bf->Width * frac;
    r   = bf->Width - w;

    for (iy = y; iy < (int)(y + bf->Height); iy++) {
        for (ix = x; ix < (int)(x + w); ix++) {
            tmp = drw[i++];

            if (!IS_TRANSPARENT(tmp))
                buf[iy * image_width + ix] = tmp;
        }

        i += r;
    }
}

void RenderAll(wsWindow *window, guiItem *Items, int nrItems, char *db)
{
    guiItem *item;
    guiImage *image = NULL;
    int i, ofs;

    image_buffer = db;
    image_width  = window->Width;

    for (i = 0; i < nrItems + 1; i++) {
        item = &Items[i];

        switch (item->pressed) {
        case btnPressed:
            ofs = 0;
            break;

        case btnReleased:
            ofs = 1;
            break;

        default:
            ofs = 2;
            break;
        }

        switch (item->type) {
        case itButton:

            PutImage(&item->Bitmap, item->x, item->y, 3, ofs);
            break;

        case itPotmeter:

            if (item->numphases == 1)
                SimplePotmeterPutImage(&item->Bitmap, item->x, item->y, item->value / 100.0);
            else
                PutImage(&item->Bitmap, item->x, item->y, item->numphases, (item->numphases - 1) * (item->value / 100.0));

            break;

        case itHPotmeter:

            if (item->numphases == 1)
                SimplePotmeterPutImage(&item->Bitmap, item->x, item->y, item->value / 100.0);
            else
                PutImage(&item->Bitmap, item->x, item->y, item->numphases, (item->numphases - 1) * (item->value / 100.0));

            PutImage(&item->Mask, item->x + (item->width - item->pwidth) * (item->value / 100.0), item->y, 3, ofs);
            break;

        case itVPotmeter:

            PutImage(&item->Bitmap, item->x, item->y, item->numphases, item->numphases * (1.0 - item->value / 100.0));
            PutImage(&item->Mask, item->x, item->y + (item->height - item->pheight) * (1.0 - item->value / 100.0), 3, ofs);
            break;

        case itSLabel:

            if (item->width == -1)
                item->width = fntTextWidth(item->fontid, item->label);

            image = fntTextRender(item, 0, item->label);

            if (image)
                PutImage(image, item->x, item->y, 1, 0);

            break;

        case itDLabel:
        {
            int x;
            unsigned int d;
            char *t = Translate(item->label);

            if (!item->text || (strcmp(item->text, t) != 0)) {
                free(item->text);
                item->text      = strdup(t);
                item->textwidth = fntTextWidth(item->fontid, t);
                item->starttime = GetTimerMS();
                item->last_x    = 0;
            }

            d = GetTimerMS() - item->starttime;

            if (d < DLABEL_DELAY)
                x = item->last_x;                     // don't scroll yet
            else {
                int l;
                char c[2];

                l    = (item->textwidth ? item->textwidth : item->width);
                x    = (l ? l - ((d - DLABEL_DELAY) / 20) % l - 1 : 0);
                c[0] = *item->text;
                c[1] = '\0';

                if (x < (fntTextWidth(item->fontid, c) + 1) >> 1) {
                    item->starttime = GetTimerMS();   // stop again
                    item->last_x    = x;              // at current x pos
                }
            }

            image = fntTextRender(item, x, t);
        }

            if (image)
                PutImage(image, item->x, item->y, 1, 0);

            break;
        }
    }

    wsImageRender(window, db);
}
