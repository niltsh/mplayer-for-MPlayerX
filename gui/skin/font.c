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

/**
 * @file
 * @brief Font file parser and font rendering
 */

#include <gtk/gtk.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"
#include "skin.h"
#include "gui/util/cut.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"

#include "libavutil/avstring.h"
#include "mp_msg.h"

#define MAX_FONTS 25

#define fntAlignLeft   0
#define fntAlignCenter 1
#define fntAlignRight  2

static bmpFont *Fonts[MAX_FONTS];

/**
 * @brief Add a font to #Fonts.
 *
 * @param name name of the font
 *
 * @return an identification >= 0 (ok), -1 (out of memory) or -2 (#MAX_FONTS exceeded)
 */
static int fntAddNewFont(char *name)
{
    int id, i;

    for (id = 0; id < MAX_FONTS; id++)
        if (!Fonts[id])
            break;

    if (id == MAX_FONTS)
        return -2;

    Fonts[id] = calloc(1, sizeof(*Fonts[id]));

    if (!Fonts[id])
        return -1;

    av_strlcpy(Fonts[id]->name, name, MAX_FONT_NAME);

    for (i = 0; i < ASCII_CHRS + EXTRA_CHRS; i++) {
        Fonts[id]->Fnt[i].x  = -1;
        Fonts[id]->Fnt[i].y  = -1;
        Fonts[id]->Fnt[i].sx = -1;
        Fonts[id]->Fnt[i].sy = -1;
    }

    return id;
}

/**
 * @brief Free all memory allocated to fonts.
 */
void fntFreeFont(void)
{
    int i;

    for (i = 0; i < MAX_FONTS; i++) {
        if (Fonts[i]) {
            bpFree(&Fonts[i]->Bitmap);
            nfree(Fonts[i]);
        }
    }
}

/**
 * @brief Read and parse a font file.
 *
 * @param path directory the font file is in
 * @param fname name of the font
 *
 * @return 0 (ok), -1 or -2 (return code of #fntAddNewFont()),
 *                 -3 (file error) or -4 (#skinImageRead() error)
 */
int fntRead(char *path, char *fname)
{
    FILE *file;
    unsigned char buf[512];
    unsigned char item[32];
    unsigned char param[256];
    int id, n, i;

    id = fntAddNewFont(fname);

    if (id < 0)
        return id;

    av_strlcpy(buf, path, sizeof(buf));
    av_strlcat(buf, fname, sizeof(buf));
    av_strlcat(buf, ".fnt", sizeof(buf));
    file = fopen(buf, "rt");

    if (!file) {
        nfree(Fonts[id]);
        return -3;
    }

    while (fgetstr(buf, sizeof(buf), file)) {
        strswap(buf, '\t', ' ');
        trim(buf);
        decomment(buf);

        if (!*buf)
            continue;

        n = (strncmp(buf, "\"=", 2) == 0 ? 1 : 0);
        cutItem(buf, item, '=', n);
        cutItem(buf, param, '=', n + 1);

        if (item[0] == '"') {
            if (!item[1])
                item[0] = '=';
            else if (item[1] == '"')
                item[1] = 0;
            else
                cutItem(item, item, '"', 1);

            if (item[0] & 0x80) {
                for (i = 0; i < EXTRA_CHRS; i++) {
                    if (!Fonts[id]->nonASCIIidx[i][0]) {
                        strncpy(Fonts[id]->nonASCIIidx[i], item, UTF8LENGTH);
                        break;
                    }
                }

                if (i == EXTRA_CHRS)
                    continue;

                i += ASCII_CHRS;
            } else
                i = item[0];

            cutItem(param, buf, ',', 0);
            Fonts[id]->Fnt[i].x = atoi(buf);

            cutItem(param, buf, ',', 1);
            Fonts[id]->Fnt[i].y = atoi(buf);

            cutItem(param, buf, ',', 2);
            Fonts[id]->Fnt[i].sx = atoi(buf);

            cutItem(param, buf, ',', 3);
            Fonts[id]->Fnt[i].sy = atoi(buf);

            mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[font]  char: '%s' params: %d,%d %dx%d\n", item, Fonts[id]->Fnt[i].x, Fonts[id]->Fnt[i].y, Fonts[id]->Fnt[i].sx, Fonts[id]->Fnt[i].sy);
        } else if (!strcmp(item, "image")) {
            av_strlcpy(buf, path, sizeof(buf));
            av_strlcat(buf, param, sizeof(buf));

            mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[font] image file: %s\n", buf);

            if (skinImageRead(buf, &Fonts[id]->Bitmap) != 0) {
                bpFree(&Fonts[id]->Bitmap);
                nfree(Fonts[id]);
                fclose(file);
                return -4;
            }
        }
    }

    fclose(file);

    return 0;
}

