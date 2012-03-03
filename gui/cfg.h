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

#ifndef MPLAYER_GUI_CFG_H
#define MPLAYER_GUI_CFG_H

#include "m_config.h"
#include "m_option.h"

extern m_config_t *gui_conf;

extern int gtkCacheOn;
extern int gtkCacheSize;

extern int gtkVfPP;
extern int gtkVfLAVC;

extern char *gtkDXR3Device;

extern int gtkAutoSyncOn;
extern int gtkAutoSync;

extern int gtkAONorm;
extern int gtkAOSurround;
extern int gtkAOExtraStereo;
extern float gtkAOExtraStereoMul;

extern char *gtkAOALSAMixer;
extern char *gtkAOALSAMixerChannel;
extern char *gtkAOALSADevice;

extern char *gtkAOOSSMixer;
extern char *gtkAOOSSMixerChannel;
extern char *gtkAOOSSDevice;

extern char *gtkAOESDDevice;

extern char *gtkAOSDLDriver;

extern int gtkEnableAudioEqualizer;
extern char *gtkEquChannel1;
extern char *gtkEquChannel2;
extern char *gtkEquChannel3;
extern char *gtkEquChannel4;
extern char *gtkEquChannel5;
extern char *gtkEquChannel6;
extern float gtkEquChannels[6][10];

extern int gtkSubDumpMPSub;
extern int gtkSubDumpSrt;

typedef struct {
    int enabled;
    int use_margins;
    int top_margin;
    int bottom_margin;
} gtkASS_t;

extern gtkASS_t gtkASS;

extern int gtkEnablePlayBar;
extern int gtkLoadFullscreen;
extern int gtkShowVideoWindow;

extern int gui_save_pos;
extern int gui_main_pos_x;
extern int gui_main_pos_y;
extern int gui_video_pos_x;
extern int gui_video_pos_y;

extern int guiWinID;

extern char *skinName;

extern char *fsHistory[5];

int cfg_gui_include(m_option_t *conf, const char *filename);
void cfg_read(void);
void cfg_write(void);

#endif /* MPLAYER_GUI_CFG_H */
