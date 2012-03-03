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

#include "config.h"

#if defined(CONFIG_LIBCDIO)
#include <cdio/cdda.h>
#elif defined(CONFIG_CDDA)
#include <cdda_interface.h>
#endif

#include <windows.h>

#if defined(__CYGWIN__) || defined(__WINE__)
#define _beginthreadex CreateThread
#ifdef __WINE__
#include <winioctl.h>
#define WINE_MOUNTMGR_EXTENSIONS
#include <ddk/mountmgr.h>
#endif
#else
#include <process.h>
#endif

#include "path.h"
#include "gui/interface.h"
#include "m_option.h"
#include "mixer.h"
#include "mp_msg.h"
#include "help_mp.h"
#include "codec-cfg.h"
#include "stream/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#ifdef CONFIG_DVDREAD
#include "stream/stream_dvd.h"
#endif
#include "input/input.h"
#include "libvo/video_out.h"
#include "libao2/audio_out.h"
#include "access_mpcontext.h"
#include "libmpcodecs/vd.h"
#include "libmpcodecs/dec_audio.h"
#include "gui/ui/actions.h"
#include "gui/ui/gmplayer.h"
#include "gui/util/mem.h"
#include "gui/util/list.h"
#include "gui/util/string.h"
#include "mp_core.h"
#include "mpcommon.h"
#include "gui.h"
#include "dialogs.h"

#define SAME_STREAMTYPE (STREAMTYPE_DUMMY - 1)

int guiWinID = 0;

char *skinName = NULL;
char *codecname = NULL;
int uiGotoTheNext = 1;
static gui_t *mygui = NULL;
static int update_videowindow(void);
static RECT old_rect;
static DWORD style;
static HANDLE hThread;
static unsigned threadId;
const ao_functions_t *audio_out = NULL;
const vo_functions_t *video_out = NULL;
mixer_t *mixer = NULL;

#ifdef __WINE__
/**
 * @brief Convert a Windows style path to a file name into an Unix style one.
 *
 * @param filename pointer to the file path to be converted
 *
 * @return pointer to the converted file path
 */
static char *unix_name (char *filename)
{
    static char *unix_filename;
    LPSTR (*CDECL wine_get_unix_file_name_ptr)(LPCWSTR);
    int wchar_conv;

    if (*filename && (filename[1] == ':'))
    {
        wine_get_unix_file_name_ptr = (void *) GetProcAddress(GetModuleHandleA("KERNEL32"), "wine_get_unix_file_name");
        wchar_conv = MultiByteToWideChar(CP_UNIXCP, 0, filename, -1, NULL, 0);

        if (wine_get_unix_file_name_ptr && wchar_conv)
        {
            WCHAR *ntpath;
            char *unix_name;

            ntpath = HeapAlloc(GetProcessHeap(), 0, sizeof(*ntpath) * (wchar_conv + 1));
            MultiByteToWideChar(CP_UNIXCP, 0, filename, -1, ntpath, wchar_conv);
            unix_name = wine_get_unix_file_name_ptr(ntpath);
            setdup(&unix_filename, unix_name);
            filename = unix_filename;
            HeapFree(GetProcessHeap(), 0, unix_name);
            HeapFree(GetProcessHeap(), 0, ntpath);
        }
    }

    return filename;
}

/**
 * @brief Convert a Windows style device name into an Unix style one.
 *
 * @param device pointer to the device name to be converted
 *
 * @return pointer to the converted device name
 */
static char *unix_device (char *device)
{
    static char *unix_devname;
    HANDLE mgr;
    DWORD size = 1024;

    mgr = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

    if (mgr != INVALID_HANDLE_VALUE)
    {
        struct mountmgr_unix_drive input;
        struct mountmgr_unix_drive *data;

        data = HeapAlloc(GetProcessHeap(), 0, size);

        if (data)
        {
            memset(&input, 0, sizeof(input));
            input.letter = *device;

            if (DeviceIoControl(mgr, IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE, &input, sizeof(input), data, size, NULL, NULL))
            {
                if (data->device_offset)
                {
                    setdup(&unix_devname, (char *) data + data->device_offset);
                    device = unix_devname;
                }
            }

            HeapFree(GetProcessHeap(), 0, data);
        }

        CloseHandle(mgr);
    }

    return device;
}
#endif

