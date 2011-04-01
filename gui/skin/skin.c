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
#include <string.h>

#include "skin.h"
#include "font.h"
#include "gui/app.h"
#include "gui/interface.h"
#include "gui/mplayer/widgets.h"
#include "gui/util/cut.h"
#include "gui/util/string.h"

#include "config.h"
#include "help_mp.h"
#include "libavutil/avstring.h"
#include "libavutil/common.h"
#include "mp_msg.h"

typedef struct {
    const char *name;
    int (*func)(char *in);
} _item;

static guiItems *skin;

static int linenumber;
static unsigned char path[512];

static unsigned char currWinName[32];
static wItem *currWin;
static int *currWinItemIdx;
static wItem *currWinItems;

static void skin_error(const char *format, ...)
{
    char p[512];
    va_list ap;

    va_start(ap, format);
    vsnprintf(p, sizeof(p), format, ap);
    va_end(ap);

    gmp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_ERRORMESSAGE, linenumber, p);
}

static int section_cmd(char *cmd)
{
    if (!skin) {
        skin_error(MSGTR_SKIN_ERROR_SECTION, cmd);
        return 0;
    }

    return 1;
}

static int window_cmd(char *cmd)
{
    if (!currWinName[0]) {
        skin_error(MSGTR_SKIN_ERROR_WINDOW, cmd);
        return 0;
    }

    return 1;
}

static int in_window(char *name)
{
    if (strcmp(currWinName, name) == 0) {
        skin_error(MSGTR_SKIN_ERROR_ITEM, name);
        return 1;
    }

    return 0;
}

int skinBPRead(char *fname, txSample *bf)
{
    int i = bpRead(fname, bf);

    switch (i) {
    case -1:
        skin_error(MSGTR_SKIN_BITMAP_16bit, fname);
        break;

    case -2:
        skin_error(MSGTR_SKIN_BITMAP_FileNotFound, fname);
        break;

    case -5:
        skin_error(MSGTR_SKIN_BITMAP_PNGReadError, fname);
        break;

    case -8:
        skin_error(MSGTR_SKIN_BITMAP_ConversionError, fname);
        break;
    }

    return i;
}

static wItem *next_item(void)
{
    wItem *item = NULL;

    if (*currWinItemIdx < MAX_ITEMS - 1) {
        (*currWinItemIdx)++;
        item = &currWinItems[*currWinItemIdx];
    } else
        skin_error(MSGTR_SKIN_TooManyItemsDeclared);

    return item;
}

// section=movieplayer
static int cmd_section(char *in)
{
    if (skin) {
        skin_error(MSGTR_SKIN_ERROR_ITEM, "section");
        return 1;
    }

    if (!strcmp(strlower(in), "movieplayer"))
        skin = &appMPlayer;
    else {
        skin_error(MSGTR_SKIN_UNKNOWN_NAME, in);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]  section: %s\n", in);

    return 0;
}

// end
static int cmd_end(char *in)
{
#ifdef MP_DEBUG
    char *space, *name;

    if (currWinName[0]) {
        space = " ";
        name  = currWinName;
    } else {
        space = "";
        name  = "section";
    }
#endif

    (void)in;

    if (!section_cmd("end"))
        return 1;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]  %send (%s)\n", space, name);

    if (currWinName[0]) {
        currWinName[0] = 0;
        currWin = NULL;
        currWinItemIdx = NULL;
        currWinItems   = NULL;
    } else
        skin = NULL;

    return 0;
}

// window=main|sub|playbar|menu
static int cmd_window(char *in)
{
    if (!section_cmd("window"))
        return 1;

    if (currWinName[0]) {
        skin_error(MSGTR_SKIN_ERROR_ITEM, "window");
        return 1;
    }

    strlower(in);

    if (strcmp(in, "main") == 0) {
        currWin = &skin->main;
        currWinItemIdx = &skin->IndexOfMainItems;
        currWinItems   = skin->mainItems;
    } else if (strcmp(in, "sub") == 0) {
        currWin = &skin->sub;
        currWinItemIdx = NULL;
        currWinItems   = NULL;
    } else if (strcmp(in, "playbar") == 0) {
        currWin = &skin->bar;
        currWinItemIdx = &skin->IndexOfBarItems;
        currWinItems   = skin->barItems;
    } else if (strcmp(in, "menu") == 0) {
        currWin = &skin->menu;
        currWinItemIdx = &skin->IndexOfMenuItems;
        currWinItems   = skin->menuItems;
    } else {
        skin_error(MSGTR_SKIN_UNKNOWN_NAME, in);
        return 1;
    }

    av_strlcpy(currWinName, in, sizeof(currWinName));

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]   window: %s\n", currWinName);

    return 0;
}

