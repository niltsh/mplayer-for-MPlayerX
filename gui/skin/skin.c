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

    av_strlcpy(currWinName, strlower(in), sizeof(currWinName));

    if (!strncmp(in, "main", 4)) {
        currWin = &appMPlayer.main;
        currWinItemIdx = &appMPlayer.IndexOfMainItems;
        currWinItems   = appMPlayer.mainItems;
    } else if (!strncmp(in, "sub", 3))
        currWin = &appMPlayer.sub;
    else if (!strncmp(in, "playbar", 7)) {
        currWin = &appMPlayer.bar;
        currWinItemIdx = &appMPlayer.IndexOfBarItems;
        currWinItems   = appMPlayer.barItems;
    } else if (!strncmp(in, "menu", 4)) {
        currWin = &appMPlayer.menuBase;
        currWinItemIdx = &appMPlayer.IndexOfMenuItems;
        currWinItems   = appMPlayer.menuItems;
    } else {
        skin_error(MSGTR_SKIN_UNKNOWN_NAME, in);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]   window: %s\n", currWinName);

    return 0;
}

// base=image,x,y[,width,height]
static int cmd_base(char *in)
{
    unsigned char fname[512];
    unsigned char tmp[512];
    int x, y;
    int sx = 0, sy = 0;

    if (!window_cmd("base"))
        return 1;

    cutItem(in, fname, ',', 0);
    x  = cutItemToInt(in, ',', 1);
    y  = cutItemToInt(in, ',', 2);
    sx = cutItemToInt(in, ',', 3);
    sy = cutItemToInt(in, ',', 4);

    if (!strcmp(currWinName, "main")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s %d,%d\n", fname, x, y);

        skin->main.x    = x;
        skin->main.y    = y;
        skin->main.type = itBase;

        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, fname, sizeof(tmp));

        if (skinBPRead(tmp, &skin->main.Bitmap) != 0)
            return 1;

        skin->main.width  = skin->main.Bitmap.Width;
        skin->main.height = skin->main.Bitmap.Height;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", skin->main.width, skin->main.height);

#ifdef CONFIG_XSHAPE
        Convert32to1(&skin->main.Bitmap, &skin->main.Mask, 0x00ff00ff);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     mask: %lux%lu\n", skin->main.Mask.Width, skin->main.Mask.Height);
#else
        skin->main.Mask.Image = NULL;
#endif
    }

    if (!strcmp(currWinName, "sub")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s %d,%d\n", fname, x, y);

        skin->sub.type = itBase;

        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, fname, sizeof(tmp));

        if (skinBPRead(tmp, &skin->sub.Bitmap) != 0)
            return 1;

        skin->sub.x      = x;
        skin->sub.y      = y;
        skin->sub.width  = skin->sub.Bitmap.Width;
        skin->sub.height = skin->sub.Bitmap.Height;

        if (sx && sy) {
            skin->sub.width  = sx;
            skin->sub.height = sy;
        }

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", skin->sub.width, skin->sub.height);
    }

    if (!strcmp(currWinName, "menu")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s\n", fname);

        skin->menuIsPresent = 1;
        skin->menuBase.type = itBase;

        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, fname, sizeof(tmp));

        if (skinBPRead(tmp, &skin->menuBase.Bitmap) != 0)
            return 1;

        skin->menuBase.width  = skin->menuBase.Bitmap.Width;
        skin->menuBase.height = skin->menuBase.Bitmap.Height;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", skin->menuBase.width, skin->menuBase.height);

#ifdef CONFIG_XSHAPE
        Convert32to1(&skin->menuBase.Bitmap, &skin->menuBase.Mask, 0x00ff00ff);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     mask: %lux%lu\n", skin->menuBase.Mask.Width, skin->menuBase.Mask.Height);
#else
        skin->menuBase.Mask.Image = NULL;
#endif
    }

    if (!strcmp(currWinName, "playbar")) {
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s %d,%d\n", fname, x, y);

        skin->barIsPresent = 1;
        skin->bar.x    = x;
        skin->bar.y    = y;
        skin->bar.type = itBase;

        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, fname, sizeof(tmp));

        if (skinBPRead(tmp, &skin->bar.Bitmap) != 0)
            return 1;

        skin->bar.width  = skin->bar.Bitmap.Width;
        skin->bar.height = skin->bar.Bitmap.Height;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", skin->bar.width, skin->bar.height);