/* test for playlist files, no need to specify -playlist on the commandline.
 * add any conceivable playlist extensions here.
 * - Erik
 */
int parse_filename(char *file, play_tree_t *playtree, m_config_t *mconfig, int clear)
{
    if(clear)
        mygui->playlist->clear_playlist(mygui->playlist);

    if(strstr(file, ".m3u") || strstr(file, ".pls"))
    {
        playtree = parse_playlist_file(file);
        guiPlaylistAdd(playtree, mconfig);
        return 1;
    }
    return 0;
}

/* this function gets called by the gui to update mplayer */
static void guiSetEvent(int event)
{
    if(guiInfo.mpcontext)
        mixer = mpctx_get_mixer(guiInfo.mpcontext);

    switch(event)
    {
        case evPlay:
        case evPlaySwitchToPause:
        case evPauseSwitchToPlay:
            uiPlay();
            break;
        case evPause:
            uiPause();
            break;
#ifdef CONFIG_DVDREAD
        case evPlayDVD:
        {
            static char dvdname[MAX_PATH];
            guiInfo.Track = 1;
            guiInfo.Chapter = 1;
            guiInfo.Angle = 1;
            guiInfo.NewPlay = GUI_FILE_SAME;

#ifdef __WINE__
            // dvd_device is in the Windows style (D:\), which needs to be
            // converted for MPlayer, so that it will find the device in the
            // Linux filesystem.
            dvd_device = unix_device(dvd_device);
#endif
            uiSetFileName(NULL, dvd_device, STREAMTYPE_DVD);
            dvdname[0] = 0;
            strcat(dvdname, "DVD Movie");
            GetVolumeInformation(dvd_device, dvdname, MAX_PATH, NULL, NULL, NULL, NULL, 0);
            capitalize(dvdname);
            mp_msg(MSGT_GPLAYER, MSGL_V, "Opening DVD %s -> %s\n", dvd_device, dvdname);
            mygui->playlist->clear_playlist(mygui->playlist);
            mygui->playlist->add_track(mygui->playlist, filename, NULL, dvdname, 0);
            uiPlay();
            break;
        }
#endif
#ifdef CONFIG_CDDA
        case evPlayCD:
        {
            int i;
            char track[10];
            char trackname[10];
#ifdef CONFIG_LIBCDIO
            cdrom_drive_t *cd;
#else
            cdrom_drive *cd;
#endif
            int i_tracks;

#ifdef __WINE__
            // cdrom_device is in the Windows style (D:\), which needs to be
            // converted for MPlayer, so that it will find the device in the
            // Linux filesystem.
            cdrom_device = unix_device(cdrom_device);
#endif
            cd = cdda_identify(cdrom_device, 0, NULL);
            if (cd)
            {
                if (cdda_open(cd) != 0)
                {
                    cdda_close(cd);
                    cd = NULL;
                }
            }
            if(!cd)
            {
                printf("Couldn't find a driver.\n");
                break;
            }
            i_tracks = cdda_tracks(cd);

            mygui->playlist->clear_playlist(mygui->playlist);
            for(i=0;i<i_tracks;i++)
            {
                sprintf(track, "cdda://%d", i+1);
                sprintf(trackname, "Track %d", i+1);
                mygui->playlist->add_track(mygui->playlist, track, NULL, trackname, 0);
            }
            cdda_close(cd);
            mygui->startplay(mygui);
            break;
        }
#endif
        case evFullScreen:
            mp_input_queue_cmd(mp_input_parse_cmd("vo_fullscreen"));
            break;
        case evExit:
        {
            /* We are asking mplayer to exit, later it will ask us after uninit is made
               this should be the only safe way to quit */
            mygui->activewidget = NULL;
            mp_input_queue_cmd(mp_input_parse_cmd("quit"));
            break;
        }
        case evStop:
            if(guiInfo.Playing)
                gui(GUI_SET_STATE, (void *) GUI_STOP);
            break;
        case evSetMoviePosition:
        {
            rel_seek_secs = guiInfo.Position / 100.0f;
            abs_seek_pos = 3;
            break;
        }
        case evForward10sec:
        {
            rel_seek_secs = 10.0f;
            abs_seek_pos = 0;
            break;
        }
        case evBackward10sec:
        {
            rel_seek_secs = -10.0f;
            abs_seek_pos = 0;
            break;
        }
        case evSetBalance:
        case evSetVolume:
        {
            float l,r;

            if (guiInfo.Playing == GUI_STOP)
                break;

            if (guiInfo.Balance == 50.0f)
                mixer_setvolume(mixer, guiInfo.Volume, guiInfo.Volume);

            l = guiInfo.Volume * ((100.0f - guiInfo.Balance) / 50.0f);
            r = guiInfo.Volume * ((guiInfo.Balance) / 50.0f);

            if (l > guiInfo.Volume) l=guiInfo.Volume;
            if (r > guiInfo.Volume) r=guiInfo.Volume;
            mixer_setvolume(mixer, l, r);
            /* Check for balance support on mixer - there is a better way ?? */
            if (r != l)
            {
                mixer_getvolume(mixer, &l, &r);
                if (r == l)
                {
                    mp_msg(MSGT_GPLAYER, MSGL_V, "[GUI] Mixer doesn't support balanced audio\n");
                    mixer_setvolume(mixer, guiInfo.Volume, guiInfo.Volume);
                    guiInfo.Balance = 50.0f;
                }
            }
            break;
        }
        case evMute:
        {
            mp_cmd_t * cmd = calloc(1, sizeof(*cmd));
            cmd->id=MP_CMD_MUTE;
            cmd->name=strdup("mute");
            mp_input_queue_cmd(cmd);
            break;
        }
        case evLoadPlay:
        {
            switch(guiInfo.StreamType)
            {
                case STREAMTYPE_DVD:
                {
                    guiInfo.NewPlay = GUI_FILE_SAME;
                    gui(GUI_SET_STATE, (void *) GUI_PLAY);
                    break;
                }
                default:
                {
                    guiInfo.NewPlay = GUI_FILE_NEW;
                    update_playlistwindow();
                    uiGotoTheNext = guiInfo.Playing? 0 : 1;
                    gui(GUI_SET_STATE, (void *) GUI_STOP);
                    gui(GUI_SET_STATE, (void *) GUI_PLAY);
                    break;
               }
           }
           break;
        }
        case evNext:
            uiNext();
            break;
        case evPrev:
            uiPrev();
            break;
    }
}