// base=image,x,y[,width,height]
static int cmd_base(char *in)
{
    unsigned char fname[256];
    unsigned char file[512];
    int x, y;
    int w = 0, h = 0;

    if (!window_cmd("base"))
        return 1;

    cutItem(in, fname, ',', 0);
    x = cutItemToInt(in, ',', 1);
    y = cutItemToInt(in, ',', 2);
    w = cutItemToInt(in, ',', 3);
    h = cutItemToInt(in, ',', 4);

    if (!strcmp(currWinName, "main")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s %d,%d\n", fname, x, y);

        currWin->x    = x;
        currWin->y    = y;
        currWin->type = itBase;

        av_strlcpy(file, path, sizeof(file));
        av_strlcat(file, fname, sizeof(file));

        if (skinBPRead(file, &currWin->Bitmap) != 0)
            return 1;

        currWin->width  = currWin->Bitmap.Width;
        currWin->height = currWin->Bitmap.Height;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", currWin->width, currWin->height);

#ifdef CONFIG_XSHAPE
        Convert32to1(&currWin->Bitmap, &currWin->Mask, 0x00ff00ff);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     mask: %lux%lu\n", currWin->Mask.Width, currWin->Mask.Height);
#else
        currWin->Mask.Image = NULL;
#endif
    }

    if (!strcmp(currWinName, "sub")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s %d,%d\n", fname, x, y);

        currWin->type = itBase;

        av_strlcpy(file, path, sizeof(file));
        av_strlcat(file, fname, sizeof(file));

        if (skinBPRead(file, &currWin->Bitmap) != 0)
            return 1;

        currWin->x      = x;
        currWin->y      = y;
        currWin->width  = currWin->Bitmap.Width;
        currWin->height = currWin->Bitmap.Height;

        if (w && h) {
            currWin->width  = w;
            currWin->height = h;
        }

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", currWin->width, currWin->height);
    }

    if (!strcmp(currWinName, "menu")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s\n", fname);

        skin->menuIsPresent = 1;
        currWin->type       = itBase;

        av_strlcpy(file, path, sizeof(file));
        av_strlcat(file, fname, sizeof(file));

        if (skinBPRead(file, &currWin->Bitmap) != 0)
            return 1;

        currWin->width  = currWin->Bitmap.Width;
        currWin->height = currWin->Bitmap.Height;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", currWin->width, currWin->height);

#ifdef CONFIG_XSHAPE
        Convert32to1(&currWin->Bitmap, &currWin->Mask, 0x00ff00ff);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     mask: %lux%lu\n", currWin->Mask.Width, currWin->Mask.Height);
#else
        currWin->Mask.Image = NULL;
#endif
    }

    if (!strcmp(currWinName, "playbar")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s %d,%d\n", fname, x, y);

        skin->barIsPresent = 1;
        currWin->x    = x;
        currWin->y    = y;
        currWin->type = itBase;

        av_strlcpy(file, path, sizeof(file));
        av_strlcat(file, fname, sizeof(file));

        if (skinBPRead(file, &currWin->Bitmap) != 0)
            return 1;

        currWin->width  = currWin->Bitmap.Width;
        currWin->height = currWin->Bitmap.Height;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", currWin->width, currWin->height);

#ifdef CONFIG_XSHAPE
        Convert32to1(&currWin->Bitmap, &currWin->Mask, 0x00ff00ff);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     mask: %lux%lu\n", currWin->Mask.Width, currWin->Mask.Height);
#else
        currWin->Mask.Image = NULL;
#endif
    }

    return 0;
}

// background=R,G,B
static int cmd_background(char *in)
{
    if (!window_cmd("background"))
        return 1;

    if (in_window("main"))
        return 1;
    if (in_window("playbar"))
        return 1;
    if (in_window("menu"))
        return 1;

    currWin->R = cutItemToInt(in, ',', 0);
    currWin->G = cutItemToInt(in, ',', 1);
    currWin->B = cutItemToInt(in, ',', 2);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    background color: #%02x%02x%02x\n", currWin->R, currWin->G, currWin->B);

    return 0;
}

