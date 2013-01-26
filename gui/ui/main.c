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

/* main window */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "ui.h"
#include "gui/app/app.h"
#include "gui/app/gui.h"
#include "gui/interface.h"
#include "gui/skin/font.h"
#include "gui/skin/skin.h"
#include "gui/util/list.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"
#include "gui/wm/ws.h"
#include "gui/wm/wsxdnd.h"

#include "help_mp.h"
#include "mp_msg.h"
#include "libvo/x11_common.h"
#include "libvo/fastmemcpy.h"
#include "libvo/wskeys.h"

#include "stream/stream.h"
#include "stream/url.h"

#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#include "codec-cfg.h"
#include "m_option.h"
#include "mp_core.h"
#include "libavutil/common.h"

#include "actions.h"

static unsigned char * mainDrawBuffer;
int             uiMainRender = True;

int             mainVisible = True;

int             boxMoved = False;
int             sx = 0,sy = 0;
int             i,pot = 0;

#include "render.h"

static void uiMainDraw( void )
{

 if ( guiApp.mainWindow.State == wsWindowClosed ) mplayer( MPLAYER_EXIT_GUI, EXIT_QUIT, 0 );

 if ( guiApp.mainWindow.Visible == wsWindowNotVisible ||
      !mainVisible ) return;
//      !guiApp.mainWindow.Mapped ) return;

 if ( uiMainRender && guiApp.mainWindow.State == wsWindowExpose )
  {
   btnModify( evSetMoviePosition,guiInfo.Position );
   btnModify( evSetVolume,guiInfo.Volume );

   fast_memcpy( mainDrawBuffer,guiApp.main.Bitmap.Image,guiApp.main.Bitmap.ImageSize );
   RenderAll( &guiApp.mainWindow,guiApp.mainItems,guiApp.IndexOfMainItems,mainDrawBuffer );
   uiMainRender=False;
  }
 // NOTE TO MYSELF: probably not, if uiMainRender is False
 wsImageDraw( &guiApp.mainWindow );
// XFlush( wsDisplay );
}

