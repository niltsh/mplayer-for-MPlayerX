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

#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "gmplayer.h"
#include "gui/app.h"
#include "gui/interface.h"
#include "gui/skin/font.h"
#include "gui/skin/skin.h"
#include "gui/util/list.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"
#include "gui/wm/wsxdnd.h"
#include "widgets.h"

#include "config.h"
#include "help_mp.h"
#include "input/input.h"
#include "libmpcodecs/vd.h"
#include "libvo/video_out.h"
#include "mp_core.h"
#include "stream/stream.h"

int uiGotoTheNext = 1;

void uiFullScreen(void)
{
    if (!guiInfo.MovieWindow && guiInfo.Playing)
        return;

    if (guiInfo.Playing && guiApp.subWindow.isFullScreen) {
        guiApp.subWindow.OldWidth  = guiInfo.MovieWidth;
        guiApp.subWindow.OldHeight = guiInfo.MovieHeight;

        switch (guiApp.sub.x) {
        case -1:
            guiApp.subWindow.OldX = wsMaxX / 2 - guiApp.subWindow.OldWidth / 2 + wsOrgX;
            break;

        case -2:
            guiApp.subWindow.OldX = wsMaxX - guiApp.subWindow.OldWidth + wsOrgX;
            break;

        default:
            guiApp.subWindow.OldX = guiApp.sub.x;
            break;
        }

        switch (guiApp.sub.y) {
        case -1:
            guiApp.subWindow.OldY = wsMaxY / 2 - guiApp.subWindow.OldHeight / 2 + wsOrgY;
            break;

        case -2:
            guiApp.subWindow.OldY = wsMaxY - guiApp.subWindow.OldHeight + wsOrgY;
            break;

        default:
            guiApp.subWindow.OldY = guiApp.sub.y;
            break;
        }
    }

    if (guiInfo.Playing || gtkShowVideoWindow)
        wsFullScreen(&guiApp.subWindow);

    fullscreen = vo_fs = guiApp.subWindow.isFullScreen;
    wsSetLayer(wsDisplay, guiApp.mainWindow.WindowID, guiApp.subWindow.isFullScreen);

    if (guiApp.menuIsPresent)
        wsSetLayer(wsDisplay, guiApp.menuWindow.WindowID, guiApp.subWindow.isFullScreen);

    if (guiInfo.Playing)
        wsSetBackgroundRGB(&guiApp.subWindow, 0, 0, 0);
    else
        wsSetBackgroundRGB(&guiApp.subWindow, guiApp.sub.R, guiApp.sub.G, guiApp.sub.B);
}

void uiPlay(void)
{
    if (!guiInfo.Filename ||
        (guiInfo.Filename[0] == 0) ||
        (guiInfo.Playing == GUI_PLAY))
        return;

    if (guiInfo.Playing == GUI_PAUSE) {
        uiPause();
        return;
    }

    gui(GUI_SET_STATE, (void *)GUI_PLAY);
    uiSubRender = 0;
    wsSetBackgroundRGB(&guiApp.subWindow, 0, 0, 0);
    wsClearWindow(guiApp.subWindow);
}

void uiPause(void)
{
    if (!guiInfo.Playing)
        return;

    if (guiInfo.Playing == GUI_PLAY) {
        mp_cmd_t *cmd = calloc(1, sizeof(*cmd));
        cmd->id   = MP_CMD_PAUSE;
        cmd->name = strdup("pause");
        mp_input_queue_cmd(cmd);
    } else
        guiInfo.Playing = GUI_PLAY;
}

void uiState(void)
{
    if (guiInfo.Playing == GUI_STOP || guiInfo.Playing == GUI_PAUSE) {
        btnModify(evPlaySwitchToPause, btnReleased);
        btnModify(evPauseSwitchToPlay, btnDisabled);
    } else {
        btnModify(evPlaySwitchToPause, btnDisabled);
        btnModify(evPauseSwitchToPlay, btnReleased);
    }
}

void uiRelSeek(float sec)
{
    rel_seek_secs = sec;
    abs_seek_pos  = 0;
}