void uiPlay( void )
{
   if((!guiInfo.Filename ) || (guiInfo.Filename[0] == 0))
     return;

   if(guiInfo.Playing > GUI_STOP)
   {
       uiPause();
       return;
   }
   guiInfo.NewPlay = GUI_FILE_NEW;
   gui(GUI_SET_STATE, (void *) GUI_PLAY);
}

void uiPause( void )
{
   if(!guiInfo.Playing) return;

   if(guiInfo.Playing == GUI_PLAY)
   {
       mp_cmd_t * cmd = calloc(1, sizeof(*cmd));
       cmd->id=MP_CMD_PAUSE;
       cmd->name=strdup("pause");
       mp_input_queue_cmd(cmd);
   } else guiInfo.Playing = GUI_PLAY;
}

void uiNext(void)
{
    if(guiInfo.Playing == GUI_PAUSE) return;
    switch(guiInfo.StreamType)
    {
        case STREAMTYPE_DVD:
            if(guiInfo.Chapter == (guiInfo.Chapters - 1))
                return;
            guiInfo.Chapter++;
            break;
        default:
            if(mygui->playlist->current == (mygui->playlist->trackcount - 1))
                return;
            uiSetFileName(NULL, mygui->playlist->tracks[(mygui->playlist->current)++]->filename,
                           STREAMTYPE_FILE);
            break;
    }
    mygui->startplay(mygui);
}

void uiPrev(void)
{
    if(guiInfo.Playing == GUI_PAUSE) return;
    switch(guiInfo.StreamType)
    {
        case STREAMTYPE_DVD:
            if(guiInfo.Chapter == 1)
                return;
            guiInfo.Chapter--;
            break;
        default:
            if(mygui->playlist->current == 0)
                return;
            uiSetFileName(NULL, mygui->playlist->tracks[(mygui->playlist->current)--]->filename,
                           STREAMTYPE_FILE);
            break;
    }
    mygui->startplay(mygui);
}