#ifdef CONFIG_XSHAPE
        Convert32to1(&skin->bar.Bitmap, &skin->bar.Mask, 0x00ff00ff);
        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     mask: %lux%lu\n", skin->bar.Mask.Width, skin->bar.Mask.Height);
#else
        skin->bar.Mask.Image = NULL;
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
    unsigned char fname[512];
    unsigned char tmp[512];
    int x, y, sx, sy;
    char msg[32];

    if (!window_cmd("button"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(in, fname, ',', 0);
    x  = cutItemToInt(in, ',', 1);
    y  = cutItemToInt(in, ',', 2);
    sx = cutItemToInt(in, ',', 3);
    sy = cutItemToInt(in, ',', 4);
    cutItem(in, msg, ',', 5);

    (*currWinItemIdx)++;
    currWinItems[*currWinItemIdx].type   = itButton;
    currWinItems[*currWinItemIdx].x      = x;
    currWinItems[*currWinItemIdx].y      = y;
    currWinItems[*currWinItemIdx].width  = sx;
    currWinItems[*currWinItemIdx].height = sy;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    button image: %s %d,%d\n", fname, x, y);

    if ((currWinItems[*currWinItemIdx].message = appFindMessage(msg)) == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, msg);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", msg, currWinItems[*currWinItemIdx].message);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     size: %dx%d\n", sx, sy);

    currWinItems[*currWinItemIdx].pressed = btnReleased;

    if (currWinItems[*currWinItemIdx].message == evPauseSwitchToPlay)
        currWinItems[*currWinItemIdx].pressed = btnDisabled;

    currWinItems[*currWinItemIdx].tmp = 1;

    currWinItems[*currWinItemIdx].Bitmap.Image = NULL;

    if (strcmp(fname, "NULL") != 0) {
        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, fname, sizeof(tmp));

        if (skinBPRead(tmp, &currWinItems[*currWinItemIdx].Bitmap) != 0)
            return 1;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (bitmap: %lux%lu)\n", currWinItems[*currWinItemIdx].Bitmap.Width, currWinItems[*currWinItemIdx].Bitmap.Height);
    }

    return 0;
}

// selected=image
static int cmd_selected(char *in)
{
    unsigned char fname[512];
    unsigned char tmp[512];

    if (!window_cmd("selected"))
        return 1;

    if (in_window("main"))
        return 1;
    if (in_window("sub"))
        return 1;
    if (in_window("playbar"))
        return 1;

    cutItem(in, fname, ',', 0);

    skin->menuSelected.type = itBase;

    av_strlcpy(tmp, path, sizeof(tmp));
    av_strlcat(tmp, fname, sizeof(tmp));

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image selected: %s\n", fname);

    if (skinBPRead(tmp, &skin->menuSelected.Bitmap) != 0)
        return 1;

    skin->menuSelected.width  = skin->menuSelected.Bitmap.Width;
    skin->menuSelected.height = skin->menuSelected.Bitmap.Height;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", skin->menuSelected.width, skin->menuSelected.height);

    return 0;
}

// menu=x,y,width,height,message
static int cmd_menu(char *in)
{
    int x, y, sx, sy, message;
    unsigned char tmp[64];

    if (!window_cmd("menu"))
        return 1;

    if (in_window("main"))
        return 1;
    if (in_window("sub"))
        return 1;
    if (in_window("playbar"))
        return 1;

    x  = cutItemToInt(in, ',', 0);
    y  = cutItemToInt(in, ',', 1);
    sx = cutItemToInt(in, ',', 2);
    sy = cutItemToInt(in, ',', 3);
    cutItem(in, tmp, ',', 4);

    message = appFindMessage(tmp);

    skin->IndexOfMenuItems++;
    skin->menuItems[skin->IndexOfMenuItems].x      = x;
    skin->menuItems[skin->IndexOfMenuItems].y      = y;
    skin->menuItems[skin->IndexOfMenuItems].width  = sx;
    skin->menuItems[skin->IndexOfMenuItems].height = sy;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    item #%d: %d,%d %dx%d\n", skin->IndexOfMenuItems, x, y, sx, sy);

    if ((skin->menuItems[skin->IndexOfMenuItems].message = message) == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, tmp);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", tmp, skin->menuItems[skin->IndexOfMenuItems].message);

    skin->menuItems[skin->IndexOfMenuItems].Bitmap.Image = NULL;

    return 0;
}

