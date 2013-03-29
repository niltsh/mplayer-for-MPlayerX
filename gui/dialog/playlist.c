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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "help_mp.h"
#include "stream/stream.h"

#include "gui/app/cfg.h"
#include "gui/app/gui.h"
#include "gui/interface.h"
#include "dialog.h"
#include "gui/util/list.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"
#include "playlist.h"
#include "tools.h"

#include "pixmaps/open2.xpm"
#include "pixmaps/dir2.xpm"

#define CFG_OLD_PLAYLIST 1
#include "gui/app/cfg-old.c"

       GtkWidget * Playlist = NULL;
static GtkWidget * CTDirTree;
static GtkWidget * CLFiles;
static GtkWidget * CLSelected;
static GtkWidget * Add;
static GtkWidget * Remove;
static GtkWidget * Ok;
static GtkWidget * Cancel;
static GdkPixmap * pxOpenedBook;
static GdkPixmap * pxClosedBook;
static GdkBitmap * msOpenedBook;
static GdkBitmap * msClosedBook;

static int   NrOfEntrys = 0;
static int   NrOfSelected = 0;
static int * CLFileSelected = NULL;
static int * CLListSelected = NULL;

static int sigSel;
static int sigUnsel;
static int sigEvent;

typedef struct
{
 int    scaned;
 char * path;
} DirNodeType;

static GtkCTreeNode * sibling;
static GtkCTreeNode * parent;
static gchar        * current_path;
static gchar        * old_path = NULL;

static int compare_func(const void *a, const void *b)
{
 char * tmp;
 int    i;
 if ( !a || !b || !( (DirNodeType *)a )->path ) return -1;
 tmp=strdup( (char *)b ); tmp[strlen( tmp )-1]=0;
 i=strcmp( ( (DirNodeType *)a )->path,tmp );
 free( tmp );
 return i;
}

static void plRowSelect( GtkCList * clist,gint row,gint column,GdkEvent * event,gpointer user_data )
{
 switch ( (int) user_data )
  {
   case 0: CLFileSelected[row]=True; break;
   case 1: CLListSelected[row]=True; break;
  }
}

static void plUnRowSelect( GtkCList * clist,gint row,gint column,GdkEvent * event,gpointer user_data )
{
 switch ( (int) user_data )
  {
   case 0: CLFileSelected[row]=False; break;
   case 1: CLListSelected[row]=False; break;
  }
}

