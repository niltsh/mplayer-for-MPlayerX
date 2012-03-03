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

/* playbar window */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "gui/app.h"
#include "gui/interface.h"
#include "gui/skin/font.h"
#include "gui/skin/skin.h"
#include "gui/util/mem.h"
#include "gui/wm/ws.h"

#include "help_mp.h"
#include "mp_core.h"
#include "libvo/x11_common.h"
#include "libvo/fastmemcpy.h"

#include "stream/stream.h"
#include "mixer.h"
#include "sub/sub.h"

#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#include "codec-cfg.h"

#include "gmplayer.h"
#include "actions.h"
#include "widgets.h"
#include "render.h"

unsigned int GetTimerMS( void );
unsigned int GetTimer( void );

unsigned char * playbarDrawBuffer = NULL;
int		playbarVisible = 0;
int  		playbarLength = 0;
int		uiPlaybarFade = 0;

static void uiPlaybarDraw( void )
{
 int x;

 if ( !guiApp.videoWindow.isFullScreen ) return;
 if ( !playbarVisible || !guiApp.playbarIsPresent ) return;

// guiApp.playbar.x=( guiApp.videoWindow.Width - guiApp.playbar.width ) / 2;
 switch( guiApp.playbar.x )
  {
   case -1: x=( guiApp.videoWindow.Width - guiApp.playbar.width ) / 2; break;
   case -2: x=( guiApp.videoWindow.Width - guiApp.playbar.width ); break;
   default: x=guiApp.playbar.x;
  }

 switch ( uiPlaybarFade )
  {
   case 1: // fade in
        playbarLength--;
        if ( guiApp.videoWindow.Height - guiApp.playbar.height >= playbarLength )
	 {
	  playbarLength=guiApp.videoWindow.Height - guiApp.playbar.height;
	  uiPlaybarFade=0;
	  vo_mouse_autohide=0;
	 }
        wsMoveWindow( &guiApp.playbarWindow,True,x,playbarLength );
	break;
   case 2: // fade out
	playbarLength+=10;
	if ( playbarLength > guiApp.videoWindow.Height )
	 {
	  playbarLength=guiApp.videoWindow.Height;
	  uiPlaybarFade=playbarVisible=0;
          vo_mouse_autohide=1;
          wsVisibleWindow( &guiApp.playbarWindow,wsHideWindow );
	  return;
	 }
        wsMoveWindow( &guiApp.playbarWindow,True,x,playbarLength );
	break;
  }

/* render */
 if ( guiApp.playbarWindow.State == wsWindowExpose )
  {
   btnModify( evSetMoviePosition,guiInfo.Position );
   btnModify( evSetVolume,guiInfo.Volume );

   vo_mouse_autohide=0;

   fast_memcpy( playbarDrawBuffer,guiApp.playbar.Bitmap.Image,guiApp.playbar.Bitmap.ImageSize );
   RenderAll( &guiApp.playbarWindow,guiApp.playbarItems,guiApp.IndexOfPlaybarItems,playbarDrawBuffer );
   wsConvert( &guiApp.playbarWindow,playbarDrawBuffer );
  }
 wsPutImage( &guiApp.playbarWindow );
}