void uiAbsSeek(float percent)
{
    if (guiInfo.StreamType == STREAMTYPE_STREAM)
        return;

    rel_seek_secs = percent / 100.0;
    abs_seek_pos  = 3;
}

void uiChangeSkin(char *name)
{
    int prev, bprev;

    prev  = guiApp.menuIsPresent;
    bprev = guiApp.playbarIsPresent;

    mainVisible = 0;

    if (skinRead(name) != 0) {
        if (skinRead(skinName) != 0) {
            mainVisible = 1;
            return;
        }
    }

    // reload menu window

    if (prev && guiApp.menuIsPresent) {
        free(menuDrawBuffer);
        menuDrawBuffer = calloc(1, guiApp.menu.Bitmap.ImageSize);

        if (!menuDrawBuffer) {
            gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_NEMDB);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }

        wsResizeWindow(&guiApp.menuWindow, guiApp.menu.width, guiApp.menu.height);
        wsResizeImage(&guiApp.menuWindow, guiApp.menu.width, guiApp.menu.height);
        wsSetShape(&guiApp.menuWindow, guiApp.menu.Mask.Image);
        wsVisibleWindow(&guiApp.menuWindow, wsHideWindow);
    } else
        uiMenuInit();

    // reload sub window

    if (guiApp.sub.Bitmap.Image)
        wsResizeImage(&guiApp.subWindow, guiApp.sub.Bitmap.Width, guiApp.sub.Bitmap.Height);

    if (!guiApp.subWindow.isFullScreen && !guiInfo.Playing) {
        wsResizeWindow(&guiApp.subWindow, guiApp.sub.width, guiApp.sub.height);
        wsMoveWindow(&guiApp.subWindow, True, guiApp.sub.x, guiApp.sub.y);
    }

    if (guiApp.sub.Bitmap.Image)
        wsConvert(&guiApp.subWindow, guiApp.sub.Bitmap.Image);

    if (!guiInfo.Playing) {
        uiSubRender = 1;
        wsSetBackgroundRGB(&guiApp.subWindow, guiApp.sub.R, guiApp.sub.G, guiApp.sub.B);
        wsClearWindow(guiApp.subWindow);
        wsPostRedisplay(&guiApp.subWindow);
    }

    // reload playbar

    if (bprev)
        wsDestroyWindow(&guiApp.playbarWindow);

    uiPlaybarInit();

    // reload main window

    free(mainDrawBuffer);
    mainDrawBuffer = calloc(1, guiApp.main.Bitmap.ImageSize);

    if (!mainDrawBuffer) {
        gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_NEMDB);
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
    }

    wsDestroyWindow(&guiApp.mainWindow);

    wsCreateWindow(&guiApp.mainWindow, guiApp.main.x, guiApp.main.y, guiApp.main.width, guiApp.main.height, wsNoBorder, wsShowMouseCursor | wsHandleMouseButton | wsHandleMouseMove, wsShowFrame | wsMaxSize | wsHideWindow, "MPlayer");
    wsCreateImage(&guiApp.mainWindow, guiApp.main.Bitmap.Width, guiApp.main.Bitmap.Height);
    wsSetShape(&guiApp.mainWindow, guiApp.main.Mask.Image);
    wsSetIcon(wsDisplay, guiApp.mainWindow.WindowID, &guiIcon);

    guiApp.mainWindow.ReDraw       = (void *)uiMainDraw;
    guiApp.mainWindow.MouseHandler = uiMainMouseHandle;
    guiApp.mainWindow.KeyHandler   = uiMainKeyHandle;
    guiApp.mainWindow.DandDHandler = uiDandDHandler;

    wsXDNDMakeAwareness(&guiApp.mainWindow);

    if (!guiApp.mainDecoration)
        wsWindowDecoration(&guiApp.mainWindow, 0);

    wsVisibleWindow(&guiApp.mainWindow, wsShowWindow);
    mainVisible = 1;

    btnModify(evSetVolume, guiInfo.Volume);
    btnModify(evSetBalance, guiInfo.Balance);
    btnModify(evSetMoviePosition, guiInfo.Position);
    btnModify(evFullScreen, !guiApp.subWindow.isFullScreen);

    wsSetLayer(wsDisplay, guiApp.mainWindow.WindowID, guiApp.subWindow.isFullScreen);
    wsSetLayer(wsDisplay, guiApp.menuWindow.WindowID, guiApp.subWindow.isFullScreen);
}