static void uiMainMouse( int Button,int X,int Y,int RX,int RY )
{
 static int     itemtype = 0;
        int     i;
        guiItem * item = NULL;
        float   value = 0.0f;

 static int     SelectedItem = -1;
        int     currentselected = -1;

 for ( i=0;i <= guiApp.IndexOfMainItems;i++ )
  if ( ( guiApp.mainItems[i].pressed != btnDisabled )&&
       ( isInside( X,Y,guiApp.mainItems[i].x,guiApp.mainItems[i].y,guiApp.mainItems[i].x+guiApp.mainItems[i].width,guiApp.mainItems[i].y+guiApp.mainItems[i].height ) ) )
   { currentselected=i; break; }

 switch ( Button )
  {
   case wsPMMouseButton:
	  gtkShow( ivHidePopUpMenu,NULL );
          uiMenuShow( RX,RY );
          itemtype=itPRMButton;
          break;
   case wsRMMouseButton:
          uiMenuHide( RX,RY,0 );
          break;

   case wsPLMouseButton:
	  gtkShow( ivHidePopUpMenu,NULL );
          sx=X; sy=Y; boxMoved=True; itemtype=itPLMButton;
          SelectedItem=currentselected;
          if ( SelectedItem == -1 ) break;
          boxMoved=False;
          item=&guiApp.mainItems[SelectedItem];
          itemtype=item->type;
          item->pressed=btnPressed;
          // NOTE TO MYSELF: commented, because the expression can never be true
          /*switch( item->type )
           {
            case itButton:
                 if ( ( SelectedItem > -1 ) &&
                    ( ( ( item->message == evPlaySwitchToPause && item->message == evPauseSwitchToPlay ) ) ||
                      ( ( item->message == evPauseSwitchToPlay && item->message == evPlaySwitchToPause ) ) ) )
                  { item->pressed=btnDisabled; }
                 break;
           }*/
          break;
   case wsRLMouseButton:
          boxMoved=False;
          if ( SelectedItem != -1 )   // NOTE TO MYSELF: only if itButton, itHPotmeter or itVPotmeter
           {
            item=&guiApp.mainItems[SelectedItem];
            item->pressed=btnReleased;
           }
          if ( currentselected == - 1 || SelectedItem == -1 ) { itemtype=0; break; }
          SelectedItem=-1;
          value=0;
          switch( itemtype )
           {
            case itPotmeter:
            case itHPotmeter:
                 btnModify( item->message,(float)( X - item->x ) / item->width * 100.0f );
		 uiEvent( item->message,item->value );
                 value=item->value;
                 break;
	    case itVPotmeter:
                 btnModify( item->message, ( 1. - (float)( Y - item->y ) / item->height) * 100.0f );
		 uiEvent( item->message,item->value );
                 value=item->value;
                 break;
           }
          uiEvent( item->message,value );
          itemtype=0;
          break;

   case wsRRMouseButton:
        gtkShow( ivShowPopUpMenu,NULL );
        break;

/* rolled mouse ... de szar :))) */
   case wsP5MouseButton: value=-2.5f; goto rollerhandled;
   case wsP4MouseButton: value= 2.5f;
rollerhandled:
          if (currentselected != - 1)
           {
            item=&guiApp.mainItems[currentselected];
            if ( ( item->type == itHPotmeter )||( item->type == itVPotmeter )||( item->type == itPotmeter ) )
             {
              item->value+=value;
              btnModify( item->message,item->value );
              uiEvent( item->message,item->value );
             }
           }
          break;

/* moving */
   case wsMoveMouse:
          item=&guiApp.mainItems[SelectedItem];
          switch ( itemtype )
           {
            case itPLMButton:
                 wsWindowMove( &guiApp.mainWindow,True,RX - abs( sx ),RY - abs( sy ) );
                 break;
            case itPRMButton:
                 if (guiApp.menuIsPresent) guiApp.menuWindow.MouseHandler( 0,RX,RY,0,0 );
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
                 uiEvent( item->message,item->value );
                 break;
           }
          break;
  }
}

static void uiMainKey( int KeyCode,int Type,int Key )
{
 int msg = evNone;

 if ( Type != wsKeyPressed ) return;

 if ( !Key )
  {
   switch ( KeyCode )
    {
     // NOTE TO MYSELF: This is only for the Acer AirKey V keyboard.
   /*case wsXFMMPrev:     msg=evPrev;              break;
     case wsXFMMStop:	  msg=evStop;              break;
     case wsXFMMPlay:	  msg=evPlaySwitchToPause; break;
     case wsXFMMNext:	  msg=evNext;	           break;
     case wsXFMMVolUp:	  msg=evIncVolume;         break;
     case wsXFMMVolDown:  msg=evDecVolume;         break;
     case wsXFMMMute: 	  msg=evMute;	           break;*/
    }
  }
  else
   {
    switch ( Key )
     {
      case wsEnter:            msg=evPlay; break;
      case wsXF86LowerVolume:  msg=evDecVolume; break;
      case wsXF86RaiseVolume:  msg=evIncVolume; break;
      case wsXF86Mute:         msg=evMute; break;
      case wsXF86Pause:
      case wsXF86Play:         msg=evPlaySwitchToPause; break;
      case wsXF86Stop:         msg=evStop; break;
      case wsXF86Prev:         msg=evPrev; break;
      case wsXF86Next:         msg=evNext; break;
      case wsXF86Media:        msg=evLoad; break;
      case wsEscape:
    	    if ( guiInfo.VideoWindow && guiInfo.Playing && guiApp.videoWindow.isFullScreen )
	     {
	      uiEvent( evNormalSize,0 );
	      return;
	     }
      default:          vo_x11_putkey( Key ); return;
     }
   }
 if ( msg != evNone ) uiEvent( msg,0 );
}