static void plButtonReleased( GtkButton * button,gpointer user_data )
{
 switch ( (int) user_data )
 {
  case 1: // ok
       {
	int i;
	plItem curr, * item, * old;
	item = listMgr( PLAYLIST_ITEM_GET_CURR,0 );
	if (item)
	 {
	  curr.path = gstrdup(item->path);
	  curr.name = gstrdup(item->name);
	 }
	else
	 {
	  curr.path = NULL;
	  curr.name = NULL;
	 }
	listMgr( PLAYLIST_DELETE,0 );
	for ( i=0;i<NrOfSelected;i++ )
	 {
	  char * text[2];
	  item=calloc( 1,sizeof( plItem ) );
	  gtk_clist_get_text( GTK_CLIST( CLSelected ),i,2,&text[0] );
	  gtk_clist_get_text( GTK_CLIST( CLSelected ),i,3,&text[1] );
	  item->name = strdup( text[0] );
	  item->path = strdup( text[1] );
	  listMgr( PLAYLIST_ITEM_APPEND,item );
	 }
	item = listMgr( PLAYLIST_GET,0 );
	if ( item )
	 {
	  if ( guiInfo.Playing )
	   {
	    old = listMgr( PLAYLIST_ITEM_FIND,&curr );
	    if ( old )
	     {
	      listMgr( PLAYLIST_ITEM_SET_CURR,old );
	      guiInfo.Track = (int) listMgr( PLAYLIST_ITEM_GET_POS,old );
	      item = NULL;
	     }
	   }
	  if ( item )
	   {
	    uiSetFile( item->path,item->name,STREAMTYPE_FILE );
	    guiInfo.NewPlay = GUI_FILE_NEW;
	    guiInfo.PlaylistNext = !guiInfo.Playing;
	    guiInfo.Track = 1;
	   }
	 }
	else if (isPlaylistStreamtype && !guiInfo.Playing) uiUnsetFile();
	guiInfo.Tracks = (int) listMgr( PLAYLIST_ITEM_GET_POS,0 );
	free(curr.path);
	free(curr.name);
       }
  case 0: // cancel
       NrOfSelected=NrOfEntrys=0;
       nfree( CLListSelected );
       nfree( CLFileSelected );
       free( old_path );
       old_path=strdup( current_path );
       gtk_widget_destroy( Playlist );
       Playlist=NULL;
       break;
  case 2: // remove
       {
	int i; int j;

	gtk_signal_handler_block( GTK_OBJECT( CLSelected ),sigSel );
	gtk_signal_handler_block( GTK_OBJECT( CLSelected ),sigUnsel );
	gtk_signal_handler_block( GTK_OBJECT( CLSelected ),sigEvent );

        gtk_clist_freeze( GTK_CLIST( CLSelected ) );
        i = 0;
        while ( i<NrOfSelected )
        {
  	 if ( CLListSelected[i] )
	  {
	   gtk_clist_remove( GTK_CLIST( CLSelected ),i );
	   NrOfSelected--;
	   for ( j=i;j<NrOfSelected;j++ )
		CLListSelected[j] = CLListSelected[j+1];
	  }
  	 else i++;
        }
	gtk_clist_thaw( GTK_CLIST( CLSelected ) );

	gtk_signal_handler_unblock( GTK_OBJECT( CLSelected ),sigSel );
	gtk_signal_handler_unblock( GTK_OBJECT( CLSelected ),sigUnsel );
	gtk_signal_handler_unblock( GTK_OBJECT( CLSelected ),sigEvent );

       }
       break;
  case 3: // add
       {
        int i;
        void *p;
        char * itext[2];
        char * text[1][5]; text[0][4]="";
        gtk_clist_freeze( GTK_CLIST( CLSelected ) );
        for ( i=0;i<NrOfEntrys;i++ )
         {
          if ( CLFileSelected[i] )
           {
	    NrOfSelected++;
	    p=realloc( CLListSelected,NrOfSelected * sizeof( int ) );
	    if ( !p ) NrOfSelected--;
	    else
	     {
	      CLListSelected=p;
	      CLListSelected[NrOfSelected - 1]=False;
	      gtk_clist_get_text( GTK_CLIST( CLFiles ),i,0,&itext[0] );
	      gtk_clist_get_text( GTK_CLIST( CLFiles ),i,1,&itext[1] );
	      text[0][0]=itext[0]; text[0][1]=g_filename_display_name( current_path );
	      text[0][2]=itext[1]; text[0][3]=current_path;
	      gtk_clist_append( GTK_CLIST( CLSelected ),text[0] );
	      g_free( text[0][1] );
	     }
	   }
	 }
	gtk_clist_thaw( GTK_CLIST( CLSelected ) );
       }
       break;
 }
}

static gboolean plKeyReleased( GtkWidget * widget,
                               GdkEventKey * event,
                               gpointer user_data )
{
 if (event->keyval == GDK_Return)
  {
   if ( GTK_WIDGET_TYPE( widget ) == GTK_TYPE_BUTTON ) plButtonReleased( NULL, user_data );
   else
    {
     switch ( (int) user_data )
      {
       case 0:
            plButtonReleased( NULL, (void *) 3 );
            break;
       case 1:
            plButtonReleased( NULL, (void *) 2 );
            break;
      }
    }
  }
 return FALSE;
}

