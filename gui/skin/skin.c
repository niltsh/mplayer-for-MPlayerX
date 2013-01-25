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
 * @brief Skin parser
 */

#include <stdio.h>
#include <string.h>

#include "skin.h"
#include "font.h"
#include "gui/app/app.h"
#include "gui/app/gui.h"
#include "gui/dialog/dialog.h"
#include "gui/interface.h"
#include "gui/util/cut.h"
#include "gui/util/string.h"

#include "help_mp.h"
#include "libavutil/avstring.h"
#include "libavutil/common.h"
#include "mp_msg.h"

typedef struct {
    const char *name;
    int (*func)(char *in);
} _item;

char *skinDirInHome;
char *skinMPlayerDir;

static guiItems *skin;

static int linenumber;
static unsigned char path[512];

static unsigned char currWinName[32];
static guiItem *currWin;
static int *currWinItemIdx;
static guiItem *currWinItems;

/**
 * @brief Display a skin error message.
 *
 * @param format format string
 * @param ... arguments
 */
static void skin_error(const char *format, ...)
{
    char p[512];
    va_list ap;

    va_start(ap, format);
    vsnprintf(p, sizeof(p), format, ap);
    va_end(ap);

    gmp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_ERRORMESSAGE, linenumber, p);
}

/**
 * @brief Check whether a @a section definition has started.
 *
 * @param item name of the item to be put in a message in case of an error
 *
 * @return #True (ok) or #False (error)
 */
static int section_item(char *item)
{
    if (!skin) {
        skin_error(MSGTR_SKIN_ERROR_SECTION, item);
        return False;
    }

    return True;
}

/**
 * @brief Check whether a @a window definition has started.
 *
 * @param item name of the item to be put in a message in case of an error
 *
 * @return #True (ok) or #False (error)
 */
static int window_item(char *item)
{
    if (!currWinName[0]) {
        skin_error(MSGTR_SKIN_ERROR_WINDOW, item);
        return False;
    }

    return True;
}

/**
 * @brief Check whether a specific @a window definition has started.
 *
 * @param name name of the window to be checked
 *
 * @return 0 (ok) or 1 (error)
 */
static int in_window(char *name)
{
    if (strcmp(currWinName, name) == 0) {
        skin_error(MSGTR_SKIN_ERROR_ITEM, name);
        return 1;
    }

    return 0;
}

/**
 * @brief Get next free item in current @a window.
 *
 * @return pointer to next free item (ok) or NULL (error)
 */
static guiItem *next_item(void)
{
    guiItem *item = NULL;

    if (*currWinItemIdx < MAX_ITEMS - 1) {
        (*currWinItemIdx)++;
        item = &currWinItems[*currWinItemIdx];
    } else
        skin_error(MSGTR_SKIN_TooManyItemsDeclared);

    return item;
}