void uiSetFileName(char *dir, char *name, int type)
{
    if(!name) return;
    if(!dir)
        setdup(&guiInfo.Filename, name);
    else
        setddup(&guiInfo.Filename, dir, name);

    filename = guiInfo.Filename;
#ifdef __WINE__
    // When the GUI receives the files to be played in guiPlaylistInitialize()
    // and guiPlaylistAdd(), it calls import_file_into_gui() where the call of
    // Wine's GetFullPathName() converts each file name into the Windows style
    // (C:\path\to\file), which needs to be reconverted for MPlayer, so that
    // it will find the filename in the Linux filesystem.
    filename = unix_name(filename);
#endif

    if (type != SAME_STREAMTYPE)
        guiInfo.StreamType = type;

    nfree(guiInfo.AudioFilename);
    nfree(guiInfo.SubtitleFilename);
}

void uiFullScreen( void )
{
    if(!guiInfo.sh_video) return;

    if(video_window)
    {
        if(!fullscreen && IsWindowVisible(mygui->videowindow) && !IsIconic(mygui->videowindow))
           GetWindowRect(mygui->videowindow, &old_rect);

        if(fullscreen)
        {
            fullscreen = 0;
            style = WS_OVERLAPPEDWINDOW | WS_SIZEBOX;
        } else {
            fullscreen = 1;
            style = WS_VISIBLE | WS_POPUP;
        }
        SetWindowLong(mygui->videowindow, GWL_STYLE, style);
        update_videowindow();
    }
    video_out->control(VOCTRL_FULLSCREEN, 0);
    if(video_window) ShowWindow(mygui->videowindow, SW_SHOW);
}

static unsigned __stdcall GuiThread(void* param)
{
    MSG msg;

    if(!skinName) skinName = strdup("Blue");
    if(!mygui) mygui = create_gui(get_path("skins"), guiSetEvent);
    if(!mygui) exit_player(EXIT_ERROR);

    if(autosync && autosync != gtkAutoSync)
    {
       gtkAutoSyncOn = 1;
       gtkAutoSync = autosync;
    }

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    fprintf(stderr, "[GUI] GUI thread terminated.\n");
    fflush(stderr);
    return 0;
}

void guiInit(void)
{
    memset(&guiInfo, 0, sizeof(guiInfo));
    /* Create The gui thread */
    if (!mygui)
    {
        hThread = _beginthreadex(NULL, 0, GuiThread, NULL, 0, &threadId);
        mp_msg(MSGT_GPLAYER, MSGL_V, "[GUI] Creating GUI Thread 0x%04x\n", threadId);
    }

    /* Wait until the gui is created */
    while(!mygui) Sleep(100);
    mp_msg(MSGT_GPLAYER, MSGL_V, "[GUI] GUI thread started.\n");
}

void guiDone(void)
{
    if(mygui)
    {
        fprintf(stderr, "[GUI] Closed by main mplayer window\n");
        fflush(stderr);
        PostThreadMessage(threadId, WM_QUIT, 0, 0);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        mygui->uninit(mygui);
        nfree(mygui);
    }
    /* Remove tray icon */
    Shell_NotifyIcon(NIM_DELETE, &nid);
    cfg_write();
}