void uiSetFileName(char *dir, char *name, int type)
{
    if (!name)
        return;

    if (!dir)
        setdup(&guiInfo.Filename, name);
    else
        setddup(&guiInfo.Filename, dir, name);

    guiInfo.StreamType = type;
    nfree(guiInfo.AudioFile);
    nfree(guiInfo.Subtitlename);
}

void uiCurr(void)
{
    plItem *curr;
    int stop = 0;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
#ifdef CONFIG_DVDREAD
    case STREAMTYPE_DVD:
        break;
#endif

#ifdef CONFIG_VCD
    case STREAMTYPE_VCD:
        break;
#endif

    default:

        curr = listSet(gtkGetCurrPlItem, NULL);

        if (curr) {
            uiSetFileName(curr->path, curr->name, STREAMTYPE_FILE);
            uiGotoTheNext = 0;
            break;
        }

        return;
    }

    if (stop)
        uiEventHandling(evStop, 0);

    if (guiInfo.Playing == GUI_PLAY)
        uiEventHandling(evPlay, 0);
}

void uiPrev(void)
{
    plItem *prev;
    int stop = 0;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
#ifdef CONFIG_DVDREAD
    case STREAMTYPE_DVD:

        if (--guiInfo.DVD.current_chapter == 0) {
            guiInfo.DVD.current_chapter = 1;

            if (--guiInfo.DVD.current_title <= 0) {
                guiInfo.DVD.current_title = 1;
                stop = 1;
            }
        }

        guiInfo.Track = guiInfo.DVD.current_title;
        break;
#endif

#ifdef CONFIG_VCD
    case STREAMTYPE_VCD:
        if (--guiInfo.Track == 0) {
            guiInfo.Track = 1;
            stop = 1;
        }
        break;
#endif

    default:

        prev = listSet(gtkGetPrevPlItem, NULL);

        if (prev) {
            uiSetFileName(prev->path, prev->name, STREAMTYPE_FILE);
            uiGotoTheNext = 0;
            break;
        }

        return;
    }

    if (stop)
        uiEventHandling(evStop, 0);

    if (guiInfo.Playing == GUI_PLAY)
        uiEventHandling(evPlay, 0);
}

void uiNext(void)
{
    int stop = 0;
    plItem *next;

    if (guiInfo.Playing == GUI_PAUSE)
        return;

    switch (guiInfo.StreamType) {
#ifdef CONFIG_DVDREAD
    case STREAMTYPE_DVD:

        if (guiInfo.DVD.current_chapter++ == guiInfo.DVD.chapters) {
            guiInfo.DVD.current_chapter = 1;

            if (++guiInfo.DVD.current_title > guiInfo.DVD.titles) {
                guiInfo.DVD.current_title = guiInfo.DVD.titles;
                stop = 1;
            }
        }

        guiInfo.Track = guiInfo.DVD.current_title;
        break;
#endif

#ifdef CONFIG_VCD
    case STREAMTYPE_VCD:

        if (++guiInfo.Track >= guiInfo.VCDTracks) {
            guiInfo.Track = guiInfo.VCDTracks;

            if (guiInfo.VCDTracks > 1)
                guiInfo.Track--;

            stop = 1;
        }

        break;
#endif

    default:

        next = listSet(gtkGetNextPlItem, NULL);

        if (next) {
            uiSetFileName(next->path, next->name, STREAMTYPE_FILE);
            uiGotoTheNext = 0;
            break;
        }

        return;
    }

    if (stop)
        uiEventHandling(evStop, 0);

    if (guiInfo.Playing == GUI_PLAY)
        uiEventHandling(evPlay, 0);
}