static void uiPlaybarMouseHandle( int Button, int X, int Y, int RX, int RY )
{
 static int     itemtype = 0;
        int     i;
        wItem * item = NULL;
	float   value = 0.0f;

 static int     SelectedItem = -1;
	int     currentselected = -1;

 for ( i=0;i <= guiApp.IndexOfPlaybarItems;i++ )
   if ( ( guiApp.playbarItems[i].pressed != btnDisabled )&&
      ( wgIsRect( X,Y,guiApp.playbarItems[i].x,guiApp.playbarItems[i].y,guiApp.playbarItems[i].x+guiApp.playbarItems[i].width,guiApp.playbarItems[i].y+guiApp.playbarItems[i].height ) ) )
    { currentselected=i; break; }

 switch ( Button )
  {
   case wsPMMouseButton:
        gtkShow( ivHidePopUpMenu,NULL );
        uiShowMenu( RX,RY );
        break;
   case wsRMMouseButton:
        uiHideMenu( RX,RY,0 );
        break;
   case wsRRMouseButton:
        gtkShow( ivShowPopUpMenu,NULL );
	break;
/* --- */
   case wsPLMouseButton:
	gtkShow( ivHidePopUpMenu,NULL );
        SelectedItem=currentselected;
        if ( SelectedItem == -1 ) break; // yeees, i'm move the fucking window
        item=&guiApp.playbarItems[SelectedItem];
	itemtype=item->type;
	item->pressed=btnPressed;

	switch( item->type )
	 {
	  case itButton:
	       if ( ( SelectedItem > -1 ) &&
	         ( ( ( item->message == evPlaySwitchToPause && item->message == evPauseSwitchToPlay ) ) ||
		 ( ( item->message == evPauseSwitchToPlay && item->message == evPlaySwitchToPause ) ) ) )
		 { item->pressed=btnDisabled; }
	       break;
	 }

	break;
   case wsRLMouseButton:
        if ( SelectedItem != -1 )   // NOTE TO MYSELF: only if itButton, itHPotmeter or itVPotmeter
         {
          item=&guiApp.playbarItems[SelectedItem];
          item->pressed=btnReleased;
         }
	SelectedItem=-1;
	if ( currentselected == - 1 ) { itemtype=0; break; }
	value=0;

	switch( itemtype )
	 {
	  case itPotmeter:
	  case itHPotmeter:
	       btnModify( item->message,(float)( X - item->x ) / item->width * 100.0f );
	       uiEventHandling( item->message,item->value );
	       value=item->value;
	       break;
	  case itVPotmeter:
	       btnModify( item->message, ( 1. - (float)( Y - item->y ) / item->height) * 100.0f );
	       uiEventHandling( item->message,item->value );
	       value=item->value;
	       break;
	 }
	uiEventHandling( item->message,value );

	itemtype=0;
	break;
/* --- */
   case wsP5MouseButton: value=-2.5f; goto rollerhandled;
   case wsP4MouseButton: value= 2.5f;
rollerhandled:
        item=&guiApp.playbarItems[currentselected];
        if ( ( item->type == itHPotmeter )||( item->type == itVPotmeter )||( item->type == itPotmeter ) )
	 {
	  item->value+=value;
	  btnModify( item->message,item->value );
	  uiEventHandling( item->message,item->value );
	 }
	break;
/* --- */
   case wsMoveMouse:
        item=&guiApp.playbarItems[SelectedItem];
	switch ( itemtype )
	 {
	  case itPRMButton:
	       uiMenuMouseHandle( RX,RY );
	       break;
	  case itPotmeter:
	       item->value=(float)( X - item->x ) / item->width * 100.0f;
	       goto potihandled;
	  case itVPotmeter:
	       item->value=(1. - (float)( Y - item->y ) / item->height) * 100.0f;
	       goto potihandled;
	  case itHPotmeter:
	       item->value=(float)( X - item->x ) / item->width * 100.0f;
potihandled:
	       if ( item->value > 100.0f ) item->value=100.0f;
	       if ( item->value < 0.0f ) item->value=0.0f;
	       uiEventHandling( item->message,item->value );
	       break;
	 }
        break;
  }
}

void uiPlaybarShow( int y )
{
 if ( !guiApp.playbarIsPresent || !gtkEnablePlayBar ) return;
 if ( !guiApp.videoWindow.isFullScreen ) return;

 if ( y > guiApp.videoWindow.Height - guiApp.playbar.height )
  {
   if ( !uiPlaybarFade ) wsVisibleWindow( &guiApp.playbarWindow,wsShowWindow );
   uiPlaybarFade=1; playbarVisible=1; wsPostRedisplay( &guiApp.playbarWindow );
  }
  else if ( !uiPlaybarFade ) uiPlaybarFade=2;
}

void uiPlaybarInit( void )
{
 if ( !guiApp.playbarIsPresent ) return;

 nfree( playbarDrawBuffer );

 if ( ( playbarDrawBuffer = malloc( guiApp.playbar.Bitmap.ImageSize ) ) == NULL )
  {
   gmp_msg( MSGT_GPLAYER,MSGL_FATAL,MSGTR_NEMDB );
   mplayer( MPLAYER_EXIT_GUI, EXIT_ERROR, 0 );
  }

 guiApp.playbarWindow.Parent=guiApp.videoWindow.WindowID;
 wsCreateWindow( &guiApp.playbarWindow,
   guiApp.playbar.x,guiApp.playbar.y,guiApp.playbar.width,guiApp.playbar.height,
   wsNoBorder,wsShowMouseCursor|wsHandleMouseButton|wsHandleMouseMove,wsHideFrame|wsHideWindow,"PlayBar" );

 wsSetShape( &guiApp.playbarWindow,guiApp.playbar.Mask.Image );

 guiApp.playbarWindow.ReDraw=(void *)uiPlaybarDraw;
 guiApp.playbarWindow.MouseHandler=uiPlaybarMouseHandle;
 guiApp.playbarWindow.KeyHandler=uiMainKeyHandle;

 playbarLength=guiApp.videoWindow.Height;
}
