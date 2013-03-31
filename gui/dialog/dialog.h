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

#ifndef MPLAYER_GUI_DIALOG_H
#define MPLAYER_GUI_DIALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "osdep/shmem.h"
#include "gui/ui/actions.h"
#include "mplayer.h"

#define GTK_MB_SIMPLE 0
#define GTK_MB_MODAL 1
#define GTK_MB_FATAL 2
#define GTK_MB_ERROR 4
#define GTK_MB_WARNING 8

extern GtkWidget *Options;

extern GtkWidget *WarningPixmap;
extern GtkWidget *ErrorPixmap;

extern GtkWidget *SkinList;
extern GtkWidget *gtkMessageBoxText;

extern int gtkPopupMenu;
extern int gtkPopupMenuParam;

extern char *sbMPlayerDirInHome;
extern char *sbMPlayerPrefixDir;

typedef struct {
    Pixmap small;
    Pixmap small_mask;
    Pixmap normal;
    Pixmap normal_mask;
    int collection_size;
    long *collection;
} guiIcon_t;

extern guiIcon_t guiIcon;

void widgetsCreate(void);

void gtkInit(char *display_name);
void gtkAddIcon(GtkWidget *window);

int gtkFillSkinList(gchar *dir);
int gtkFindCList(GtkWidget *list, char *item);

void gtkEventHandling(void);

void gtkShow(int type, char *param);
void gtkMessageBox(int type, const gchar *str);
void gtkSetLayer(GtkWidget *window);
void gtkRaise(GtkWidget *window);

#endif /* MPLAYER_GUI_DIALOG_H */