/* this function gets called by mplayer to update the gui */
int gui(int what, void *data)
{
    stream_t *stream = data;
#ifdef CONFIG_DVDREAD
    dvd_priv_t *dvdp;
#endif
    if(!mygui || !mygui->skin) return 0;

    if(guiInfo.mpcontext)
    {
        audio_out = mpctx_get_audio_out(guiInfo.mpcontext);
        video_out = mpctx_get_video_out(guiInfo.mpcontext);
        mixer = mpctx_get_mixer(guiInfo.mpcontext);
        playtree = mpctx_get_playtree_iter(guiInfo.mpcontext);
    }

    switch (what)
    {
        case GUI_PREPARE:
        {
            audio_id = -1;
            video_id = -1;
            dvdsub_id = -1;
            vobsub_id = -1;
            stream_cache_size = -1;
            autosync = 0;
            dvd_title = 0;
            force_fps = 0;
            if(!mygui->playlist->tracks) return 0;
            switch(guiInfo.StreamType)
            {
                case STREAMTYPE_FILE:
                case STREAMTYPE_STREAM:
                    uiSetFileName(NULL, mygui->playlist->tracks[mygui->playlist->current]->filename, SAME_STREAMTYPE);
                    guiInfo.Track = mygui->playlist->current + 1;
                    break;
                case STREAMTYPE_DVD:
                {
                    char tmp[512];
                    dvd_chapter = guiInfo.Chapter;
                    dvd_angle = guiInfo.Angle;
                    sprintf(tmp,"dvd://%d", guiInfo.Track);
                    uiSetFileName(NULL, tmp, SAME_STREAMTYPE);
                    break;
                }
            }
            guiInfo.VideoWindow = 1;
            if(gtkAONorm) listRepl(&af_cfg.list, "volnorm", "volnorm");
            if(gtkAOExtraStereo)
            {
                char *name = malloc(12 + 20 + 1);
                snprintf(name, 12 + 20, "extrastereo=%f", gtkAOExtraStereoMul);
                name[12 + 20] = 0;
                listRepl(&af_cfg.list, "extrastereo", name);
                free(name);
            }
            if(gtkCacheOn) stream_cache_size = gtkCacheSize;
            if(gtkAutoSyncOn) autosync = gtkAutoSync;
            guiInfo.NewPlay = 0;
            break;
        }
        case GUI_SET_AUDIO:
        {
            if (data && !guiInfo.sh_video) guiInfo.VideoWindow = 0;
            if(IsWindowVisible(mygui->videowindow) && !guiInfo.VideoWindow)
                ShowWindow(mygui->videowindow, SW_HIDE);
            break;
        }
        case GUI_SET_CONTEXT:
            guiInfo.mpcontext = data;
            break;
        case GUI_SET_VIDEO:
        {
            guiInfo.sh_video = data;
            if (data)
            {
                sh_video_t *sh = data;
                codecname = sh->codec->name;

                /* we have video, show the video window */
                if(!IsWindowVisible(mygui->videowindow) || IsIconic(mygui->videowindow))
                    ShowWindow(mygui->videowindow, SW_SHOWNORMAL);
                if(WinID == -1)
                    update_videowindow();

            }
            break;
        }
        case GUI_SETUP_VIDEO_WINDOW:
        {
            guiInfo.VideoWidth = vo_dwidth;
            guiInfo.VideoHeight = vo_dheight;

            video_aspect = (float)guiInfo.VideoWidth/guiInfo.VideoHeight;
            if(WinID != -1)
               update_videowindow();
            break;
        }
        case GUI_SET_STREAM:
        {
            guiInfo.StreamType = stream->type;
            switch(stream->type)
            {
                case STREAMTYPE_DVD:
                    guiInfo.Tracks = 0;
                    stream_control(stream, STREAM_CTRL_GET_NUM_TITLES, &guiInfo.Tracks);
                    guiInfo.Chapters = 0;
                    stream_control(stream, STREAM_CTRL_GET_NUM_CHAPTERS, &guiInfo.Chapters);
                    guiInfo.Angles = 0;
                    stream_control(stream, STREAM_CTRL_GET_NUM_ANGLES, &guiInfo.Angles);
#ifdef CONFIG_DVDREAD
                    dvdp = stream->priv;
                    guiInfo.AudioStreams = dvdp->nr_of_channels;
                    memcpy(guiInfo.AudioStream, dvdp->audio_streams, sizeof(dvdp->audio_streams));
                    guiInfo.Subtitles = dvdp->nr_of_subtitles;
                    memcpy(guiInfo.Subtitle, dvdp->subtitles, sizeof(dvdp->subtitles));
#endif
                    guiInfo.Chapter = dvd_chapter + 1;
                    guiInfo.Angle = dvd_angle + 1;
                    guiInfo.Track = dvd_title + 1;
                    break;
            }
            break;
        }
        case GUI_REDRAW:
            mygui->updatedisplay(mygui, mygui->mainwindow);
            break;
        case GUI_SET_AFILTER:
            guiInfo.afilter = data;
            break;
        case GUI_SET_STATE:
        {
            guiInfo.Playing = (int) data;
            switch (guiInfo.Playing)
            {
                case GUI_PLAY:
                {
                    guiInfo.Playing = GUI_PLAY;
                    break;
                }
                case GUI_STOP:
                {
                    guiInfo.Playing = GUI_STOP;
                    if(movie_aspect >= 0)
                        movie_aspect = -1;
                    update_videowindow();
                    break;
                }
                case GUI_PAUSE:
                    guiInfo.Playing = GUI_PAUSE;
                    break;
            }
            break;
        }
        case GUI_RUN_COMMAND:
        {
            mp_msg(MSGT_GPLAYER,MSGL_V, "cmd: %d\n", (int) data);
            /* MPlayer asks us to quit */
            switch((int) data)
            {
                case MP_CMD_VO_FULLSCREEN:
                    uiFullScreen();
                    break;
                case MP_CMD_QUIT:
                {
                    mygui->uninit(mygui);
                    nfree(mygui);
                    exit_player(EXIT_QUIT);
                    return 1;
                }
                case MP_CMD_PLAY_TREE_STEP:
                  guiSetEvent(evNext);
                  break;
                case -MP_CMD_PLAY_TREE_STEP:
                  guiSetEvent(evPrev);
                  break;
                case MP_CMD_STOP:
                  guiSetEvent(evStop);
                  break;
                default:
                    break;
            }
            break;
        }
        case GUI_RUN_MESSAGE:
          break;
        case GUI_HANDLE_EVENTS:
          break;
        case GUI_SET_MIXER:
        {
            if(audio_out)
            {
                /* Some audio_out drivers do not support balance e.g. dsound */
                /* FIXME this algo is not correct */
                float l, r;
                mixer_getvolume(mixer, &l, &r);
                guiInfo.Volume = (r > l ? r : l); /* max(r,l) */
                if (r != l)
                    guiInfo.Balance = ((r-l) + 100.0f) * 0.5f;
                else
                    guiInfo.Balance = 50.0f;
            }
            break;
        }
        case GUI_END_FILE:
        {
          if(!uiGotoTheNext && guiInfo.Playing)
          {
              uiGotoTheNext = 1;
              break;
          }

          if(uiGotoTheNext && guiInfo.Playing &&
            (mygui->playlist->current < (mygui->playlist->trackcount - 1)) &&
            guiInfo.StreamType != STREAMTYPE_DVD &&
            guiInfo.StreamType != STREAMTYPE_DVDNAV)
          {
              /* we've finished this file, reset the aspect */
              if(movie_aspect >= 0)
                  movie_aspect = -1;

              uiGotoTheNext = 1;
              guiInfo.NewPlay = GUI_FILE_NEW;
              uiSetFileName(NULL, mygui->playlist->tracks[(mygui->playlist->current)++]->filename, STREAMTYPE_FILE);
              //sprintf(guiInfo.Filename, mygui->playlist->tracks[(mygui->playlist->current)++]->filename);
          }

          if(guiInfo.NewPlay == GUI_FILE_NEW)
              break;

          guiInfo.ElapsedTime = 0;
          guiInfo.Position = 0;
          guiInfo.AudioChannels = 0;

          guiInfo.Track = 1;
          guiInfo.Chapter = 1;
          guiInfo.Angle = 1;

          if (mygui->playlist->current == (mygui->playlist->trackcount - 1))
              mygui->playlist->current = 0;

          fullscreen = 0;
          if(style == WS_VISIBLE | WS_POPUP)
          {
              style = WS_OVERLAPPEDWINDOW | WS_SIZEBOX;
              SetWindowLong(mygui->videowindow, GWL_STYLE, style);
          }
          gui(GUI_SET_STATE, (void *) GUI_STOP);
          break;
        }
        default:
            mp_msg(MSGT_GPLAYER, MSGL_ERR, "[GUI] GOT UNHANDLED EVENT %i\n", what);
    }
    return 1;
}

