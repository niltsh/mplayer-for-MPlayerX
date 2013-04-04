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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>

#include "tools.h"

GtkWidget * gtkAddDialogFrame( GtkWidget * parent )
{
 GtkWidget * frame;
 frame=gtkAddFrame( NULL,GTK_SHADOW_IN,parent,1 );
 gtk_container_set_border_width( GTK_CONTAINER( frame ),1 );
 frame=gtkAddFrame( NULL,GTK_SHADOW_NONE,frame,1 );
 frame=gtkAddFrame( NULL,GTK_SHADOW_ETCHED_OUT,frame,1 );
 frame=gtkAddFrame( NULL,GTK_SHADOW_NONE,frame,1 );
 return frame;
}

GtkWidget * gtkAddFrame( const char * title,int type,GtkWidget * parent,int add )
{
 GtkWidget * frame = NULL;
 frame=gtk_frame_new( title );
 gtk_widget_show( frame );
 gtk_frame_set_shadow_type( GTK_FRAME( frame ),type );
 if ( !parent ) return frame;
 if ( add ) gtk_container_add( GTK_CONTAINER( parent ),frame );
  else gtk_box_pack_start( GTK_BOX( parent ),frame,FALSE,FALSE,0 );
 return frame;
}

GtkWidget * gtkAddLabel( const char * title,GtkWidget * parent )
{
 GtkWidget * label;
 label=gtk_label_new( title );
 gtk_widget_show( label );
 if ( parent ) gtk_box_pack_start( GTK_BOX( parent ),label,FALSE,FALSE,0 );
 gtk_misc_set_alignment( GTK_MISC( label ),0,0.5 );
 gtk_misc_set_padding( GTK_MISC( label ),4,0 );
 return label;
}

GtkWidget * gtkAddVBox( GtkWidget * parent,int type )
{
 GtkWidget * vbox;
 vbox=gtk_vbox_new( FALSE,0 );
 gtk_widget_show( vbox );
 if ( parent )
  {
   if ( type ) gtk_box_pack_start( GTK_BOX( parent ),vbox,FALSE,FALSE,0 );
    else gtk_container_add( GTK_CONTAINER( parent ),vbox );
  }
 return vbox;
}

GtkWidget * gtkAddHBox( GtkWidget * parent,int type )
{
 GtkWidget * hbox;
 hbox=gtk_hbox_new( FALSE,0 );
 gtk_widget_show( hbox );
 if ( parent )
  {
   if ( type ) gtk_box_pack_start( GTK_BOX( parent ),hbox,FALSE,FALSE,0 );
    else gtk_container_add( GTK_CONTAINER( parent ),hbox );
  }
 return hbox;
}

GtkWidget * gtkAddCheckButton( const char * title, GtkWidget * parent )
{
 GtkWidget * CB;
 CB=gtk_check_button_new_with_label( title );
 gtk_widget_show( CB );
 gtk_box_pack_start( GTK_BOX( parent ),CB,FALSE,FALSE,0 );
 return CB;
}

GtkWidget * gtkAddRadioButton( const char * title,GSList ** group,GtkWidget * parent )
{
 GtkWidget * RB;
 RB=gtk_radio_button_new_with_label( *group,title );
 *group=gtk_radio_button_group( GTK_RADIO_BUTTON( RB ) );
 gtk_widget_show( RB );
 gtk_box_pack_start( GTK_BOX( parent ),RB,FALSE,FALSE,0 );
 return RB;
}

GtkWidget * gtkAddSpinButton( const char * title,GtkAdjustment * adj,GtkWidget * parent )
{
 GtkWidget * SB;
 GtkWidget * label;
 label=gtk_label_new( title );
 gtk_misc_set_alignment( GTK_MISC( label ),0,0.5 );
 gtk_box_pack_start( GTK_BOX( parent ),label,FALSE,FALSE,0 );
 gtk_widget_show( label );
 SB=gtk_spin_button_new( adj,0,0 );
 gtk_box_pack_start( GTK_BOX( parent ),SB,FALSE,FALSE,0 );
 gtk_widget_show( SB );
 return SB;
}

GtkWidget * gtkAddButton( const char * title,GtkWidget * parent )
{
 GtkWidget * B;
 B=gtk_button_new_with_label( title );
 gtk_widget_show( B );
 gtk_container_add( GTK_CONTAINER( parent ),B );
 return B;
}

GtkWidget * gtkAddHSeparator( GtkWidget * parent )
{
 GtkWidget * hseparator;
 hseparator=gtk_hseparator_new();
 gtk_widget_show( hseparator );
 gtk_box_pack_start( GTK_BOX( parent ),hseparator,FALSE,FALSE,0 );
 gtk_widget_set_usize( hseparator,-2,6 );
 return hseparator;
}

GtkWidget * gtkAddHButtonBox( GtkWidget * parent )
{
 GtkWidget * hbuttonbox;
 hbuttonbox=gtk_hbutton_box_new();
 gtk_widget_show( hbuttonbox );
 gtk_box_pack_start( GTK_BOX( parent ),hbuttonbox,FALSE,FALSE,0 );
 gtk_button_box_set_child_size( GTK_BUTTON_BOX( hbuttonbox ),85,20 );
 return hbuttonbox;
}

GtkWidget * gtkAddHScale( GtkAdjustment * adj,GtkWidget * parent,int digit )
{
 GtkWidget * HS;
 HS=gtk_hscale_new( adj );
 gtk_widget_show( HS );
 if ( parent ) gtk_box_pack_start( GTK_BOX( parent ),HS,TRUE,TRUE,0 );
 gtk_scale_set_value_pos( GTK_SCALE( HS ),GTK_POS_RIGHT );
 gtk_scale_set_digits( GTK_SCALE( HS ),digit );
 return HS;
}

GtkWidget * gtkAddVScale( GtkAdjustment * adj,GtkWidget * parent,int digit )
{
 GtkWidget * VS;
 VS=gtk_vscale_new( adj );
 gtk_widget_show( VS );
 if ( parent ) gtk_box_pack_start( GTK_BOX( parent ),VS,TRUE,TRUE,0 );
// gtk_scale_set_value_pos( GTK_SCALE( VS ),GTK_POS_RIGHT );
 if ( digit == -1 ) gtk_scale_set_draw_value( GTK_SCALE( VS ),FALSE );
  else gtk_scale_set_digits( GTK_SCALE( VS ),digit );
 return VS;
}

GtkWidget * gtkAddCombo( GtkWidget * parent )
{
 GtkWidget * CB;
 CB=gtk_combo_new();
 gtk_widget_show( CB );
 if ( parent ) gtk_box_pack_start( GTK_BOX( parent ),CB,TRUE,TRUE,0 );
 return CB;
}

int gtkFindInCList (GtkWidget *list, char *item)
{
  gint j;
  gchar *tmpstr;

  for (j = 0; j < GTK_CLIST(list)->rows; j++)
  {
    gtk_clist_get_text(GTK_CLIST(list), j, 0, &tmpstr);

    if (!strcmp(tmpstr, item)) return j;
  }

  return -1;
}
