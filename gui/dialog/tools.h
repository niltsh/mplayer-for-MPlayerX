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

#ifndef MPLAYER_GUI_TOOLS_H
#define MPLAYER_GUI_TOOLS_H

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

GtkWidget * gtkAddDialogFrame( GtkWidget * parent );
GtkWidget * gtkAddFrame( const char * title, int type, GtkWidget * parent, int add );
GtkWidget * gtkAddLabel( const char * title, GtkWidget * parent );
GtkWidget * gtkAddVBox( GtkWidget * parent, int type );
GtkWidget * gtkAddHBox( GtkWidget * parent, int type );
GtkWidget * gtkAddCheckButton( const char * title, GtkWidget * parent );
GtkWidget * gtkAddRadioButton( const char * title, GSList ** group, GtkWidget * parent );
GtkWidget * gtkAddSpinButton( const char * title, GtkAdjustment * adj, GtkWidget * parent );
GtkWidget * gtkAddButton( const char * title, GtkWidget * parent );
GtkWidget * gtkAddHSeparator( GtkWidget * parent );
GtkWidget * gtkAddHButtonBox( GtkWidget * parent );
GtkWidget * gtkAddHScale( GtkAdjustment * adj, GtkWidget * parent, int digit );
GtkWidget * gtkAddVScale( GtkAdjustment * adj, GtkWidget * parent, int digit );
GtkWidget * gtkAddCombo( GtkWidget * parent );

#endif /* MPLAYER_GUI_TOOLS_H */