/**
 * @brief Find the ID of a font by its name.
 *
 * @param name name of the font
 *
 * @return an identification >= 0 (ok) or -1 (not found)
 */
int fntFindID(char *name)
{
    int i;

    for (i = 0; i < MAX_FONTS; i++)
        if (Fonts[i])
            if (!strcmp(name, Fonts[i]->name))
                return i;

    return -1;
}

/**
 * @brief Get the #bmpFont::Fnt index of the character @a *str points to.
 *
 *        Move pointer @a *str to the character according to @a direction
 *        afterwards.
 *
 * @param id font ID
 * @param str pointer to the string
 * @param uft8 flag indicating whether @a str contains UTF-8 characters
 * @param direction +1 (forward) or -1 (backward)
 *
 * @return index >= 0 (ok) or -1 (not found)
 */
static int fntGetCharIndex(int id, unsigned char **str, gboolean utf8, int direction)
{
    unsigned char *p, uchar[6] = "";   // glib implements 31-bit UTF-8
    int i, c = -1;

    if (**str & 0x80) {
        if (utf8) {
            p    = *str;
            *str = g_utf8_next_char(*str);
            strncpy(uchar, p, *str - p);

            if (direction < 0)
                *str = g_utf8_prev_char(p);
        } else {
            uchar[0] = **str;
            *str    += direction;
        }

        for (i = 0; (i < EXTRA_CHRS) && Fonts[id]->nonASCIIidx[i][0]; i++) {
            if (strncmp(Fonts[id]->nonASCIIidx[i], uchar, UTF8LENGTH) == 0)
                return i + ASCII_CHRS;

            if (!utf8 &&
                (Fonts[id]->nonASCIIidx[i][0] == (*uchar >> 6 | 0xc0) &&
                 Fonts[id]->nonASCIIidx[i][1] == ((*uchar & 0x3f) | 0x80) &&
                 Fonts[id]->nonASCIIidx[i][2] == 0))
                c = i + ASCII_CHRS;
        }
    } else {
        c = **str;

        if (utf8 && (direction < 0))
            *str = g_utf8_prev_char(*str);
        else
            *str += direction;
    }

    return c;
}

/**
 * @brief Get the rendering width of a text.
 *
 * @param id font ID
 * @param str string to be examined
 *
 * @return width of the rendered string (in pixels)
 */
int fntTextWidth(int id, char *str)
{
    int size = 0, c;
    gboolean utf8;
    unsigned char *p;

    utf8 = g_utf8_validate(str, -1, NULL);
    p    = str;

    while (*p) {
        c = fntGetCharIndex(id, &p, utf8, 1);

        if (c == -1 || Fonts[id]->Fnt[c].sx == -1)
            c = ' ';

        if (Fonts[id]->Fnt[c].sx != -1)
            size += Fonts[id]->Fnt[c].sx;
    }

    return size;
}

/**
 * @brief Get the rendering height of a text.
 *
 * @param id font ID
 * @param str string to be examined
 *
 * @return height of the rendered string (in pixels)
 */
static int fntTextHeight(int id, char *str)
{
    int max = 0, c, h;
    gboolean utf8;
    unsigned char *p;

    utf8 = g_utf8_validate(str, -1, NULL);
    p    = str;

    while (*p) {
        c = fntGetCharIndex(id, &p, utf8, 1);

        if (c == -1 || Fonts[id]->Fnt[c].sx == -1)
            c = ' ';

        h = Fonts[id]->Fnt[c].sy;

        if (h > max)
            max = h;
    }

    return max;
}