// button=image,x,y,width,height,message
static int cmd_button(char *in)
{
    unsigned char fname[256];
    unsigned char file[512];
    int x, y, w, h, message;
    char msg[32];
    wItem *item;

    if (!window_cmd("button"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(in, fname, ',', 0);
    x = cutItemToInt(in, ',', 1);
    y = cutItemToInt(in, ',', 2);
    w = cutItemToInt(in, ',', 3);
    h = cutItemToInt(in, ',', 4);
    cutItem(in, msg, ',', 5);

    message = appFindMessage(msg);

    if (message == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, msg);
        return 1;
    }

    item = next_item();

    if (!item)
        return 1;

    item->type    = itButton;
    item->x       = x;
    item->y       = y;
    item->width   = w;
    item->height  = h;
    item->message = message;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    button image: %s %d,%d\n", fname, x, y);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", msg, message);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     size: %dx%d\n", w, h);

    item->pressed = btnReleased;

    if (item->message == evPauseSwitchToPlay)
        item->pressed = btnDisabled;

    item->tmp = 1;

    item->Bitmap.Image = NULL;

    if (strcmp(fname, "NULL") != 0) {
        av_strlcpy(file, path, sizeof(file));
        av_strlcat(file, fname, sizeof(file));

        if (skinBPRead(file, &item->Bitmap) != 0)
            return 1;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    return 0;
}

// selected=image
static int cmd_selected(char *in)
{
    unsigned char file[512];
    wItem *currItem;

    if (!window_cmd("selected"))
        return 1;

    if (in_window("main"))
        return 1;
    if (in_window("sub"))
        return 1;
    if (in_window("playbar"))
        return 1;

    currItem       = &skin->menuSelected;
    currItem->type = itBase;

    av_strlcpy(file, path, sizeof(file));
    av_strlcat(file, in, sizeof(file));

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image selected: %s\n", in);

    if (skinBPRead(file, &currItem->Bitmap) != 0)
        return 1;

    currItem->width  = currItem->Bitmap.Width;
    currItem->height = currItem->Bitmap.Height;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", currItem->width, currItem->height);

    return 0;
}

// menu=x,y,width,height,message
static int cmd_menu(char *in)
{
    int x, y, w, h, message;
    char msg[32];
    wItem *item;

    if (!window_cmd("menu"))
        return 1;

    if (in_window("main"))
        return 1;
    if (in_window("sub"))
        return 1;
    if (in_window("playbar"))
        return 1;

    x = cutItemToInt(in, ',', 0);
    y = cutItemToInt(in, ',', 1);
    w = cutItemToInt(in, ',', 2);
    h = cutItemToInt(in, ',', 3);
    cutItem(in, msg, ',', 4);

    message = appFindMessage(msg);

    if (message == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, msg);
        return 1;
    }

    item = next_item();

    if (!item)
        return 1;

    item->x       = x;
    item->y       = y;
    item->width   = w;
    item->height  = h;
    item->message = message;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    item #%d: %d,%d %dx%d\n", *currWinItemIdx, x, y, w, h);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", msg, message);

    item->Bitmap.Image = NULL;

    return 0;
}

