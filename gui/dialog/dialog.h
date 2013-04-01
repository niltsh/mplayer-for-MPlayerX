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

#include <gtk/gtk.h>
#include <X11/Xlib.h>

#define GTK_MB_SIMPLE 0
#define GTK_MB_MODAL 1
#define GTK_MB_FATAL 2
#define GTK_MB_ERROR 4
#define GTK_MB_WARNING 8

typedef struct {
    Pixmap small;
    Pixmap small_mask;
    Pixmap normal;
    Pixmap normal_mask;
    int collection_size;
    long *collection;
} guiIcon_t;

extern guiIcon_t guiIcon;

void gtkAddIcon(GtkWidget *window);
void gtkEventHandling(void);
int gtkFillSkinList(gchar *dir);
void gtkInit(char *display_name);
void gtkMessageBox(int type, const gchar *str);
void gtkRaise(GtkWidget *window);
void gtkSetLayer(GtkWidget *window);
void gtkShow(int type, char *param);

#endif /* MPLAYER_GUI_DIALOG_H */