/**
 * @brief Parse a @a section definition.
 *
 *        Syntax: section=movieplayer
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_section(char *in)
{
    if (skin) {
        skin_error(MSGTR_SKIN_ERROR_ITEM, "section");
        return 1;
    }

    if (!strcmp(strlower(in), "movieplayer"))
        skin = &guiApp;
    else {
        skin_error(MSGTR_SKIN_UNKNOWN_NAME, in);
        return 1;
    }

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]  section: %s\n", in);

    return 0;
}

/**
 * @brief Parse an @a end definition.
 *
 *        Syntax: end
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_end(char *in)
{
    char *space, *name;

    (void)in;

    if (currWinName[0]) {
        space = " ";
        name  = currWinName;
    } else {
        space = "";
        name  = "section";
    }

    if (!section_item("end"))
        return 1;

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]  %send (%s)\n", space, name);

    if (currWinName[0]) {
        currWinName[0] = 0;
        currWin = NULL;
        currWinItemIdx = NULL;
        currWinItems   = NULL;
    } else
        skin = NULL;

    return 0;
}

/**
 * @brief Parse a @a window definition.
 *
 *        Syntax: window=main|video|playbar|menu
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_window(char *in)
{
    if (!section_item("window"))
        return 1;

    if (currWinName[0]) {
        skin_error(MSGTR_SKIN_ERROR_ITEM, "window");
        return 1;
    }

    strlower(in);

    if (strcmp(in, "sub") == 0)
        strcpy(in, "video");                           // legacy

    if (strcmp(in, "main") == 0) {
        currWin = &skin->main;
        currWinItemIdx = &skin->IndexOfMainItems;
        currWinItems   = skin->mainItems;
    } else if (strcmp(in, "video") == 0) {
        currWin = &skin->video;
        currWinItemIdx = NULL;
        currWinItems   = NULL;
    } else if (strcmp(in, "playbar") == 0) {
        currWin = &skin->playbar;
        currWinItemIdx = &skin->IndexOfPlaybarItems;
        currWinItems   = skin->playbarItems;
    } else if (strcmp(in, "menu") == 0) {
        currWin = &skin->menu;
        currWinItemIdx = &skin->IndexOfMenuItems;
        currWinItems   = skin->menuItems;
    } else {
        skin_error(MSGTR_SKIN_UNKNOWN_NAME, in);
        return 1;
    }

    av_strlcpy(currWinName, in, sizeof(currWinName));

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]   window: %s\n", currWinName);

    return 0;
}

/**
 * @brief Parse a @a base definition.
 *
 *        Syntax: base=image,x,y[,width,height]
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_base(char *in)
{
    unsigned char fname[256];
    unsigned char file[512];
    int x, y;
    int w = 0, h = 0;
    int is_video, is_bar, is_menu;

    if (!window_item("base"))
        return 1;

    is_video = (strcmp(currWinName, "video") == 0);
    is_bar   = (strcmp(currWinName, "playbar") == 0);
    is_menu  = (strcmp(currWinName, "menu") == 0);

    cutItem(in, fname, ',', 0);
    x = cutItemToInt(in, ',', 1);
    y = cutItemToInt(in, ',', 2);
    w = cutItemToInt(in, ',', 3);
    h = cutItemToInt(in, ',', 4);

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image: %s", fname);

    currWin->type = itBase;

    if (!is_menu) {
        currWin->x = x;
        currWin->y = y;

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, " %d,%d", x, y);
    }

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "\n");

    av_strlcpy(file, path, sizeof(file));
    av_strlcat(file, fname, sizeof(file));

    if (skinImageRead(file, &currWin->Bitmap) != 0)
        return 1;

    currWin->width  = currWin->Bitmap.Width;
    currWin->height = currWin->Bitmap.Height;

    if (is_video) {
        if (w && h) {
            currWin->width  = w;
            currWin->height = h;
        }
    }

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    %s: %dx%d\n", (is_video && w && h ? "size" : " bitmap"), currWin->width, currWin->height);

    if (!is_video) {
        if (!bpRenderMask(&currWin->Bitmap, &currWin->Mask)) {
            skin_error(MSGTR_SKIN_NotEnoughMemory);
            return 1;
        }
        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     mask: %lux%lu\n", currWin->Mask.Width, currWin->Mask.Height);
    }

    if (is_bar)
        skin->playbarIsPresent = True;
    if (is_menu)
        skin->menuIsPresent = True;

    return 0;
}

/**
 * @brief Parse a @a background definition.
 *
 *        Syntax: background=R,G,B
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_background(char *in)
{
    if (!window_item("background"))
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

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    background color: #%02x%02x%02x\n", currWin->R, currWin->G, currWin->B);

    return 0;
}

/**
 * @brief Parse a @a button definition.
 *
 *        Syntax: button=image,x,y,width,height,message
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_button(char *in)
{
    unsigned char fname[256];
    unsigned char file[512];
    int x, y, w, h, message;
    char msg[32];
    guiItem *item;

    if (!window_item("button"))
        return 1;

    if (in_window("video"))
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

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    button image: %s %d,%d\n", fname, x, y);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", msg, message);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     size: %dx%d\n", w, h);

    item = next_item();

    if (!item)
        return 1;

    item->type    = itButton;
    item->x       = x;
    item->y       = y;
    item->width   = w;
    item->height  = h;
    item->message = message;
    item->pressed = btnReleased;

    if (item->message == evPauseSwitchToPlay)
        item->pressed = btnDisabled;

    item->Bitmap.Image = NULL;

    if (strcmp(fname, "NULL") != 0) {
        av_strlcpy(file, path, sizeof(file));
        av_strlcat(file, fname, sizeof(file));

        if (skinImageRead(file, &item->Bitmap) != 0)
            return 1;

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    return 0;
}

/**
 * @brief Parse a @a selected definition.
 *
 *        Syntax: selected=image
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_selected(char *in)
{
    unsigned char file[512];
    guiItem *currItem;

    if (!window_item("selected"))
        return 1;

    if (in_window("main"))
        return 1;
    if (in_window("video"))
        return 1;
    if (in_window("playbar"))
        return 1;

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    image selected: %s\n", in);

    currItem       = &skin->menuSelected;
    currItem->type = itBase;

    av_strlcpy(file, path, sizeof(file));
    av_strlcat(file, in, sizeof(file));

    if (skinImageRead(file, &currItem->Bitmap) != 0)
        return 1;

    currItem->width  = currItem->Bitmap.Width;
    currItem->height = currItem->Bitmap.Height;

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     bitmap: %dx%d\n", currItem->width, currItem->height);

    return 0;
}

/**
 * @brief Parse a @a menu definition.
 *
 *        Syntax: menu=x,y,width,height,message
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_menu(char *in)
{
    int x, y, w, h, message;
    char msg[32];
    guiItem *item;

    if (!window_item("menu"))
        return 1;

    if (in_window("main"))
        return 1;
    if (in_window("video"))
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

    item->type    = itMenu;
    item->x       = x;
    item->y       = y;
    item->width   = w;
    item->height  = h;
    item->message = message;

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    item #%d: %d,%d %dx%d\n", *currWinItemIdx, x, y, w, h);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", msg, message);

    item->Bitmap.Image = NULL;

    return 0;
}

/**
 * @brief Parse a @a hpotmeter definition.
 *
 *        Syntax: hpotmeter=button,bwidth,bheight,phases,numphases,default,x,y,width,height,message
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_hpotmeter(char *in)
{
    unsigned char pfname[256];
    unsigned char phfname[256];
    unsigned char buf[512];
    int pwidth, pheight, ph, d, x, y, w, h, message;
    guiItem *item;

    if (!window_item("h/v potmeter"))
        return 1;

    if (in_window("video"))
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

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    h/v potmeter image: %s %d,%d %dx%d\n", phfname, x, y, w, h);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     button image: %s %dx%d\n", pfname, pwidth, pheight);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     numphases: %d, default: %d%%\n", ph, d);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", buf, message);

    item = next_item();

    if (!item)
        return 1;

    item->type      = itHPotmeter;
    item->x         = x;
    item->y         = y;
    item->width     = w;
    item->height    = h;
    item->pwidth    = pwidth;
    item->pheight   = pheight;
    item->numphases = ph;
    item->value     = (float)d;
    item->message   = message;
    item->pressed   = btnReleased;

    item->Bitmap.Image = NULL;

    if (strcmp(phfname, "NULL") != 0) {
        av_strlcpy(buf, path, sizeof(buf));
        av_strlcat(buf, phfname, sizeof(buf));

        if (skinImageRead(buf, &item->Bitmap) != 0)
            return 1;

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (potmeter bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    item->Mask.Image = NULL;

    if (strcmp(pfname, "NULL") != 0) {
        av_strlcpy(buf, path, sizeof(buf));
        av_strlcat(buf, pfname, sizeof(buf));

        if (skinImageRead(buf, &item->Mask) != 0)
            return 1;

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (button bitmap: %lux%lu)\n", item->Mask.Width, item->Mask.Height);
    }

    return 0;
}

/**
 * @brief Parse a @a vpotmeter definition.
 *
 *        Syntax: vpotmeter=button,bwidth,bheight,phases,numphases,default,x,y,width,height,message
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_vpotmeter(char *in)
{
    int r;
    guiItem *item;

    r = item_hpotmeter(in);

    if (r == 0) {
        item       = &currWinItems[*currWinItemIdx];
        item->type = itVPotmeter;
    }

    return r;
}

/**
 * @brief Parse a @a potmeter definition.
 *
 *        Syntax: potmeter=phases,numphases,default,x,y,width,height,message
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_potmeter(char *in)
{
    unsigned char phfname[256];
    unsigned char buf[512];
    int ph, d, x, y, w, h, message;
    guiItem *item;

    if (!window_item("potmeter"))
        return 1;

    if (in_window("video"))
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

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    potmeter image: %s %d,%d %dx%d\n", phfname, x, y, w, h);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     numphases: %d, default: %d%%\n", ph, d);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     message: %s (#%d)\n", buf, message);

    item = next_item();

    if (!item)
        return 1;

    item->type      = itPotmeter;
    item->x         = x;
    item->y         = y;
    item->width     = w;
    item->height    = h;
    item->numphases = ph;
    item->value     = (float)d;
    item->message   = message;

    item->Bitmap.Image = NULL;

    if (strcmp(phfname, "NULL") != 0) {
        av_strlcpy(buf, path, sizeof(buf));
        av_strlcat(buf, phfname, sizeof(buf));

        if (skinImageRead(buf, &item->Bitmap) != 0)
            return 1;

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     (bitmap: %lux%lu)\n", item->Bitmap.Width, item->Bitmap.Height);
    }

    return 0;
}

/**
 * @brief Parse a @a font definition.
 *
 *        Syntax: font=fontfile
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_font(char *in)
{
    char fnt[256];

    if (!window_item("font"))
        return 1;

    if (in_window("video"))
        return 1;
    if (in_window("menu"))
        return 1;

    cutItem(in, fnt, ',', 0);   // Note: This seems needless but isn't for compatibility
                                // reasons with a meanwhile depreciated second parameter.
    switch (fntRead(path, fnt)) {
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

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    font: %s (#%d)\n", fnt, fntFindID(fnt));

    return 0;
}

/**
 * @brief Parse a @a slabel definition.
 *
 *        Syntax: slabel=x,y,fontfile,"text"
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_slabel(char *in)
{
    int x, y, id;
    char fnt[256];
    char txt[256];
    guiItem *item;

    if (!window_item("slabel"))
        return 1;

    if (in_window("video"))
        return 1;
    if (in_window("menu"))
        return 1;

    x = cutItemToInt(in, ',', 0);
    y = cutItemToInt(in, ',', 1);
    cutItem(in, fnt, ',', 2);
    cutItem(in, txt, ',', 3);
    cutItem(txt, txt, '"', 1);

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    slabel: \"%s\"\n", txt);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     pos: %d,%d\n", x, y);

    id = fntFindID(fnt);

    if (id < 0) {
        skin_error(MSGTR_SKIN_FONT_NonExistentFont, fnt);
        return 1;
    }

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     font: %s (#%d)\n", fnt, id);

    item = next_item();

    if (!item)
        return 1;

    item->type   = itSLabel;
    item->x      = x;
    item->y      = y;
    item->width  = -1;
    item->height = -1;
    item->fontid = id;
    item->label  = strdup(txt);

    if (!item->label) {
        skin_error(MSGTR_SKIN_NotEnoughMemory);
        return 1;
    }

    return 0;
}

/**
 * @brief Parse a @a dlabel definition.
 *
 *        Syntax: dlabel=x,y,width,align,fontfile,"text"
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_dlabel(char *in)
{
    int x, y, w, a, id;
    char fnt[256];
    char txt[256];
    guiItem *item;

    if (!window_item("dlabel"))
        return 1;

    if (in_window("video"))
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

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    dlabel: \"%s\"\n", txt);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     pos: %d,%d\n", x, y);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     width: %d, align: %d\n", w, a);

    id = fntFindID(fnt);

    if (id < 0) {
        skin_error(MSGTR_SKIN_FONT_NonExistentFont, fnt);
        return 1;
    }

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]     font: %s (#%d)\n", fnt, id);

    item = next_item();

    if (!item)
        return 1;

    item->type   = itDLabel;
    item->x      = x;
    item->y      = y;
    item->width  = w;
    item->height = -1;
    item->fontid = id;
    item->align  = a;
    item->label  = strdup(txt);

    if (!item->label) {
        skin_error(MSGTR_SKIN_NotEnoughMemory);
        return 1;
    }

    return 0;
}

/**
 * @brief Parse a @a decoration definition.
 *
 *        Syntax: decoration=enable|disable
 *
 * @param in definition to be analyzed
 *
 * @return 0 (ok) or 1 (error)
 */