static gboolean plEvent ( GtkWidget * widget,
                          GdkEvent * event,
                          gpointer user_data )
{
  GdkEventButton *bevent;
  gint row, col;

  bevent = (GdkEventButton *) event;

  if ( event->type == GDK_BUTTON_RELEASE && bevent->button == 2 )
  {
    if ( gtk_clist_get_selection_info( GTK_CLIST( widget ), bevent->x, bevent->y, &row, &col ) )
    {
      switch ( (int) user_data )
      {
        case 0:
          CLFileSelected[row] = True;
          plButtonReleased( NULL, (void *) 3 );
          CLFileSelected[row] = False;
          return TRUE;

        case 1:
          CLListSelected[row] = True;
          plButtonReleased( NULL, (void *) 2 );
          return TRUE;
      }
    }
  }

  return FALSE;
}

static int check_for_subdir( gchar * path )
{
 DIR 	       * dir;
 struct dirent * dirent;
 struct stat     statbuf;
 gchar 	       * npath;

 if ( (dir=opendir( path )) )
  {
   while ( (dirent=readdir( dir )) )
    {
     if ( dirent->d_name[0] != '.' )
      {
       npath=calloc( 1,strlen( path ) + strlen( dirent->d_name ) + 3 );
       sprintf( npath,"%s/%s",path,dirent->d_name );
       if ( stat( npath,&statbuf ) != -1 && S_ISDIR( statbuf.st_mode ) )
        { free( npath ); closedir( dir ); return True; }
       free( npath );
      }
    }
   closedir( dir );
  }
 return False;
}

static void plCTree( GtkCTree * ctree,GtkCTreeNode * parent_node,gpointer user_data )
{
 GtkCTreeNode  * node;
 DirNodeType   * DirNode;
 gchar 		   * text, * utf8name = NULL;
 gchar 		   * dummy = "dummy";
 int     	 	 subdir = True;
 DIR   		   * dir = NULL;
 struct dirent * dirent;
 gchar  	   * path;
 struct 		 stat statbuf;

 DirNode=gtk_ctree_node_get_row_data( ctree,parent_node );
 if ( !DirNode->scaned )
  {
   DirNode->scaned=True; current_path=DirNode->path;
   gtk_clist_freeze( GTK_CLIST( ctree ) );
   node=gtk_ctree_find_by_row_data( ctree,parent_node,NULL );
   gtk_ctree_remove_node( ctree,node );

   if ( (dir=opendir( DirNode->path ) ) )
    {
     while( (dirent=readdir( dir )) )
      {
       path=calloc( 1,strlen( DirNode->path ) + strlen( dirent->d_name ) + 2 );
       if ( !strcmp( current_path,"/" ) ) sprintf( path,"/%s",dirent->d_name );
	else sprintf( path,"%s/%s",current_path,dirent->d_name );
       text=dirent->d_name;
       g_free( utf8name );
       utf8name=g_filename_display_name( text );

       if ( stat( path,&statbuf ) != -1 && S_ISDIR( statbuf.st_mode ) && dirent->d_name[0] != '.' )
	{
	 DirNode=malloc( sizeof( DirNodeType ) ); DirNode->scaned=False; DirNode->path=strdup( path );
	 subdir=check_for_subdir( path );
	 node=gtk_ctree_insert_node( ctree,parent_node,NULL,&utf8name,4,pxOpenedBook,msOpenedBook,pxClosedBook,msClosedBook,!subdir,FALSE );
	 gtk_ctree_node_set_row_data_full( ctree,node,DirNode,NULL );
	 if ( subdir ) gtk_ctree_insert_node( ctree,node,NULL,&dummy,4,NULL,NULL,NULL,NULL,FALSE,FALSE );
	}
       free( path ); path=NULL;
      }
     closedir( dir );
    }

   gtk_ctree_sort_node( ctree,parent_node );
   gtk_clist_thaw( GTK_CLIST( ctree ) );
  }

  g_free( utf8name );
}