// hpotmeter=button,bwidth,bheight,phases,numphases,default,x,y,width,height,message
static int cmd_hpotmeter(char *in)
{
    unsigned char pfname[256];
    unsigned char phfname[256];
    unsigned char buf[512];
    int pwidth, pheight, ph, d, x, y, w, h, message;
    wItem *item;

    if (!window_cmd("h/v potmeter"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(in, pfname, ',', 0);
    pwidth  = cutItemToInt(in, ',', 1);
    pheight = cutItemToInt(in, ',', 2);
    cutItem(in, phfname, ',', 3);
    ph = cutItemToInt(in, ',', 4);
    d  = cutItemToInt(in, ',', 5);
    x  = cutItemToInt(in, ',', 6);
    y  = cutItemToInt(in, ',', 7);
    w  = cutItemToInt(in, ',', 8);
    h  = cutItemToInt(in, ',', 9);
    cutItem(in, buf, ',', 10);

    message = appFindMessage(buf);

    if (message == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, buf);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    h/v potmeter image: %s %d,%d %dx%d\n", phfname, x, y, w, h);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     button image: %s %dx%d\n", pfname, pwidth, pheight);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     numphases: %d, default: %d%%\n", ph, d);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", buf, message);

    item = next_item();

    if (!item)
        return 1;

    item->type         = itHPotmeter;
    item->x            = x;
    item->y            = y;
    item->width        = w;
    item->height       = h;
    item->numphases    = ph;
    item->pwidth       = pwidth;
    item->pheight      = pheight;
    item->message      = message;
    item->value        = (float)d;
    item->pressed      = btnReleased;
    item->Bitmap.Image = NULL;

    if (strcmp(phfname, "NULL") != 0) {
        av_strlcpy(buf, path, sizeof(buf));
        av_strlcat(buf, phfname, sizeof(buf));

        if (skinBPRead(buf, &item->Bitmap) != 0)
            return 1;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (potmeter bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    item->Mask.Image = NULL;

    if (strcmp(pfname, "NULL") != 0) {
        av_strlcpy(buf, path, sizeof(buf));
        av_strlcat(buf, pfname, sizeof(buf));

        if (skinBPRead(buf, &item->Mask) != 0)
            return 1;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (button bitmap: %lux%lu)\n", item->Mask.Width, item->Mask.Height);
    }

    return 0;
}

// vpotmeter=button,bwidth,bheight,phases,numphases,default,x,y,width,height,message
static int cmd_vpotmeter(char *in)
{
    int r;
    wItem *item;

    r = cmd_hpotmeter(in);

    if (r == 0) {
        item       = &currWinItems[*currWinItemIdx];
        item->type = itVPotmeter;
    }

    return r;
}

// potmeter=phases,numphases,default,x,y,width,height,message
static int cmd_potmeter(char *in)
{
    unsigned char phfname[256];
    unsigned char buf[512];
    int ph, d, x, y, w, h, message;
    wItem *item;

    if (!window_cmd("potmeter"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(in, phfname, ',', 0);
    ph = cutItemToInt(in, ',', 1);
    d  = cutItemToInt(in, ',', 2);
    x  = cutItemToInt(in, ',', 3);
    y  = cutItemToInt(in, ',', 4);
    w  = cutItemToInt(in, ',', 5);
    h  = cutItemToInt(in, ',', 6);
    cutItem(in, buf, ',', 7);

    message = appFindMessage(buf);

    if (message == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, buf);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    potmeter image: %s %d,%d %dx%d\n", phfname, x, y, w, h);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     numphases: %d, default: %d%%\n", ph, d);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", buf, message);

    item = next_item();

    if (!item)
        return 1;

    item->type         = itPotmeter;
    item->x            = x;
    item->y            = y;
    item->width        = w;
    item->height       = h;
    item->numphases    = ph;
    item->message      = message;
    item->value        = (float)d;
    item->Bitmap.Image = NULL;

    if (strcmp(phfname, "NULL") != 0) {
        av_strlcpy(buf, path, sizeof(buf));
        av_strlcat(buf, phfname, sizeof(buf));

        if (skinBPRead(buf, &item->Bitmap) != 0)
            return 1;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    return 0;
}

// font=fontfile
static int cmd_font(char *in)
{
    char fnt[256];
    wItem *item;

    if (!window_cmd("font"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(in, fnt, ',', 0);   // Note: This seems needless but isn't for compatibility
                                // reasons with a meanwhile depreciated second parameter.
    item = next_item();

    if (!item)
        return 1;

    item->type   = itFont;
    item->fontid = fntRead(path, fnt);

    switch (item->fontid) {
    case -1:
        skin_error(MSGTR_SKIN_NotEnoughMemory);
        return 1;

    case -2:
        skin_error(MSGTR_SKIN_FONT_TooManyFontsDeclared);
        return 1;

    case -3:
        skin_error(MSGTR_SKIN_FONT_FontFileNotFound);
        return 1;

    case -4:
        skin_error(MSGTR_SKIN_FONT_FontImageNotFound);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    font: %s (#%d)\n", fnt, fntFindID(fnt));

    return 0;
}

// slabel=x,y,fontfile,"text"
static int cmd_slabel(char *in)
{
    int x, y, id;
    char fnt[256];
    char txt[256];
    wItem *item;

    if (!window_cmd("slabel"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    x = cutItemToInt(in, ',', 0);
    y = cutItemToInt(in, ',', 1);
    cutItem(in, fnt, ',', 2);
    cutItem(in, txt, ',', 3);
    cutItem(txt, txt, '"', 1);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    slabel: \"%s\"\n", txt);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     pos: %d,%d\n", x, y);

    id = fntFindID(fnt);

    if (id < 0) {
        skin_error(MSGTR_SKIN_FONT_NonExistentFont, fnt);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     font: %s (#%d)\n", fnt, id);

    item = next_item();

    if (!item)
        return 1;

    item->type   = itSLabel;
    item->fontid = id;
    item->x      = x;
    item->y      = y;
    item->width  = -1;
    item->height = -1;
    item->label  = strdup(txt);

    if (!item->label) {
        skin_error(MSGTR_SKIN_NotEnoughMemory);
        return 1;
    }

    return 0;
}

// dlabel=x,y,width,align,fontfile,"text"
static int cmd_dlabel(char *in)
{
    int x, y, w, a, id;
    char fnt[256];
    char txt[256];
    wItem *item;

    if (!window_cmd("dlabel"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    x = cutItemToInt(in, ',', 0);
    y = cutItemToInt(in, ',', 1);
    w = cutItemToInt(in, ',', 2);
    a = cutItemToInt(in, ',', 3);
    cutItem(in, fnt, ',', 4);
    cutItem(in, txt, ',', 5);
    cutItem(txt, txt, '"', 1);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    dlabel: \"%s\"\n", txt);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     pos: %d,%d\n", x, y);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     width: %d, align: %d\n", w, a);

    id = fntFindID(fnt);

    if (id < 0) {
        skin_error(MSGTR_SKIN_FONT_NonExistentFont, fnt);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     font: %s (#%d)\n", fnt, id);

    item = next_item();

    if (!item)
        return 1;

    item->type   = itDLabel;
    item->fontid = id;
    item->align  = a;
    item->x      = x;
    item->y      = y;
    item->width  = w;
    item->height = -1;
    item->label  = strdup(txt);

    if (!item->label) {
        skin_error(MSGTR_SKIN_NotEnoughMemory);
        return 1;
    }

    return 0;
}

// decoration=enable|disable
static int cmd_decoration(char *in)
{
    if (!window_cmd("decoration"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("playbar"))
        return 1;
    if (in_window("menu"))
        return 1;

    strlower(in);

    if (strcmp(in, "enable") != 0 && strcmp(in, "disable") != 0) {
        skin_error(MSGTR_SKIN_UnknownParameter, in);
        return 1;
    }

    skin->mainDecoration = (strcmp(in, "enable") == 0);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    decoration: %s\n", in);

    return 0;
}

static _item skinItem[] = {
    { "section",    cmd_section    },
    { "end",        cmd_end        },
    { "window",     cmd_window     },
    { "base",       cmd_base       },
    { "button",     cmd_button     },
    { "selected",   cmd_selected   },
    { "background", cmd_background },
    { "vpotmeter",  cmd_vpotmeter  },
    { "hpotmeter",  cmd_hpotmeter  },
    { "potmeter",   cmd_potmeter   },
    { "font",       cmd_font       },
    { "slabel",     cmd_slabel     },
    { "dlabel",     cmd_dlabel     },
    { "decoration", cmd_decoration },
    { "menu",       cmd_menu       }
};

static char *setname(char *dir, char *sname)
{
    static char skinfname[512];

    av_strlcpy(skinfname, dir, sizeof(skinfname));
    av_strlcat(skinfname, "/", sizeof(skinfname));
    av_strlcat(skinfname, sname, sizeof(skinfname));
    av_strlcat(skinfname, "/", sizeof(skinfname));
    av_strlcpy(path, skinfname, sizeof(path));
    av_strlcat(skinfname, "skin", sizeof(skinfname));

    return skinfname;
}

int skinRead(char *sname)
{
    char *skinfname;
    FILE *skinFile;
    unsigned char line[256];
    unsigned char item[32];
    unsigned char param[256];
    unsigned int i;

    skinfname = setname(skinDirInHome, sname);

    if ((skinFile = fopen(skinfname, "rt")) == NULL) {
        skinfname = setname(skinMPlayerDir, sname);

        if ((skinFile = fopen(skinfname, "rt")) == NULL) {
            mp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_SkinFileNotFound, skinfname);
            return -1;
        }
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin] configuration file: %s\n", skinfname);

    appFreeStruct();

    skin = NULL;
    currWinName[0] = 0;
    linenumber     = 0;

    while (fgets(line, sizeof(line), skinFile)) {
        linenumber++;

        line[strcspn(line, "\n\r")] = 0; // remove any kind of newline, if any
        strswap(line, '\t', ' ');
        trim(line);
        decomment(line);

        if (!*line)
            continue;

        cutItem(line, item, '=', 0);
        cutItem(line, param, '=', 1);
        strlower(item);

        for (i = 0; i < FF_ARRAY_ELEMS(skinItem); i++) {
            if (!strcmp(item, skinItem[i].name)) {
                if (skinItem[i].func(param) != 0)
                    return -2;
                else
                    break;
            }
        }

        if (i == FF_ARRAY_ELEMS(skinItem)) {
            skin_error(MSGTR_SKIN_UNKNOWN_ITEM, item);
            return -2;
        }
    }

    if (linenumber == 0) {
        mp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_SkinFileNotReadable, skinfname);
        return -1;
    }

    return 0;
}
