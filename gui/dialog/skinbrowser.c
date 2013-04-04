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

#include <sys/stat.h>
#include <glob.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "skinbrowser.h"
#include "tools.h"

#include "gui/app/app.h"
#include "gui/app/cfg.h"
#include "gui/app/gui.h"
#include "gui/ui/actions.h"
#include "gui/skin/skin.h"
#include "help_mp.h"

#include "dialog.h"

GtkWidget * SkinList = NULL;
char      * sbSelectedSkin=NULL;
char      * sbSkinDirInHome=NULL;
char      * sbSkinDirInData=NULL;

char * gtkOldSkin=NULL;
static char * prev=NULL;

GtkWidget * SkinBrowser = NULL;

static void prButton( GtkButton * button,gpointer user_data )
{
 (void) button;

 if ( sbSelectedSkin )
 {
  switch ( (int)user_data )
   {
    case 0: // cancel
	if ( strcmp( sbSelectedSkin,gtkOldSkin ) ) uiChangeSkin( gtkOldSkin );
	break;
   case 1: // ok
	uiChangeSkin( sbSelectedSkin );   // NOTE TO MYSELF: skin already changed!
	free( skinName );
	skinName=strdup( sbSelectedSkin );
	break;
  }
 }
 gtk_widget_destroy( SkinBrowser );
}

static void on_SkinList_select_row( GtkCList * clist,gint row,gint column,GdkEvent * event,gpointer user_data )
{
 (void) column;
 (void) user_data;

 gtk_clist_get_text( clist,row,0,&sbSelectedSkin );
 if ( strcmp( prev,sbSelectedSkin ) )
  {
   prev=sbSelectedSkin;
   uiChangeSkin( sbSelectedSkin );
   gtkRaise( SkinBrowser );
  }
 if( !event ) return;
 if( event->type == GDK_2BUTTON_PRESS )
  {
   free( skinName );
   skinName=strdup( sbSelectedSkin );
   gtk_widget_destroy( SkinBrowser );
  }
}