static void scan_dir( char * path )
{
 DIR   		   * dir = NULL;
 char		   * curr;
 struct dirent * dirent;
 struct 		 stat statbuf;
 char 		   * text[1][3]; text[0][2]="";

 gtk_clist_clear( GTK_CLIST( CLFiles ) );
 if ( (dir=opendir( path )) )
  {
   NrOfEntrys=0;
   while( (dirent=readdir( dir )) )
    {
	 curr=calloc( 1,strlen( path ) + strlen( dirent->d_name ) + 3 ); sprintf( curr,"%s/%s",path,dirent->d_name );
	 if ( stat( curr,&statbuf ) != -1 && ( S_ISREG( statbuf.st_mode ) || S_ISLNK( statbuf.st_mode ) ) )
	  {
	   text[0][0]=g_filename_display_name( dirent->d_name );
	   text[0][1]=dirent->d_name;
	   gtk_clist_append( GTK_CLIST( CLFiles ), text[0] );
	   g_free( text[0][0] );
	   NrOfEntrys++;
	  }
	 free( curr );
	}
   closedir( dir );
   gtk_clist_sort( GTK_CLIST( CLFiles ) );
  }
}

static void plCTRow(GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
 DirNodeType  * DirNode;
 GtkCTreeNode * node;
 node=gtk_ctree_node_nth( GTK_CTREE( clist ),row );
 DirNode=gtk_ctree_node_get_row_data( GTK_CTREE( clist ),node );
 current_path=DirNode->path;
 gtk_ctree_expand( GTK_CTREE( clist ),node );
 scan_dir( DirNode->path );
 free( CLFileSelected );
 CLFileSelected=calloc( 1,NrOfEntrys * sizeof( int ) );
}