/**
 * @brief Render a text on an item.
 *
 * @param item item the text shall be placed on
 * @param px x position for the text in case it is wider than the item width
 * @param txt text to be rendered
 *
 * @return image containing the rendered text
 */
guiImage *fntTextRender(guiItem *item, int px, char *txt)
{
    unsigned char *u;
    unsigned int i;
    int c, dx, tw, th, fbw, iw, id, ofs;
    int x, y, fh, fw, fyc, yc;
    uint32_t *ibuf;
    uint32_t *obuf;
    gboolean utf8;

    id = item->fontid;
    tw = fntTextWidth(id, txt);

    if (!tw)
        return NULL;

    iw  = item->width;
    fbw = Fonts[id]->Bitmap.Width;
    th  = fntTextHeight(id, txt);

    if (item->height != th)
        bpFree(&item->Bitmap);

    if (!item->Bitmap.Image) {
        item->Bitmap.Height    = item->height = th;
        item->Bitmap.Width     = item->width = iw;
        item->Bitmap.ImageSize = item->height * iw * 4;

        if (!item->Bitmap.ImageSize)
            return NULL;

        item->Bitmap.Bpp   = 32;
        item->Bitmap.Image = malloc(item->Bitmap.ImageSize);

        if (!item->Bitmap.Image)
            return NULL;
    }

    obuf = (uint32_t *)item->Bitmap.Image;
    ibuf = (uint32_t *)Fonts[id]->Bitmap.Image;

    for (i = 0; i < item->Bitmap.ImageSize / 4; i++)
        obuf[i] = GUI_TRANSPARENT;

    if (tw <= iw) {
        switch (item->align) {
        default:
        case fntAlignLeft:
            dx = 0;
            break;

        case fntAlignCenter:
            dx = (iw - tw) / 2;
            break;

        case fntAlignRight:
            dx = iw - tw;
            break;
        }
    } else
        dx = px;

    ofs = dx;

    utf8 = g_utf8_validate(txt, -1, NULL);
    u    = txt;

    while (*u) {
        c = fntGetCharIndex(id, &u, utf8, 1);

        if (c != -1)
            fw = Fonts[id]->Fnt[c].sx;

        if (c == -1 || fw == -1) {
            c  = ' ';
            fw = Fonts[id]->Fnt[c].sx;
        }

        if (fw == -1)
            continue;

        fh  = Fonts[id]->Fnt[c].sy;
        fyc = Fonts[id]->Fnt[c].y * fbw + Fonts[id]->Fnt[c].x;
        yc  = dx;

        if (dx >= 0) {
            for (y = 0; y < fh; y++) {
                for (x = 0; x < fw; x++)
                    if (dx + x >= 0 && dx + x < iw)
                        obuf[yc + x] = ibuf[fyc + x];

                fyc += fbw;
                yc  += iw;
            }
        }

        dx += fw;
    }

    if (ofs > 0 && tw > item->width) {
        dx = ofs;
        u  = txt + strlen(txt);

        while (u > (unsigned char *)txt) {
            c = fntGetCharIndex(id, &u, utf8, -1);

            if (c != -1)
                fw = Fonts[id]->Fnt[c].sx;

            if (c == -1 || fw == -1) {
                c  = ' ';
                fw = Fonts[id]->Fnt[c].sx;
            }

            if (fw == -1)
                continue;

            fh  = Fonts[id]->Fnt[c].sy;
            fyc = Fonts[id]->Fnt[c].y * fbw + Fonts[id]->Fnt[c].x;

            dx -= fw;
            yc  = dx;

            if (dx >= 0) {
                for (y = 0; y < fh; y++) {
                    for (x = fw - 1; x >= 0; x--)
                        if (dx + x >= 0 && dx + x < iw)
                            obuf[yc + x] = ibuf[fyc + x];

                    fyc += fbw;
                    yc  += iw;
                }
            }
        }
    }

    return &item->Bitmap;
}