/* This function adds/inserts one file into the gui playlist */
static int import_file_into_gui(char *pathname, int insert)
{
    char filename[MAX_PATH];
    char *filepart = filename;

    if (strstr(pathname, "://"))
    {
        mp_msg(MSGT_GPLAYER, MSGL_V, "[GUI] Adding special %s\n", pathname);
        mygui->playlist->add_track(mygui->playlist, pathname, NULL, NULL, 0);
        return 1;
    }
    if (GetFullPathName(pathname, MAX_PATH, filename, &filepart))
    {
        if (!(GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY))
        {
            mp_msg(MSGT_GPLAYER, MSGL_V, "[GUI] Adding filename: %s - fullpath: %s\n", filepart, filename);
            mygui->playlist->add_track(mygui->playlist, filename, NULL, filepart, 0);
            return 1;
        }
        else
            mp_msg(MSGT_GPLAYER, MSGL_V, "[GUI] Cannot add %s\n", filename);
    }

    return 0;
}

/*  This function imports the initial playtree (based on cmd-line files) into the gui playlist
    by either:
    - overwriting gui pl (enqueue=0) */

int guiPlaylistInitialize(play_tree_t *my_playtree, m_config_t *config, int enqueue)
{
    play_tree_iter_t *my_pt_iter = NULL;
    int result = 0;

    if(!mygui) guiInit();

    if((my_pt_iter = pt_iter_create(&my_playtree, config)))
    {
        while ((filename = pt_iter_get_next_file(my_pt_iter)) != NULL)
        {
            if (parse_filename(filename, my_playtree, config, 0))
                result = 1;
            else if (import_file_into_gui(filename, 0)) /* Add it to end of list */
                result = 1;
        }
    }
    uiGotoTheNext = 1;

    if (result)
    {
        mygui->playlist->current = 0;
        uiSetFileName(NULL, mygui->playlist->tracks[0]->filename, STREAMTYPE_FILE);
    }

    if (enqueue) filename = NULL;

    return result;
}