/* this will be used to handle drag & drop files */
static void uiMainDND(int num,char** files)
{
  struct stat buf;
  int f = 0;

  char* subtitles = NULL;
  char* file = NULL;
  char* s;

  if (num <= 0)
    return;


  /* now fill it with new items */
  for(f=0; f < num; f++){
    char* str = strdup( files[f] );
    plItem* item;

    url_unescape_string(str, files[f]);

    if(stat(str,&buf) == 0 && S_ISDIR(buf.st_mode) == 0) {
      /* this is not a directory so try to play it */
      mp_msg( MSGT_GPLAYER,MSGL_V,"Received D&D %s\n",str );

      /* check if it is a subtitle file */
      {
	char* ext = strrchr(str,'.');
	if (ext) {
	  static char supported[] = "utf/sub/srt/smi/rt//txt/ssa/aqt/";
	  char* type;
	  int len;
	  if((len=strlen(++ext)) && (type=strstr(supported,ext)) &&\
	     (type-supported)%4 == 0 && *(type+len) == '/'){
	    /* handle subtitle file */
	    nfree(subtitles);
	    subtitles = str;
	    continue;
	  }
	}
      }

      /* clear playlist */
      if (file == NULL) {
	file = files[f];
	listMgr(PLAYLIST_DELETE,0);
      }

      item = calloc(1,sizeof(plItem));

      s = strrchr( str,'/' );

      /* FIXME: decompose file name ? */
      /* yes -- Pontscho */
      if ( s ) {
	*s=0; s++;
	item->name = gstrdup( s );
	item->path = gstrdup( str );
      } else {
	// NOTE TO MYSELF: this shouldn't happen, make sure we have a full path
	item->name = strdup(str);
	item->path = strdup(".");
      }
      listMgr(PLAYLIST_ITEM_APPEND,item);
    } else {
      mp_msg( MSGT_GPLAYER,MSGL_WARN,MSGTR_NotAFile,str );
    }
    free( str );
  }

  if (file) {
    uiSetFile( NULL,file,STREAMTYPE_FILE );
    if ( guiInfo.Playing == GUI_PLAY ) uiEvent( evStop,0 );
    uiEvent( evPlay,0 );
  }
  if (subtitles) {
    nfree(guiInfo.SubtitleFilename);
    guiInfo.SubtitleFilename = subtitles;
    mplayerLoadSubtitle(guiInfo.SubtitleFilename);
  }
}

void uiMainInit (void)
{
  mainDrawBuffer = malloc(guiApp.main.Bitmap.ImageSize);

  if (!mainDrawBuffer)
  {
    gmp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_NEMDB);
    mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
  }

  wsWindowCreate(&guiApp.mainWindow, guiApp.main.x, guiApp.main.y, guiApp.main.width, guiApp.main.height, (guiApp.mainDecoration ? wsShowFrame : 0 ) | wsMinSize | wsMaxSize | wsHideWindow, wsShowMouseCursor | wsHandleMouseButton | wsHandleMouseMove, "MPlayer");
  mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[main] mainWindow ID: 0x%x\n", (int) guiApp.mainWindow.WindowID);
  wsWindowShape(&guiApp.mainWindow, guiApp.main.Mask.Image);
  wsWindowIcon(wsDisplay, guiApp.mainWindow.WindowID, &guiIcon);
  wsXDNDMakeAwareness(&guiApp.mainWindow);

  guiApp.mainWindow.DrawHandler = uiMainDraw;
  guiApp.mainWindow.MouseHandler = uiMainMouse;
  guiApp.mainWindow.KeyHandler = uiMainKey;
  guiApp.mainWindow.DNDHandler = uiMainDND;
}

void uiMainDone (void)
{
  nfree(mainDrawBuffer);
  wsWindowDestroy(&guiApp.mainWindow);
}