static GtkWidget * CreateSkinBrowser( void )
{
 GtkWidget     * vbox5;
 GtkWidget     * scrolledwindow1;
 GtkWidget     * hbuttonbox4;
 GtkWidget     * Cancel;
 GtkWidget     * Ok;
 GtkAccelGroup * accel_group;

 accel_group = gtk_accel_group_new ();

 SkinBrowser=gtk_window_new( GTK_WINDOW_TOPLEVEL );
 gtk_widget_set_name( SkinBrowser,MSGTR_SkinBrowser );
 gtk_widget_set_usize( SkinBrowser,256,320 );
 gtk_container_set_border_width( GTK_CONTAINER( SkinBrowser ),1 );
 GTK_WIDGET_SET_FLAGS( SkinBrowser,GTK_CAN_DEFAULT );
 gtk_widget_set_events( SkinBrowser,GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_PROPERTY_CHANGE_MASK | GDK_VISIBILITY_NOTIFY_MASK );
 gtk_window_set_title( GTK_WINDOW( SkinBrowser ),MSGTR_SkinBrowser );
 gtk_window_set_position( GTK_WINDOW( SkinBrowser ),GTK_WIN_POS_CENTER );
 gtk_window_set_policy( GTK_WINDOW( SkinBrowser ),FALSE,FALSE,TRUE );
 gtk_window_set_wmclass( GTK_WINDOW( SkinBrowser ),"SkinBrowser","MPlayer" );

 gtk_widget_realize( SkinBrowser );
 gtkAddIcon( SkinBrowser );

 vbox5=gtkAddVBox( gtkAddDialogFrame( SkinBrowser ),0 );
 gtkAddLabel( MSGTR_SKIN_LABEL,vbox5 );
 gtkAddHSeparator( vbox5 );

 scrolledwindow1=gtk_scrolled_window_new( NULL,NULL );
 gtk_widget_set_name( scrolledwindow1,"scrolledwindow1" );
 gtk_widget_ref( scrolledwindow1 );
 gtk_object_set_data_full( GTK_OBJECT( SkinBrowser ),"scrolledwindow1",scrolledwindow1,(GtkDestroyNotify)gtk_widget_unref );
 gtk_widget_show( scrolledwindow1 );
 gtk_box_pack_start( GTK_BOX( vbox5 ),scrolledwindow1,TRUE,TRUE,0 );
 gtk_container_set_border_width( GTK_CONTAINER( scrolledwindow1 ),2 );
 gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolledwindow1 ),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC );

 SkinList=gtk_clist_new( 1 );
 gtk_widget_set_name( SkinList,"SkinList" );
 gtk_widget_ref( SkinList );
 gtk_object_set_data_full( GTK_OBJECT( SkinBrowser ),"SkinList",SkinList,(GtkDestroyNotify)gtk_widget_unref );
 gtk_widget_show( SkinList );
 gtk_container_add( GTK_CONTAINER( scrolledwindow1 ),SkinList );
 gtk_clist_set_column_width( GTK_CLIST( SkinList ),0,80 );
 gtk_clist_set_selection_mode( GTK_CLIST( SkinList ),GTK_SELECTION_SINGLE );
 gtk_clist_column_titles_hide( GTK_CLIST( SkinList ) );
 gtk_clist_set_shadow_type( GTK_CLIST( SkinList ),GTK_SHADOW_ETCHED_OUT );

 gtkAddHSeparator( vbox5 );

 hbuttonbox4=gtkAddHButtonBox( vbox5 );
  gtk_button_box_set_layout( GTK_BUTTON_BOX( hbuttonbox4 ),GTK_BUTTONBOX_SPREAD );
  gtk_button_box_set_spacing( GTK_BUTTON_BOX( hbuttonbox4 ),10 );

 Ok=gtkAddButton( MSGTR_Ok,hbuttonbox4 );
 Cancel=gtkAddButton( MSGTR_Cancel,hbuttonbox4 );

 gtk_widget_add_accelerator( Ok,"clicked",accel_group,GDK_Return,0,GTK_ACCEL_VISIBLE );
 gtk_widget_add_accelerator( Cancel,"clicked",accel_group,GDK_Escape,0,GTK_ACCEL_VISIBLE );

 gtk_signal_connect( GTK_OBJECT( SkinBrowser ),"destroy",GTK_SIGNAL_FUNC( gtk_widget_destroyed ),&SkinBrowser );
 gtk_signal_connect( GTK_OBJECT( SkinList ),"select_row",GTK_SIGNAL_FUNC( on_SkinList_select_row ),NULL );
 gtk_signal_connect( GTK_OBJECT( Ok ),"clicked",GTK_SIGNAL_FUNC( prButton ),(void *)1 );
 gtk_signal_connect( GTK_OBJECT( Cancel ),"clicked",GTK_SIGNAL_FUNC( prButton ),(void *)0 );

 if ( ( sbSkinDirInHome=calloc( 1,strlen( skinDirInHome ) + 4 ) ) != NULL )
  { strcpy( sbSkinDirInHome,skinDirInHome ); strcat( sbSkinDirInHome,"/*" ); }
 if ( ( sbSkinDirInData=calloc( 1,strlen( skinDirInData ) + 4 ) ) != NULL )
  { strcpy( sbSkinDirInData,skinDirInData ); strcat( sbSkinDirInData,"/*" ); }

 gtk_window_add_accel_group( GTK_WINDOW( SkinBrowser ),accel_group );
 gtk_widget_grab_focus( SkinList );

 return SkinBrowser;
}

void ShowSkinBrowser( void )
{
 if ( SkinBrowser ) gtkRaise( SkinBrowser );
   else SkinBrowser=CreateSkinBrowser();
}

int FillSkinList( gchar * mdir )
{
 gchar         * str[2];
 gchar         * tmp;
 int             i;
 glob_t          gg;
 struct stat     fs;

 gtkOldSkin=strdup( skinName );
 prev=gtkOldSkin;

 str[0]="default";
 str[1]="";
 if ( gtkFindInCList( SkinList,str[0] ) == -1 ) gtk_clist_append( GTK_CLIST( SkinList ),str );

 glob( mdir,GLOB_NOSORT,NULL,&gg );
 for( i=0;i<(int)gg.gl_pathc;i++ )
  {
   if ( !strcmp( gg.gl_pathv[i],"." ) || !strcmp( gg.gl_pathv[i],".." ) ) continue;
   if ( ( stat( gg.gl_pathv[i],&fs ) == 0 ) && S_ISDIR( fs.st_mode ) )
    {
     tmp=strrchr( gg.gl_pathv[i],'/' );
     if (tmp) tmp++;
     else tmp = gg.gl_pathv[i];
     if ( !strcmp( tmp,"default" ) ) continue;
     str[0]=tmp;
     if ( gtkFindInCList( SkinList,str[0] ) == -1 ) gtk_clist_append( GTK_CLIST( SkinList ),str );
    }
  }
 globfree( &gg );
 return True;
}