/* This function imports and inserts an playtree, that is created "on the fly", for example by
   parsing some MOV-Reference-File; or by loading an playlist with "File Open"
   The file which contained the playlist is thereby replaced with it's contents. */

int guiPlaylistAdd(play_tree_t *my_playtree, m_config_t *config)
{
    play_tree_iter_t *my_pt_iter = NULL;
    int result = 0;

    if((my_pt_iter = pt_iter_create(&my_playtree, config)))
    {
        while ((filename = pt_iter_get_next_file(my_pt_iter)) != NULL)
            if (import_file_into_gui(filename, 1)) /* insert it into the list and set plCurrent = new item */
                result = 1;
        pt_iter_destroy(&my_pt_iter);
    }
    return result;
}

static int update_videowindow(void)
{
    int x,y;
    RECT rd;
    WINDOWPOS wp;

    if(!video_window)
    {
        WinID = -1;

        if(IsWindowVisible(mygui->videowindow) && guiInfo.sh_video && guiInfo.Playing)
        {
            ShowWindow(mygui->videowindow, SW_HIDE);
            return 0;
        }
        else if(!guiInfo.VideoWindow)
            return 0;
        else ShowWindow(mygui->videowindow, SW_SHOW);
    }

    /* we've come out of fullscreen at the end of file */
    if((!IsWindowVisible(mygui->videowindow) || IsIconic(mygui->videowindow)) && guiInfo.VideoWindow)
        ShowWindow(mygui->videowindow, SW_SHOWNORMAL);

    /* get our current window coordinates */
    GetWindowRect(mygui->videowindow, &rd);

    x = rd.left;
    y = rd.top;

    /* restore video window position when coming out of fullscreen */
    if(x <= 0) x = old_rect.left;
    if(y <= 0) y = old_rect.top;

    if(!guiInfo.Playing)
    {
        window *desc = NULL;
        int i;

        for (i=0; i<mygui->skin->windowcount; i++)
            if(mygui->skin->windows[i]->type == wiVideo)
                desc = mygui->skin->windows[i];

        rd.right = rd.left+desc->base->bitmap[0]->width;
        rd.bottom = rd.top+desc->base->bitmap[0]->height;
        video_aspect = (float)(rd.right-rd.left)/(rd.bottom-rd.top);
    }
    else
    {
        rd.right = rd.left+guiInfo.VideoWidth;
        rd.bottom = rd.top+guiInfo.VideoHeight;

        if (movie_aspect > 0.0)       // forced aspect from the cmdline
            video_aspect = movie_aspect;
    }


    AdjustWindowRect(&rd, WS_OVERLAPPEDWINDOW | WS_SIZEBOX, 0);
    SetWindowPos(mygui->videowindow, 0, x, y, rd.right-rd.left, rd.bottom-rd.top, SWP_NOOWNERZORDER);

    wp.hwnd = mygui->videowindow;
    wp.x = rd.left;
    wp.y = rd.top;
    wp.cx = rd.right-rd.left;
    wp.cy = rd.bottom-rd.top;
    wp.flags = SWP_NOOWNERZORDER | SWP_SHOWWINDOW;

    /* erase the bitmap image if there's video */
    if(guiInfo.Playing != GUI_STOP && guiInfo.sh_video)
        SendMessage(mygui->videowindow, WM_ERASEBKGND, (WPARAM)GetDC(mygui->videowindow), 0);

    /* reset the window aspect */
    SendMessage(mygui->videowindow, WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp);
    return 0;
}