static GtkWidget * CreatePlaylist( void )
{
  GtkWidget 	* vbox1;
  GtkWidget 	* hbox1;
  GtkWidget 	* scrolledwindow1;
  GtkWidget 	* vbox2;
  GtkWidget 	* scrolledwindow2;
  GtkWidget 	* scrolledwindow3;
  GtkWidget 	* hbuttonbox1;
  GtkAccelGroup * accel_group;
  GdkColor 	  transparent = { 0,0,0,0 };
  gchar 	* root = "/";
  gchar 	* dummy = "dummy";
  DirNodeType 	* DirNode;
  plItem        * last;

  accel_group=gtk_accel_group_new();

  Playlist=gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_object_set_data( GTK_OBJECT( Playlist ),"PlayList",Playlist );
  gtk_widget_set_usize( Playlist,512,384 );
  gtk_window_set_title( GTK_WINDOW( Playlist ),MSGTR_PlayList );
  gtk_window_set_position( GTK_WINDOW( Playlist ),GTK_WIN_POS_CENTER );
//  gtk_window_set_policy( GTK_WINDOW( Playlist ),FALSE,FALSE,FALSE );
  gtk_window_set_wmclass( GTK_WINDOW( Playlist ),"Playlist","MPlayer" );

  gtk_widget_realize( Playlist );
  gtkAddIcon( Playlist );

  vbox1=AddVBox( AddDialogFrame( Playlist ),0 );
  hbox1=AddHBox( NULL,1 );
   gtk_box_pack_start( GTK_BOX( vbox1 ),hbox1,TRUE,TRUE,0 );

  scrolledwindow1=gtk_scrolled_window_new( NULL,NULL );
  gtk_widget_show( scrolledwindow1 );
  gtk_container_add( GTK_CONTAINER(
    AddFrame( NULL,0,hbox1,1 ) ),scrolledwindow1 );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolledwindow1 ),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC );

  CTDirTree=gtk_ctree_new( 1,0 );
  gtk_signal_connect( GTK_OBJECT( CTDirTree ),"tree_expand",GTK_SIGNAL_FUNC( plCTree ),(void*)0 );
  gtk_signal_connect( GTK_OBJECT( CTDirTree ),"select_row",GTK_SIGNAL_FUNC( plCTRow ),(void *)0 );
  gtk_container_add( GTK_CONTAINER( scrolledwindow1 ),CTDirTree );
  gtk_clist_set_column_auto_resize( GTK_CLIST( CTDirTree ),0,TRUE );
  gtk_clist_set_column_width( GTK_CLIST( CTDirTree ),0,80 );
  gtk_clist_set_selection_mode( GTK_CLIST( CTDirTree ),GTK_SELECTION_SINGLE );
  gtk_ctree_set_line_style( GTK_CTREE( CTDirTree ),GTK_CTREE_LINES_SOLID );
  gtk_clist_column_titles_show( GTK_CLIST( CTDirTree ) );
  gtk_clist_set_shadow_type( GTK_CLIST( CTDirTree ),GTK_SHADOW_NONE );

  if ( !pxOpenedBook ) pxOpenedBook=gdk_pixmap_create_from_xpm_d( Playlist->window,&msOpenedBook,&transparent,(gchar **)dir2_xpm );
  if ( !pxClosedBook ) pxClosedBook=gdk_pixmap_create_from_xpm_d( Playlist->window,&msClosedBook,&transparent,(gchar **)open2_xpm );

  parent=gtk_ctree_insert_node( GTK_CTREE( CTDirTree ),NULL,NULL,&root,4,pxOpenedBook,msOpenedBook,pxClosedBook,msClosedBook,FALSE,FALSE );
  DirNode=malloc( sizeof( DirNodeType ) );
  DirNode->scaned=False; DirNode->path=strdup( root );
  gtk_ctree_node_set_row_data_full(GTK_CTREE( CTDirTree ),parent,DirNode,NULL );
  sibling=gtk_ctree_insert_node( GTK_CTREE( CTDirTree ),parent,NULL,&dummy,4,NULL,NULL,NULL,NULL,TRUE,TRUE );
  gtk_ctree_expand( GTK_CTREE( CTDirTree ),parent );
  gtk_widget_show( CTDirTree );

  last=listMgr( PLAYLIST_ITEM_GET_LAST,0 );

  if ( last && last->path ) old_path = strdup( last->path );
  else if ( fsHistory[0] ) old_path = strdup( cfg_old_filename_from_utf8( fsHistory[0] ) );

  gtk_clist_set_column_widget( GTK_CLIST( CTDirTree ),0,
    AddLabel( MSGTR_PLAYLIST_DirectoryTree,NULL ) );

  gtk_clist_column_title_passive( GTK_CLIST( CTDirTree ),0 );

  vbox2=AddVBox(
    AddFrame( NULL,1,hbox1,1 ),0 );

  scrolledwindow2=gtk_scrolled_window_new( NULL,NULL );
  gtk_widget_show( scrolledwindow2 );
  gtk_box_pack_start( GTK_BOX( vbox2 ),scrolledwindow2,TRUE,TRUE,0 );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolledwindow2 ),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC );

  CLFiles=gtk_clist_new( 2 );
  gtk_widget_show( CLFiles );
  gtk_container_add( GTK_CONTAINER( scrolledwindow2 ),CLFiles );
  gtk_clist_set_column_width( GTK_CLIST( CLFiles ),0,80 );
  gtk_clist_set_column_visibility( GTK_CLIST( CLFiles ),1,FALSE );
  gtk_clist_set_selection_mode( GTK_CLIST( CLFiles ),GTK_SELECTION_EXTENDED );
  gtk_clist_column_titles_show( GTK_CLIST( CLFiles ) );
  gtk_clist_set_shadow_type( GTK_CLIST( CLFiles ),GTK_SHADOW_NONE );

  gtk_clist_set_column_widget( GTK_CLIST( CLFiles ),0,
    AddLabel( MSGTR_PLAYLIST_Files,NULL ) );

  gtk_clist_column_title_passive( GTK_CLIST( CLFiles ),0 );

  AddHSeparator( vbox2 );

  scrolledwindow3=gtk_scrolled_window_new( NULL,NULL );
  gtk_widget_show( scrolledwindow3 );
  gtk_box_pack_start( GTK_BOX( vbox2 ),scrolledwindow3,TRUE,TRUE,0 );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolledwindow3 ),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC );

  CLSelected=gtk_clist_new( 4 );
  gtk_widget_show( CLSelected );
  gtk_container_add( GTK_CONTAINER( scrolledwindow3 ),CLSelected );
  gtk_clist_set_column_width( GTK_CLIST( CLSelected ),0,295 );
  gtk_clist_set_column_width( GTK_CLIST( CLSelected ),1,295 );
  gtk_clist_set_column_visibility( GTK_CLIST( CLSelected ),2,FALSE );
  gtk_clist_set_column_visibility( GTK_CLIST( CLSelected ),3,FALSE );
  gtk_clist_set_selection_mode( GTK_CLIST( CLSelected ),GTK_SELECTION_MULTIPLE );
  gtk_clist_column_titles_show( GTK_CLIST( CLSelected ) );
  gtk_clist_set_shadow_type( GTK_CLIST( CLSelected ),GTK_SHADOW_NONE );

  gtk_clist_set_column_widget( GTK_CLIST( CLSelected ),0,
    AddLabel( MSGTR_PLAYLIST_Selected,NULL ) );

  gtk_clist_set_column_widget( GTK_CLIST( CLSelected ),1,
    AddLabel( MSGTR_PLAYLIST_Path,NULL ) );

  gtk_clist_column_title_passive( GTK_CLIST( CLSelected ),0 );

  AddHSeparator( vbox1 );

  hbuttonbox1=AddHButtonBox( vbox1 );
    gtk_button_box_set_layout( GTK_BUTTON_BOX( hbuttonbox1 ),GTK_BUTTONBOX_END );
    gtk_button_box_set_spacing( GTK_BUTTON_BOX( hbuttonbox1 ),10 );

  Add=AddButton( MSGTR_Add,hbuttonbox1 );
  Remove=AddButton( MSGTR_Remove,hbuttonbox1 );
  Ok=AddButton( MSGTR_Ok,hbuttonbox1 );
  Cancel=AddButton( MSGTR_Cancel,hbuttonbox1 );

  gtk_widget_add_accelerator( Cancel,"clicked",accel_group,GDK_Escape,0,GTK_ACCEL_VISIBLE );

  gtk_signal_connect( GTK_OBJECT( Playlist ),"destroy",GTK_SIGNAL_FUNC( WidgetDestroy ),&Playlist );

  gtk_signal_connect( GTK_OBJECT( CLFiles ),"select_row",GTK_SIGNAL_FUNC( plRowSelect ),(void *)0 );
  gtk_signal_connect( GTK_OBJECT( CLFiles ),"unselect_row",GTK_SIGNAL_FUNC( plUnRowSelect ),(void *)0 );
  gtk_signal_connect( GTK_OBJECT( CLFiles ),"event",GTK_SIGNAL_FUNC( plEvent ),(void *)0 );
  gtk_signal_connect( GTK_OBJECT( CLFiles ),"key_release_event",GTK_SIGNAL_FUNC( plKeyReleased ),(void *)0 );
  sigSel=gtk_signal_connect( GTK_OBJECT( CLSelected ),"select_row",GTK_SIGNAL_FUNC( plRowSelect ),(void*)1 );
  sigUnsel=gtk_signal_connect( GTK_OBJECT( CLSelected ),"unselect_row",GTK_SIGNAL_FUNC( plUnRowSelect ),(void*)1 );
  sigEvent=gtk_signal_connect( GTK_OBJECT( CLSelected ),"event",GTK_SIGNAL_FUNC( plEvent ),(void *)1 );
  gtk_signal_connect( GTK_OBJECT( CLSelected ),"key_release_event",GTK_SIGNAL_FUNC( plKeyReleased ),(void *)1 );

  gtk_signal_connect( GTK_OBJECT( Add ),"released",GTK_SIGNAL_FUNC( plButtonReleased ),(void*)3 );
  gtk_signal_connect( GTK_OBJECT( Add ),"key_release_event",GTK_SIGNAL_FUNC( plKeyReleased ),(void*)3 );
  gtk_signal_connect( GTK_OBJECT( Remove ),"released",GTK_SIGNAL_FUNC( plButtonReleased ),(void*)2 );
  gtk_signal_connect( GTK_OBJECT( Remove ),"key_release_event",GTK_SIGNAL_FUNC( plKeyReleased ),(void*)2 );
  gtk_signal_connect( GTK_OBJECT( Ok ),"released",GTK_SIGNAL_FUNC( plButtonReleased ),(void*)1 );
  gtk_signal_connect( GTK_OBJECT( Ok ),"key_release_event",GTK_SIGNAL_FUNC( plKeyReleased ),(void*)1 );
  gtk_signal_connect( GTK_OBJECT( Cancel ),"released",GTK_SIGNAL_FUNC( plButtonReleased ),(void*)0 );
  gtk_signal_connect( GTK_OBJECT( Cancel ),"key_release_event",GTK_SIGNAL_FUNC( plKeyReleased ),(void*)0 );

  gtk_window_add_accel_group( GTK_WINDOW( Playlist ),accel_group );

  return Playlist;
}