// hpotmeter=button,bwidth,bheight,phases,numphases,default,x,y,width,height,message
static int cmd_hpotmeter(char *in)
{
    int x, y, pwidth, pheight, ph, sx, sy, message, d;
    unsigned char tmp[512];
    unsigned char pfname[512];
    unsigned char phfname[512];
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
    sx = cutItemToInt(in, ',', 8);
    sy = cutItemToInt(in, ',', 9);
    cutItem(in, tmp, ',', 10);

    message = appFindMessage(tmp);

    if (message == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, tmp);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    h/v potmeter image: %s %d,%d %dx%d\n", phfname, x, y, sx, sy);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     button image: %s %dx%d\n", pfname, pwidth, pheight);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     numphases: %d, default: %d%%\n", ph, d);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", tmp, message);

    (*currWinItemIdx)++;
    item               = &currWinItems[*currWinItemIdx];
    item->type         = itHPotmeter;
    item->x            = x;
    item->y            = y;
    item->width        = sx;
    item->height       = sy;
    item->numphases    = ph;
    item->pwidth       = pwidth;
    item->pheight      = pheight;
    item->message      = message;
    item->value        = (float)d;
    item->pressed      = btnReleased;
    item->Bitmap.Image = NULL;

    if (strcmp(phfname, "NULL") != 0) {
        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, phfname, sizeof(tmp));

        if (skinBPRead(tmp, &item->Bitmap) != 0)
            return 1;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (potmeter bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    item->Mask.Image = NULL;

    if (strcmp(pfname, "NULL") != 0) {
        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, pfname, sizeof(tmp));

        if (skinBPRead(tmp, &item->Mask) != 0)
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
    int x, y, ph, sx, sy, message, d;
    unsigned char tmp[512];
    unsigned char phfname[512];
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
    sx = cutItemToInt(in, ',', 5);
    sy = cutItemToInt(in, ',', 6);
    cutItem(in, tmp, ',', 7);

    message = appFindMessage(tmp);

    if (message == -1) {
        skin_error(MSGTR_SKIN_UnknownMessage, tmp);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    potmeter image: %s %d,%d %dx%d\n", phfname, x, y, sx, sy);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     numphases: %d, default: %d%%\n", ph, d);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", tmp, message);

    (*currWinItemIdx)++;
    item               = &currWinItems[*currWinItemIdx];
    item->type         = itPotmeter;
    item->x            = x;
    item->y            = y;
    item->width        = sx;
    item->height       = sy;
    item->numphases    = ph;
    item->message      = message;
    item->value        = (float)d;
    item->Bitmap.Image = NULL;

    if (strcmp(phfname, "NULL") != 0) {
        av_strlcpy(tmp, path, sizeof(tmp));
        av_strlcat(tmp, phfname, sizeof(tmp));

        if (skinBPRead(tmp, &item->Bitmap) != 0)
            return 1;

        mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    return 0;
}

// font=fontfile
static int cmd_font(char *in)
{
    char name[512];
    wItem *item;

    if (!window_cmd("font"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(in, name, ',', 0);

    (*currWinItemIdx)++;
    item         = &currWinItems[*currWinItemIdx];
    item->type   = itFont;
    item->fontid = fntRead(path, name);

    switch (item->fontid) {
    case -1:
        skin_error(MSGTR_SKIN_FONT_NotEnoughtMemory);
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

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    font: %s (#%d)\n", name, fntFindID(name));

    return 0;
}

// slabel=x,y,fontfile,"text"
static int cmd_slabel(char *in)
{
    char tmp[512];
    char sid[64];
    int x, y, id;
    wItem *item;

    if (!window_cmd("slabel"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    x = cutItemToInt(in, ',', 0);
    y = cutItemToInt(in, ',', 1);
    cutItem(in, sid, ',', 2);
    cutItem(in, tmp, ',', 3);
    cutItem(tmp, tmp, '"', 1);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    slabel: \"%s\"\n", tmp);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     pos: %d,%d\n", x, y);

    id = fntFindID(sid);

    if (id < 0) {
        skin_error(MSGTR_SKIN_FONT_NonExistentFontID, sid);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     font: %s (#%d)\n", sid, id);

    (*currWinItemIdx)++;
    item         = &currWinItems[*currWinItemIdx];
    item->type   = itSLabel;
    item->fontid = id;
    item->x      = x;
    item->y      = y;
    item->width  = -1;
    item->height = -1;
    item->label  = strdup(tmp);

    if (!item->label) {
        skin_error(MSGTR_SKIN_FONT_NotEnoughtMemory);
        return 1;
    }

    return 0;
}

// dlabel=x,y,width,align,fontfile,"text"
static int cmd_dlabel(char *in)
{
    char tmp[512];
    char sid[64];
    int x, y, sx, id, a;
    wItem *item;

    if (!window_cmd("dlabel"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("menu"))
        return 1;

    x  = cutItemToInt(in, ',', 0);
    y  = cutItemToInt(in, ',', 1);
    sx = cutItemToInt(in, ',', 2);
    a  = cutItemToInt(in, ',', 3);
    cutItem(in, sid, ',', 4);
    cutItem(in, tmp, ',', 5);
    cutItem(tmp, tmp, '"', 1);

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    dlabel: \"%s\"\n", tmp);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     pos: %d,%d\n", x, y);
    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     width: %d, align: %d\n", sx, a);

    id = fntFindID(sid);

    if (id < 0) {
        skin_error(MSGTR_SKIN_FONT_NonExistentFontID, sid);
        return 1;
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     font: %s (#%d)\n", sid, id);

    (*currWinItemIdx)++;
    item         = &currWinItems[*currWinItemIdx];
    item->type   = itDLabel;
    item->fontid = id;
    item->align  = a;
    item->x      = x;
    item->y      = y;
    item->width  = sx;
    item->height = -1;
    item->label  = strdup(tmp);

    if (!item->label) {
        skin_error(MSGTR_SKIN_FONT_NotEnoughtMemory);
        return 1;
    }

    return 0;
}

// decoration=enable|disable
static int cmd_decoration(char *in)
{
    char tmp[512];

    if (!window_cmd("decoration"))
        return 1;

    if (in_window("sub"))
        return 1;
    if (in_window("playbar"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(strlower(in), tmp, ',', 0);

    if (strcmp(tmp, "enable") != 0 && strcmp(tmp, "disable") != 0) {
        skin_error(MSGTR_SKIN_UnknownParameter, tmp);
        return 1;
    }

    if (strcmp(tmp, "enable") != 0)
        skin->mainDecoration = 0;
    else
        skin->mainDecoration = 1;

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    decoration: %s\n", tmp);

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

static char *setname(char *item1, char *item2)
{
    static char fn[512];

    av_strlcpy(fn, item1, sizeof(fn));
    av_strlcat(fn, "/", sizeof(fn));
    av_strlcat(fn, item2, sizeof(fn));
    av_strlcpy(path, fn, sizeof(path));
    av_strlcat(path, "/", sizeof(path));
    av_strlcat(fn, "/skin", sizeof(fn));

    return fn;
}

int skinRead(char *dname)
{
    char *fn;
    FILE *skinFile;
    unsigned char tmp[256];
    unsigned char *ptmp;
    unsigned char command[32];
    unsigned char param[256];
    unsigned int i;

    fn = setname(skinDirInHome, dname);

    if ((skinFile = fopen(fn, "rt")) == NULL) {
        fn = setname(skinMPlayerDir, dname);

        if ((skinFile = fopen(fn, "rt")) == NULL) {
            mp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_SkinFileNotFound, fn);
            return -1;
        }
    }

    mp_dbg(MSGT_GPLAYER, MSGL_DBG2, "[skin] configuration file: %s\n", fn);

    appFreeStruct();

    linenumber = 0;

    while (fgets(tmp, sizeof(tmp), skinFile)) {
        linenumber++;

        tmp[strcspn(tmp, "\n\r")] = 0; // remove any kind of newline, if any
        strswap(tmp, '\t', ' ');
        trim(tmp);
        ptmp = strchr(tmp, ';');

        if (ptmp)
            *ptmp = 0;

        if (!*tmp)
            continue;

        cutItem(tmp, command, '=', 0);
        cutItem(tmp, param, '=', 1);
        strlower(command);

        for (i = 0; i < FF_ARRAY_ELEMS(skinItem); i++)
            if (!strcmp(command, skinItem[i].name))
                if (skinItem[i].func(param) != 0)
                    return -2;
    }

    if (linenumber == 0) {
        mp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_SkinFileNotReadable, fn);
        return -1;
    }

    return 0;
}
