/*
 * MPlayer GUI for Win32
 * Copyright (C) 2003 Sascha Sommer <saschasommer@freenet.de>
 * Copyright (C) 2006 Erik Augustson <erik_27can@yahoo.com>
 * Copyright (C) 2006 Gianluigi Tiesi <sherpya@netfarm.it>
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libvo/video_out.h"
#include "libao2/audio_out.h"
#include "osdep/priority.h"
#include "mixer.h"
#include "gui/util/list.h"
#include "gui/ui/gmplayer.h"
#include "gui/interface.h"
#include "gui.h"
#include "mp_msg.h"
#include "help_mp.h"
#include "dialogs.h"


static void set_defaults(void)
{
    proc_priority = "normal";
    vo_doublebuffering = 1;
    vo_directrendering = 0;
    frame_dropping = 0;
    soft_vol = 0;
    gtkAONorm = 0;
    gtkAOExtraStereo = 0;
    gtkAOExtraStereoMul = 1.0;
    audio_delay = 0.0;
    video_window = 1;
    gtkCacheOn = 0;
    gtkCacheSize = 2048;
    gtkAutoSyncOn = 0;
    gtkAutoSync = 0;
    player_idle_mode = 1;
}

static LRESULT CALLBACK PrefsWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND btn, label, edit1, edit2, updown1, updown2, track1, track2;
    static HWND vo_driver, ao_driver, prio;
    int i = 0, j = 0;
    char procprio[11];
    float x = 10.0, y = 100.0, stereopos, delaypos;
    stereopos = gtkAOExtraStereoMul * x;
    delaypos = audio_delay * y;

    switch (iMsg)
    {
        case WM_CREATE:
        {
            /* video and audio drivers */
            label = CreateWindow("static", acp(MSGTR_PREFERENCES_Video),
                                 WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 10, 14, 60, 15, hwnd,
                                 NULL, ((LPCREATESTRUCT) lParam) -> hInstance,
                                 NULL);
            SendMessage(label, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            label = CreateWindow("static", acp(MSGTR_PREFERENCES_Audio),
                                 WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 205, 14, 60, 15, hwnd,
                                 NULL, ((LPCREATESTRUCT) lParam) -> hInstance,
                                 NULL);
            SendMessage(label, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            label = CreateWindow("static", acp(MSGTR_PREFERENCES_Coefficient),
                                 WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 10, 148, 140, 15, hwnd,
                                 NULL, ((LPCREATESTRUCT) lParam) -> hInstance,
                                 NULL);
            SendMessage(label, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            label = CreateWindow("static", acp(MSGTR_PREFERENCES_AudioDelay),
                                 WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 10, 187, 140, 15, hwnd,
                                 NULL, ((LPCREATESTRUCT) lParam) -> hInstance,
                                 NULL);
            SendMessage(label, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            label = CreateWindow("static", acp(MSGTR_PREFERENCES_FRAME_OSD_Level),
                                 WS_CHILD | WS_VISIBLE,
                                 10, 286, 115, 15, hwnd,
                                 NULL, ((LPCREATESTRUCT) lParam) -> hInstance,
                                 NULL);
            SendMessage(label, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            label = CreateWindow("static", acp(MSGTR_PREFERENCES_Priority),
                                 WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 200, 286, 100, 15, hwnd,
                                 NULL, ((LPCREATESTRUCT) lParam) -> hInstance,
                                 NULL);
            SendMessage(label, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            vo_driver = CreateWindow("combobox", NULL,
                                     CBS_DROPDOWNLIST | CB_SHOWDROPDOWN |
                                     CBS_NOINTEGRALHEIGHT | CBS_HASSTRINGS |
                                     WS_CHILD | WS_VISIBLE |
                                     WS_VSCROLL | WS_TABSTOP,
                                     80, 10, 100, 160, hwnd,
                                     (HMENU) ID_VO_DRIVER,
                                     ((LPCREATESTRUCT) lParam) -> hInstance,
                                     NULL);

            ao_driver = CreateWindow("combobox", NULL,
                                     CBS_DROPDOWNLIST | CB_SHOWDROPDOWN |
                                     CBS_NOINTEGRALHEIGHT | CBS_HASSTRINGS |
                                     WS_CHILD | WS_VISIBLE |
                                     WS_VSCROLL | WS_TABSTOP,
                                     275, 10, 100, 160, hwnd,
                                     (HMENU) ID_AO_DRIVER,
                                     ((LPCREATESTRUCT) lParam) -> hInstance,
                                     NULL);

            prio = CreateWindow("combobox", NULL,
                                CBS_DROPDOWNLIST | CB_SHOWDROPDOWN |
                                CBS_NOINTEGRALHEIGHT | CBS_HASSTRINGS |
                                WS_CHILD | WS_VISIBLE |
                                WS_VSCROLL | WS_TABSTOP,
                                310, 282, 100, 160, hwnd,
                                (HMENU) ID_PRIO,
                                ((LPCREATESTRUCT) lParam) -> hInstance,
                                NULL);

            /* checkboxes */
            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_DoubleBuffer),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               10, 35, 160, 25,
                               hwnd, (HMENU) ID_DOUBLE,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_DirectRender),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               10, 57, 160, 25,
                               hwnd, (HMENU) ID_DIRECT,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_FrameDrop),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               10, 79, 160, 25,
                               hwnd, (HMENU) ID_FRAMEDROP,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_NoIdle),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               10, 101, 225, 25,
                               hwnd, (HMENU) ID_IDLE,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_NormalizeSound),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               220, 35, 160, 25,
                               hwnd, (HMENU) ID_NORMALIZE,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_SoftwareMixer),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               220, 57, 160, 25,
                               hwnd, (HMENU) ID_SOFTMIX,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_ExtraStereo),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               220, 79, 160, 25,
                               hwnd, (HMENU) ID_EXTRASTEREO,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_Cache),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               10, 222, 90, 25,
                               hwnd, (HMENU) ID_CACHE,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_AutoSync),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               240, 222, 110, 25, hwnd,
                               (HMENU) ID_AUTOSYNC,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_ShowInVideoWin),
                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                               10, 249, 250, 25,
                               hwnd, (HMENU) ID_VIDEOWINDOW,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            /* osd level */
            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_OSD_LEVEL0),
                               WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                               15, 297, 200, 25, hwnd,
                               (HMENU) ID_NONE,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_OSD_LEVEL1),
                               WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                               15, 317, 395, 25, hwnd,
                               (HMENU) ID_OSD1,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_OSD_LEVEL2),
                               WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                               15, 337, 395, 25, hwnd,
                               (HMENU) ID_OSD2,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_PREFERENCES_OSD_LEVEL3),
                               WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                               15, 357, 395, 25, hwnd,
                               (HMENU) ID_OSD3,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_Ok),
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               248, 417, 80, 25, hwnd,
                               (HMENU) ID_APPLY,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_Cancel),
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               334, 417, 80, 25, hwnd,
                               (HMENU) ID_CANCEL,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            btn = CreateWindow("button", acp(MSGTR_Default),
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               5, 417, 80, 25, hwnd,
                               (HMENU) ID_DEFAULTS,
                               ((LPCREATESTRUCT) lParam) -> hInstance,
                               NULL);
            SendMessage(btn, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            /* extra stereo coefficient trackbar */
            track1 = CreateWindow(TRACKBAR_CLASS, "Coefficient",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                  WS_DISABLED | TBS_HORZ |
                                  TBS_BOTTOM | TBS_NOTICKS,
                                  165, 142, 245, 35, hwnd,
                                  (HMENU) ID_TRACKBAR1,
                                  ((LPCREATESTRUCT) lParam) -> hInstance,
                                  NULL);
            SendDlgItemMessage(hwnd, ID_TRACKBAR1, TBM_SETRANGE, 1, MAKELONG(-100, 100));

            /* audio delay */
            track2 = CreateWindow(TRACKBAR_CLASS, "Audio delay",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                  WS_DISABLED | TBS_HORZ |
                                  TBS_BOTTOM | TBS_NOTICKS,
                                  165, 182, 245, 35, hwnd,
                                  (HMENU) ID_TRACKBAR2,
                                  ((LPCREATESTRUCT) lParam) -> hInstance,
                                  NULL);
            SendDlgItemMessage(hwnd, ID_TRACKBAR2, TBM_SETRANGE, 1, MAKELONG(-1000, 1000));

            /* cache */
            edit1 = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "cache",
                                   WS_CHILD | WS_VISIBLE | WS_DISABLED |
                                   ES_LEFT | ES_AUTOHSCROLL,
                                   105, 225, 60, 20, hwnd,
                                   (HMENU) ID_EDIT1,
                                   ((LPCREATESTRUCT) lParam) -> hInstance,
                                   NULL);
            SendMessage(edit1, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            updown1 = CreateUpDownControl(WS_CHILD | WS_VISIBLE |
                                          WS_DISABLED | UDS_SETBUDDYINT |
                                          UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                          165, 225, 20, 20, hwnd,
                                          ID_UPDOWN1,
                                          ((LPCREATESTRUCT) lParam) -> hInstance,
                                          (HWND)edit1, 0, 0, 0);
            SendDlgItemMessage(hwnd, ID_UPDOWN1, UDM_SETRANGE32, (WPARAM)32, (LPARAM)0x7fffffff);

            /* autosync */
            edit2 = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "autosync",
                                   WS_CHILD | WS_VISIBLE | WS_DISABLED |
                                   ES_LEFT | ES_AUTOHSCROLL,
                                   355, 225, 40, 20, hwnd,
                                   (HMENU) ID_EDIT2,
                                   ((LPCREATESTRUCT) lParam) -> hInstance,
                                   NULL);
            SendMessage(edit2, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            updown2 = CreateUpDownControl(WS_CHILD | WS_VISIBLE |
                                          WS_DISABLED | UDS_SETBUDDYINT |
                                          UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                          395, 225, 20, 20, hwnd,
                                          ID_UPDOWN2,
                                          ((LPCREATESTRUCT) lParam) -> hInstance,
                                          (HWND)edit2, 0, 0, 0);
            SendDlgItemMessage(hwnd, ID_UPDOWN2, UDM_SETRANGE32, (WPARAM)0, (LPARAM)10000);

            while(video_out_drivers[i])
            {
                const vo_info_t *info = video_out_drivers[i++]->info;
                if(!video_driver_list) listSet(&video_driver_list, (char *)info->short_name);
                    SendDlgItemMessage(hwnd, ID_VO_DRIVER, CB_ADDSTRING, 0, (LPARAM) info->short_name);
            }
            /* Special case for directx:noaccel */
            SendDlgItemMessage(hwnd, ID_VO_DRIVER, CB_ADDSTRING, 0, (LPARAM) "directx:noaccel");
            SendMessage(vo_driver, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            while(audio_out_drivers[j])
            {
                const ao_info_t *info = audio_out_drivers[j++]->info;
                if(!audio_driver_list)
                {
                    // FIXME: default priority (i.e. order in audio_out_drivers) should be fixed instead
                    // if win32 as default is really desirable
                    listSet(&audio_driver_list, "win32"/*(char *)info->short_name*/);
                }
                SendDlgItemMessage(hwnd, ID_AO_DRIVER, CB_ADDSTRING, 0, (LPARAM) info->short_name);
            }
            SendMessage(ao_driver, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            /* priority list, i'm leaving out realtime for safety's sake */
            SendDlgItemMessage(hwnd, ID_PRIO, CB_INSERTSTRING, 0, (LPARAM) acp(MSGTR_PREFERENCES_PriorityLow));
            SendDlgItemMessage(hwnd, ID_PRIO, CB_INSERTSTRING, 0, (LPARAM) acp(MSGTR_PREFERENCES_PriorityNormalBelow));
            SendDlgItemMessage(hwnd, ID_PRIO, CB_INSERTSTRING, 0, (LPARAM) acp(MSGTR_PREFERENCES_PriorityNormal));
            SendDlgItemMessage(hwnd, ID_PRIO, CB_INSERTSTRING, 0, (LPARAM) acp(MSGTR_PREFERENCES_PriorityNormalAbove));
            SendDlgItemMessage(hwnd, ID_PRIO, CB_INSERTSTRING, 0, (LPARAM) acp(MSGTR_PREFERENCES_PriorityHigh));
            SendMessage(prio, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);

            /* set our preferences on what we already have */
            if(video_driver_list)
                SendDlgItemMessage(hwnd, ID_VO_DRIVER, CB_SETCURSEL,
                                   (WPARAM)SendMessage(vo_driver, CB_FINDSTRING, -1,
                                   (LPARAM)video_driver_list[0]), 0);

            if(audio_driver_list)
                SendDlgItemMessage(hwnd, ID_AO_DRIVER, CB_SETCURSEL,
                                   (WPARAM)SendMessage(ao_driver, CB_FINDSTRING, -1,
                                   (LPARAM)audio_driver_list[0]), 0);

            if(vo_doublebuffering)
                SendDlgItemMessage(hwnd, ID_DOUBLE, BM_SETCHECK, 1, 0);
            if(vo_directrendering)
                SendDlgItemMessage(hwnd, ID_DIRECT, BM_SETCHECK, 1, 0);
            if(!player_idle_mode)
                SendDlgItemMessage(hwnd, ID_IDLE, BM_SETCHECK, 1, 0);
            if(frame_dropping)
                SendDlgItemMessage(hwnd, ID_FRAMEDROP, BM_SETCHECK, 1, 0);
            if(gtkAONorm)
                SendDlgItemMessage(hwnd, ID_NORMALIZE, BM_SETCHECK, 1, 0);
            if(soft_vol)
                SendDlgItemMessage(hwnd, ID_SOFTMIX, BM_SETCHECK, 1, 0);
            if(gtkAOExtraStereo)
            {
                SendDlgItemMessage(hwnd, ID_EXTRASTEREO, BM_SETCHECK, 1, 0);
                if(!guiInfo.Playing)
                {
                    EnableWindow(track1, 1);
                    EnableWindow(track2, 1);
                }
            }
            else gtkAOExtraStereoMul = 1.0;
            SendDlgItemMessage(hwnd, ID_TRACKBAR1, TBM_SETPOS, 1, (LPARAM)stereopos);

            if(audio_delay)
                SendDlgItemMessage(hwnd, ID_TRACKBAR2, TBM_SETPOS, 1, (LPARAM)delaypos);

            if(gtkCacheOn) {
                SendDlgItemMessage(hwnd, ID_CACHE, BM_SETCHECK, 1, 0);
                EnableWindow(edit1, 1);
                EnableWindow(updown1, 1);
            }
            else gtkCacheSize = 2048;
            SendDlgItemMessage(hwnd, ID_UPDOWN1, UDM_SETPOS32, 0, (LPARAM)gtkCacheSize);

            if(gtkAutoSyncOn) {
                SendDlgItemMessage(hwnd, ID_AUTOSYNC, BM_SETCHECK, 1, 0);
                EnableWindow(edit2, 1);
                EnableWindow(updown2, 1);
            }
            else gtkAutoSync = 0;
            SendDlgItemMessage(hwnd, ID_UPDOWN2, UDM_SETPOS32, 0, (LPARAM)gtkAutoSync);

            if(video_window)
                SendDlgItemMessage(hwnd, ID_VIDEOWINDOW, BM_SETCHECK, 1, 0);

            if(!osd_level)
                SendDlgItemMessage(hwnd, ID_NONE, BM_SETCHECK, 1, 0);
            else if(osd_level == 1)
                SendDlgItemMessage(hwnd, ID_OSD1, BM_SETCHECK, 1, 0);
            else if(osd_level == 2)
                SendDlgItemMessage(hwnd, ID_OSD2, BM_SETCHECK, 1, 0);
            else if(osd_level == 3)
                SendDlgItemMessage(hwnd, ID_OSD3, BM_SETCHECK, 1, 0);

            if(proc_priority)
                SendDlgItemMessage(hwnd, ID_PRIO, CB_SETCURSEL,
                                   (WPARAM)SendMessage(prio, CB_FINDSTRING, -1,
                                   (LPARAM)proc_priority), 0);

            else SendDlgItemMessage(hwnd, ID_PRIO, CB_SETCURSEL, 2, 0);

            break;
        }
        case WM_CTLCOLORDLG:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            return (INT_PTR)SOLID_GREY;
        }
        break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_EXTRASTEREO:
                {
                    if(SendDlgItemMessage(hwnd, ID_EXTRASTEREO, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        EnableWindow(GetDlgItem(hwnd, ID_TRACKBAR1), 1);
                        EnableWindow(GetDlgItem(hwnd, ID_TRACKBAR2), 1);
                    } else {
                        EnableWindow(GetDlgItem(hwnd, ID_TRACKBAR1), 0);
                        EnableWindow(GetDlgItem(hwnd, ID_TRACKBAR2), 0);
                        SendDlgItemMessage(hwnd, ID_TRACKBAR1, TBM_SETPOS, 1, (LPARAM)10.0);
                        SendDlgItemMessage(hwnd, ID_TRACKBAR2, TBM_SETPOS, 1, (LPARAM)0);
                    }
                    break;
                }
                case ID_CACHE:
                {
                    if(SendDlgItemMessage(hwnd, ID_CACHE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        EnableWindow(GetDlgItem(hwnd, ID_EDIT1), 1);
                        EnableWindow(GetDlgItem(hwnd, ID_UPDOWN1), 1);
                    } else {
                        EnableWindow(GetDlgItem(hwnd, ID_EDIT1), 0);
                        EnableWindow(GetDlgItem(hwnd, ID_UPDOWN1), 0);
                        SendDlgItemMessage(hwnd, ID_UPDOWN1, UDM_SETPOS32, 1, (LPARAM)2048);
                    }
                    break;
                }
                case ID_AUTOSYNC:
                {
                    if(SendDlgItemMessage(hwnd, ID_AUTOSYNC, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        EnableWindow(GetDlgItem(hwnd, ID_EDIT2), 1);
                        EnableWindow(GetDlgItem(hwnd, ID_UPDOWN2), 1);
                    } else {
                        EnableWindow(GetDlgItem(hwnd, ID_EDIT2), 0);
                        EnableWindow(GetDlgItem(hwnd, ID_UPDOWN2), 0);
                        SendDlgItemMessage(hwnd, ID_UPDOWN2, UDM_SETPOS32, 1, (LPARAM)0);
                    }
                    break;
                }
                case ID_DEFAULTS:
                {
                    set_defaults();
                    SendDlgItemMessage(hwnd, ID_VO_DRIVER, CB_SETCURSEL,
                                       (WPARAM)SendMessage(vo_driver, CB_FINDSTRING, -1, (LPARAM)"directx"), 0);

                    SendDlgItemMessage(hwnd, ID_AO_DRIVER, CB_SETCURSEL,
                                       (WPARAM)SendMessage(ao_driver, CB_FINDSTRING, -1, (LPARAM)"dsound"), 0);

                    SendDlgItemMessage(hwnd, ID_PRIO, CB_SETCURSEL,
                                       (WPARAM)SendMessage(prio, CB_FINDSTRING, -1, (LPARAM)proc_priority), 0);

                    SendDlgItemMessage(hwnd, ID_TRACKBAR1, TBM_SETPOS, 1, (LPARAM)10.0);
                    SendDlgItemMessage(hwnd, ID_TRACKBAR2, TBM_SETPOS, 1, (LPARAM)0.0);
                    SendDlgItemMessage(hwnd, ID_UPDOWN1, UDM_SETPOS32, 0, (LPARAM)gtkCacheSize);
                    SendDlgItemMessage(hwnd, ID_UPDOWN2, UDM_SETPOS32, 0, (LPARAM)gtkAutoSync);
                    SendDlgItemMessage(hwnd, ID_DOUBLE, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_DIRECT, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_IDLE, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_FRAMEDROP, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_NORMALIZE, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_SOFTMIX, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_EXTRASTEREO, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_CACHE, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_AUTOSYNC, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_VIDEOWINDOW, BM_SETCHECK, 1, 0);
                    SendDlgItemMessage(hwnd, ID_NONE, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_OSD1, BM_SETCHECK, 1, 0);
                    SendDlgItemMessage(hwnd, ID_OSD2, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hwnd, ID_OSD3, BM_SETCHECK, 0, 0);
                    SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_APPLY, 0);
                    break;
                }
                case ID_CANCEL:
                    DestroyWindow(hwnd);
                    return 0;
                case ID_APPLY:
                {
                    int strl;
                    if(guiInfo.Playing) gui(GUI_SET_STATE, (void *)GUI_STOP);

                    /* Set the video driver */
                    free(video_driver_list[0]);
                    strl = SendMessage(vo_driver, CB_GETCURSEL, 0, 0);
                    video_driver_list[0] = malloc(strl);
                    SendMessage(vo_driver, CB_GETLBTEXT, (WPARAM)strl,
                                (LPARAM)video_driver_list[0]);

                    /* Set the audio driver */
                    free(audio_driver_list[0]);
                    strl = SendMessage(ao_driver, CB_GETCURSEL, 0, 0);
                    audio_driver_list[0] = malloc(strl);
                    SendMessage(ao_driver, CB_GETLBTEXT, (WPARAM)strl,
                                (LPARAM)audio_driver_list[0]);

                    /* Set the priority level */
                    SendMessage(prio, CB_GETLBTEXT, (WPARAM)SendMessage(prio, CB_GETCURSEL, 0, 0), (LPARAM)procprio);
                    proc_priority = strdup(procprio);

                    /* double buffering */
                    if(SendDlgItemMessage(hwnd, ID_DOUBLE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        vo_doublebuffering = 1;
                    else vo_doublebuffering = 0;

                    /* direct rendering */
                    if(SendDlgItemMessage(hwnd, ID_DIRECT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        vo_directrendering = 1;
                    else vo_directrendering = 0;

                    /* quit after playing */
                    if(SendDlgItemMessage(hwnd, ID_IDLE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        player_idle_mode = 0;
                    else player_idle_mode = 1;

                    /* frame dropping */
                    if(SendDlgItemMessage(hwnd, ID_FRAMEDROP, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        frame_dropping = 1;
                    else frame_dropping = 0;

                    /* normalize */
                    if(SendDlgItemMessage(hwnd, ID_NORMALIZE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        gtkAONorm = 1;
                    else gtkAONorm = 0;

                    /* software mixer */
                    if(SendDlgItemMessage(hwnd, ID_SOFTMIX, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        soft_vol = 1;
                    else soft_vol = 0;

                    /* extra stereo */
                    if(SendDlgItemMessage(hwnd, ID_EXTRASTEREO, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        gtkAOExtraStereo = 1;
                    else {
                        gtkAOExtraStereo = 0;
                        gtkAOExtraStereoMul = 10.0;
                    }
                    gtkAOExtraStereoMul = SendDlgItemMessage(hwnd, ID_TRACKBAR1, TBM_GETPOS, 0, 0) / 10.0;

                    /* audio delay */
                    audio_delay = SendDlgItemMessage(hwnd, ID_TRACKBAR2, TBM_GETPOS, 0, 0) / 100.0;

                    /* cache */
                    if(SendDlgItemMessage(hwnd, ID_CACHE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        gtkCacheOn = 1;
                    else gtkCacheOn = 0;
                    gtkCacheSize = SendDlgItemMessage(hwnd, ID_UPDOWN1, UDM_GETPOS32, 0, 0);

                    /* autosync */
                    if(SendDlgItemMessage(hwnd, ID_AUTOSYNC, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        gtkAutoSyncOn = 1;
                    else gtkAutoSyncOn = 0;
                    gtkAutoSync = SendDlgItemMessage(hwnd, ID_UPDOWN2, UDM_GETPOS32, 0, 0);

                    /* video window */
                    if(SendDlgItemMessage(hwnd, ID_VIDEOWINDOW, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        video_window = 1;
                    else video_window = 0;

                    /* osd level */
                    if(SendDlgItemMessage(hwnd, ID_NONE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        osd_level = 0;
                    else if(SendDlgItemMessage(hwnd, ID_OSD1, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        osd_level = 1;
                    else if(SendDlgItemMessage(hwnd, ID_OSD2, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        osd_level = 2;
                    else if(SendDlgItemMessage(hwnd, ID_OSD3, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        osd_level = 3;

                    MessageBox(hwnd, acp(MSGTR_PREFERENCES_Message), acp(MSGTR_MSGBOX_LABEL_Warning), MB_OK);
                    DestroyWindow(hwnd);
                    break;
                }
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void display_prefswindow(gui_t *gui)
{
    HWND hWnd;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc;
    int x, y;
    if(FindWindow(NULL, acp(MSGTR_Preferences))) return;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = PrefsWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hIcon         = gui->icon;
    wc.hbrBackground = SOLID_GREY;
    wc.lpszClassName = acp(MSGTR_Preferences);
    wc.lpszMenuName  = NULL;
    RegisterClass(&wc);
    x = (GetSystemMetrics(SM_CXSCREEN) / 2) - (425 / 2);
    y = (GetSystemMetrics(SM_CYSCREEN) / 2) - (474 / 2);
    hWnd = CreateWindow(acp(MSGTR_Preferences),
                        acp(MSGTR_Preferences),
                        WS_POPUPWINDOW | WS_CAPTION,
                        x,
                        y,
                        425,
                        474,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);
   SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) gui);
   ShowWindow(hWnd, SW_SHOW);
   UpdateWindow(hWnd);
}