void ShowPlaylist( void )
{
 plItem * next;

 if ( Playlist ) gtkActive( Playlist );
  else Playlist=CreatePlaylist();

 if ( old_path && *old_path )
  {
   char         * currentdir = strdup( old_path );
   char         * tpath,* pos;
   GtkCTreeNode * node,* nextnode;
   gboolean       leaf;
   tpath=strdup( "/" );
   pos=strtok( currentdir,"/" );
   node=gtk_ctree_find_by_row_data_custom( GTK_CTREE( CTDirTree ),NULL,"/",compare_func );
   do
    {
     char * tpathnew = g_strconcat( tpath,pos,"/",NULL );
     free( tpath ); tpath=tpathnew;
     nextnode=gtk_ctree_find_by_row_data_custom( GTK_CTREE( CTDirTree ),node,tpath,compare_func );
     if ( !nextnode ) break;
     node=nextnode;
     pos=strtok( NULL,"/" );
     gtk_ctree_get_node_info( GTK_CTREE( CTDirTree ),node,NULL,NULL,NULL,NULL,NULL,NULL,&leaf,NULL );
     if ( !leaf && pos ) gtk_ctree_expand( GTK_CTREE( CTDirTree ),node );
      else
       {
        DirNodeType * DirNode;
        gtk_ctree_select( GTK_CTREE( CTDirTree ),node );
	DirNode=gtk_ctree_node_get_row_data( GTK_CTREE( CTDirTree ),node );
	current_path=DirNode->path;
        scan_dir( DirNode->path );
	free( CLFileSelected );
	CLFileSelected=calloc( 1,NrOfEntrys * sizeof( int ) );
	break;
       }
    } while( pos );
   free( tpath );
   free( currentdir );
  }
  else gtk_ctree_select( GTK_CTREE( CTDirTree ),parent );

 gtk_clist_freeze( GTK_CLIST( CLSelected ) );
 gtk_clist_clear( GTK_CLIST( CLSelected ) );
 next = listMgr( PLAYLIST_GET,0 );
 if ( next )
  {
   while ( next || next->next )
    {
     char * text[1][5]; text[0][4]="";
     text[0][0]=g_filename_display_name( next->name );
     text[0][1]=g_filename_display_name( next->path );
     text[0][2]=next->name;
     text[0][3]=next->path;
     gtk_clist_append( GTK_CLIST( CLSelected ),text[0] );
     g_free( text[0][0] );
     g_free( text[0][1] );
     NrOfSelected++;
     if ( next->next ) next=next->next; else break;
    }
   CLListSelected=calloc( 1,NrOfSelected * sizeof( int ) );
  }
 gtk_clist_thaw( GTK_CLIST( CLSelected ) );

 gtk_widget_show( Playlist );
}