static int item_decoration(char *in)
{
    if (!window_item("decoration"))
        return 1;

    if (in_window("video"))
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

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin]    decoration: %s\n", in);

    return 0;
}

/**
 * @brief Parsing functions responsible for skin item definitions.
 */
static _item skinItem[] = {
    { "background", item_background },
    { "base",       item_base       },
    { "button",     item_button     },
    { "decoration", item_decoration },
    { "dlabel",     item_dlabel     },
    { "end",        item_end        },
    { "font",       item_font       },
    { "hpotmeter",  item_hpotmeter  },
    { "menu",       item_menu       },
    { "potmeter",   item_potmeter   },
    { "section",    item_section    },
    { "selected",   item_selected   },
    { "slabel",     item_slabel     },
    { "vpotmeter",  item_vpotmeter  },
    { "window",     item_window     }
};

/**
 * @brief Read a skin @a image file.
 *
 * @param fname filename (with path)
 * @param img pointer suitable to store the image data
 *
 * @return return code of #bpRead()
 */
int skinImageRead(char *fname, guiImage *img)
{
    int i = bpRead(fname, img);

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

/**
 * @brief Build the skin file path for a skin name.
 *
 * @param dir skins directory
 * @param sname name of the skin
 *
 * @return skin file path
 *
 * @note As a side effect, variable #path gets set to the skin path.
 */
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

/**
 * @brief Read and parse a skin.
 *
 * @param sname name of the skin
 *
 * @return 0 (ok), -1 (skin file not found or not readable) or -2 (parsing error)
 */
int skinRead(char *sname)
{
    char *skinfname;
    FILE *skinfile;
    unsigned char line[256];
    unsigned char item[32];
    unsigned char param[256];
    unsigned int i;

    skinfname = setname(skinDirInHome, sname);

    if ((skinfile = fopen(skinfname, "rt")) == NULL) {
        skinfname = setname(skinMPlayerDir, sname);

        if ((skinfile = fopen(skinfname, "rt")) == NULL) {
            mp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_SkinFileNotFound, skinfname);
            return -1;
        }
    }

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[skin] configuration file: %s\n", skinfname);

    appFreeStruct();

    skin = NULL;
    currWinName[0] = 0;
    linenumber     = 0;

    while (fgetstr(line, sizeof(line), skinfile)) {
        linenumber++;

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
                if (skinItem[i].func(param) != 0) {
                    fclose(skinfile);
                    return -2;
                } else
                    break;
            }
        }

        if (i == FF_ARRAY_ELEMS(skinItem)) {
            skin_error(MSGTR_SKIN_UNKNOWN_ITEM, item);
            fclose(skinfile);
            return -2;
        }
    }

    fclose(skinfile);

    if (linenumber == 0) {
        mp_msg(MSGT_GPLAYER, MSGL_ERR, MSGTR_SKIN_SkinFileNotReadable, skinfname);
        return -1;
    }

    return 0;
}
