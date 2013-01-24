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

#include "config.h"
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

#include "help_mp.h"
#include "mp_msg.h"
#include "libvo/x11_common.h"
#include "libvo/fastmemcpy.h"
#include "libvo/wskeys.h"

#include "stream/stream.h"
#include "stream/url.h"
#include "mixer.h"
#include "sub/sub.h"
#include "access_mpcontext.h"

#include "libmpcodecs/vd.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"
#include "codec-cfg.h"
#include "m_option.h"
#include "m_property.h"
#include "mp_core.h"
#include "mpcommon.h"
#include "libavutil/common.h"

#define GUI_REDRAW_WAIT 375

#include "actions.h"
#include "gui/dialog/dialog.h"

unsigned int GetTimerMS( void );

unsigned char * mainDrawBuffer = NULL;
static int      uiMainRender = True;

int             uiMainAutoPlay = False;

int             mainVisible = True;

int             boxMoved = False;
int             sx = 0,sy = 0;
int             i,pot = 0;

#include "render.h"

void uiMainDraw( void )
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

static void MediumPrepare (int type)
{
  switch (type)
  {
    case STREAMTYPE_DVD:
      listMgr(PLAYLIST_DELETE, 0);
      break;

    case STREAMTYPE_CDDA:
    case STREAMTYPE_VCD:
      listMgr(PLAYLIST_DELETE, 0);
    case STREAMTYPE_FILE:
    case STREAMTYPE_STREAM:
    case STREAMTYPE_PLAYLIST:
      guiInfo.AudioStreams = 0;
      guiInfo.Subtitles = 0;
      guiInfo.Chapters = 0;
      guiInfo.Angles = 0;
      break;
  }
}

static unsigned last_redraw_time = 0;

void uiEventHandling( int msg,float param )
{
 int iparam = (int)param, osd;
 mixer_t *mixer = mpctx_get_mixer(guiInfo.mpcontext);

 switch( msg )
  {
/* user events */
   case evExit:
        mplayer( MPLAYER_EXIT_GUI, EXIT_QUIT, 0 );
        break;

   case evLoadURL:
        gtkShow( evLoadURL,NULL );
	break;

   case ivSetAudio:
        if ( !mpctx_get_demuxer(guiInfo.mpcontext) || audio_id == iparam ) break;
	mp_property_do("switch_audio",M_PROPERTY_SET,&iparam,guiInfo.mpcontext);
	break;

   case ivSetVideo:
        if ( !mpctx_get_demuxer(guiInfo.mpcontext) || video_id == iparam ) break;
	mp_property_do("switch_video",M_PROPERTY_SET,&iparam,guiInfo.mpcontext);
	break;

   case ivSetSubtitle:
        mp_property_do("sub",M_PROPERTY_SET,&iparam,guiInfo.mpcontext);
	break;

#ifdef CONFIG_CDDA
   case ivSetCDTrack:
        guiInfo.Track=iparam;
   case evPlayCD:
	guiInfo.StreamType=STREAMTYPE_CDDA;
	goto play;
#endif
#ifdef CONFIG_VCD
   case ivSetVCDTrack:
        guiInfo.Track=iparam;
   case evPlayVCD:
	guiInfo.StreamType=STREAMTYPE_VCD;
	goto play;
#endif
#ifdef CONFIG_DVDREAD
   case ivSetDVDSubtitle:
        dvdsub_id=iparam;
        uiEventHandling( ivPlayDVD, 0 );
        break;
   case ivSetDVDAudio:
        audio_id=iparam;
        uiEventHandling( ivPlayDVD, 0 );
        break;
   case ivSetDVDChapter:
        guiInfo.Chapter=iparam;
        uiEventHandling( ivPlayDVD, 0 );
        break;
   case ivSetDVDTitle:
        guiInfo.Track=iparam;
        guiInfo.Chapter=1;
        guiInfo.Angle=1;
        uiEventHandling( ivPlayDVD, 0 );
        break;
   case evPlayDVD:
        guiInfo.Chapter=1;
        guiInfo.Angle=1;
   case ivPlayDVD:
        guiInfo.StreamType=STREAMTYPE_DVD;
	goto play;
#endif
   case evPlay:
   case evPlaySwitchToPause:
play:

        if ( ( msg == evPlaySwitchToPause )&&( guiInfo.Playing == GUI_PAUSE ) ) goto NoPause;

        MediumPrepare( guiInfo.StreamType );

        switch ( guiInfo.StreamType )
         {
	  case STREAMTYPE_FILE:
	  case STREAMTYPE_STREAM:
	  case STREAMTYPE_PLAYLIST:
	       if ( !guiInfo.Track )
	         guiInfo.Track=1;
	       guiInfo.NewPlay=GUI_FILE_NEW;
	       break;

          case STREAMTYPE_CDDA:
	       if ( guiInfo.Playing != GUI_PAUSE )
	        {
		 if ( !guiInfo.Track )
                   guiInfo.Track=1;
                 guiInfo.NewPlay=GUI_FILE_SAME;
		}
	       break;

          case STREAMTYPE_VCD:
	       if ( guiInfo.Playing != GUI_PAUSE )
	        {
		 if ( !guiInfo.Track )
                   guiInfo.Track=2;
                 guiInfo.NewPlay=GUI_FILE_SAME;
		}
	       break;

          case STREAMTYPE_DVD:
	       if ( guiInfo.Playing != GUI_PAUSE )
	        {
		 if ( !guiInfo.Track )
                   guiInfo.Track=1;
                 guiInfo.NewPlay=GUI_FILE_SAME;
		}
               break;
         }
        uiPlay();
        break;

   case evPause:
   case evPauseSwitchToPlay:
NoPause:
        uiPause();
        break;

   case evStop:
	guiInfo.Playing=GUI_STOP;
	uiState();
	break;

   case evLoadPlay:
        uiMainAutoPlay=True;
//	guiInfo.StreamType=STREAMTYPE_FILE;
   case evLoad:
        gtkShow( evLoad,NULL );
        break;
   case evLoadSubtitle:  gtkShow( evLoadSubtitle,NULL );  break;
   case evDropSubtitle:
	nfree( guiInfo.SubtitleFilename );
	mplayerLoadSubtitle( NULL );
	break;
   case evLoadAudioFile: gtkShow( evLoadAudioFile,NULL ); break;
   case evPrev: uiPrev(); break;
   case evNext: uiNext(); break;

   case evPlaylist:    gtkShow( evPlaylist,NULL );        break;
   case evSkinBrowser: gtkShow( evSkinBrowser,skinName ); break;
   case evAbout:       gtkShow( evAbout,NULL );           break;
   case evPreferences: gtkShow( evPreferences,NULL );     break;
   case evEqualizer:   gtkShow( evEqualizer,NULL );       break;

   case evForward10min:	    uiRelSeek( 600 ); break;
   case evBackward10min:    uiRelSeek( -600 );break;
   case evForward1min:      uiRelSeek( 60 );  break;
   case evBackward1min:     uiRelSeek( -60 ); break;
   case evForward10sec:     uiRelSeek( 10 );  break;
   case evBackward10sec:    uiRelSeek( -10 ); break;
   case evSetMoviePosition: uiAbsSeek( param ); break;

   case evIncVolume:  vo_x11_putkey( wsGrayMul ); break;
   case evDecVolume:  vo_x11_putkey( wsGrayDiv ); break;
   case evMute:       mixer_mute( mixer ); break;

   case evSetVolume:
        guiInfo.Volume=param;
        {
	 float l = guiInfo.Volume * ( ( 100.0 - guiInfo.Balance ) / 50.0 );
	 float r = guiInfo.Volume * ( ( guiInfo.Balance ) / 50.0 );
         mixer_setvolume( mixer,FFMIN(l,guiInfo.Volume),FFMIN(r,guiInfo.Volume) );
	}
	if ( osd_level )
	 {
	  osd_visible=(GetTimerMS() + 1000) | 1;
	  vo_osd_progbar_type=OSD_VOLUME;
	  vo_osd_progbar_value=( ( guiInfo.Volume ) * 256.0 ) / 100.0;
	  vo_osd_changed( OSDTYPE_PROGBAR );
	 }
        break;

   case evSetBalance:
        guiInfo.Balance=param;
        mixer_setbalance( mixer,(guiInfo.Balance - 50.0 ) / 50.0 );   // transform 0..100 to -1..1
        osd = osd_level;
        osd_level = 0;
        uiEventHandling(evSetVolume, guiInfo.Volume);
        osd_level = osd;
        if ( osd_level )
         {
          osd_visible=(GetTimerMS() + 1000) | 1;
          vo_osd_progbar_type=OSD_BALANCE;
          vo_osd_progbar_value=( ( guiInfo.Balance ) * 256.0 ) / 100.0;
          vo_osd_changed( OSDTYPE_PROGBAR );
         }
        break;

   case evMenu:
        /*if (guiApp.menuIsPresent)   NOTE TO MYSELF: Uncomment only after mouse
         {                                            pointer and cursor keys work
          gtkShow( ivHidePopUpMenu,NULL );            with this menu from skin as
          uiShowMenu( 0,0 );                          they do with normal menus.
         }
        else*/ gtkShow( ivShowPopUpMenu,NULL );
        break;

   case evIconify:
        switch ( iparam )
         {
          case 0: wsWindowIconify( &guiApp.mainWindow ); break;
          case 1: wsWindowIconify( &guiApp.videoWindow ); break;
         }
        break;
   case evHalfSize:
        if ( guiInfo.VideoWindow && guiInfo.Playing )
         {
          if ( guiApp.videoWindow.isFullScreen )
           {
            uiFullScreen();
           }
          wsWindowResize( &guiApp.videoWindow, guiInfo.VideoWidth / 2, guiInfo.VideoHeight / 2 );
          btnSet( evFullScreen,btnReleased );
         }
        break;
   case evDoubleSize:
        if ( guiInfo.VideoWindow && guiInfo.Playing )
         {
          if ( guiApp.videoWindow.isFullScreen )
           {
            uiFullScreen();
           }
          wsWindowResize( &guiApp.videoWindow, guiInfo.VideoWidth * 2, guiInfo.VideoHeight * 2 );
          wsWindowMoveWithin( &guiApp.videoWindow, False, guiApp.video.x, guiApp.video.y );
          btnSet( evFullScreen,btnReleased );
         }
        break;
   case evNormalSize:
        if ( guiInfo.VideoWindow && guiInfo.Playing )
         {
          if ( guiApp.videoWindow.isFullScreen )
           {
            uiFullScreen();
           }
          wsWindowResize( &guiApp.videoWindow, guiInfo.VideoWidth, guiInfo.VideoHeight );
          btnSet( evFullScreen,btnReleased );
	  break;
         } else if ( !guiApp.videoWindow.isFullScreen ) break;
   case evFullScreen:
        if ( guiInfo.VideoWindow && ( guiInfo.Playing || !iparam ) )
         {
          uiFullScreen();
          if ( !guiApp.videoWindow.isFullScreen )
            wsWindowResize( &guiApp.videoWindow, iparam ? guiInfo.VideoWidth : guiApp.video.width, iparam ? guiInfo.VideoHeight : guiApp.video.height );
         }
	if ( guiApp.videoWindow.isFullScreen ) btnSet( evFullScreen,btnPressed );
	 else btnSet( evFullScreen,btnReleased );
        break;

   case evSetAspect:
	switch ( iparam )
	 {
	  case 2:  movie_aspect=16.0f / 9.0f; break;
	  case 3:  movie_aspect=4.0f / 3.0f;  break;
	  case 4:  movie_aspect=2.35;         break;
	  case 1:
	  default: movie_aspect=-1;
	 }
	if ( guiInfo.StreamType == STREAMTYPE_VCD ) uiEventHandling( evPlayVCD, 0 );
	 else if ( guiInfo.StreamType == STREAMTYPE_DVD ) uiEventHandling( ivPlayDVD, 0 );
	 else
	 guiInfo.NewPlay=GUI_FILE_NEW;
	break;

/* timer events */
   case ivRedraw:
        {
          unsigned now = GetTimerMS();
          if ((now > last_redraw_time) &&
              (now < last_redraw_time + GUI_REDRAW_WAIT) &&
              !uiPlaybarFade && (iparam == 0))
            break;
          last_redraw_time = now;
        }
        uiMainRender=True;
        wsWindowRedraw( &guiApp.mainWindow );
	wsWindowRedraw( &guiApp.playbarWindow );
        break;
/* system events */
   case evNone:
        mp_msg( MSGT_GPLAYER,MSGL_DBG2,"[main] uiEventHandling: evNone\n" );
        break;
   default:
        mp_msg( MSGT_GPLAYER,MSGL_DBG2,"[main] uiEventHandling: unknown event %d, param %.2f\n", msg, param );
        break;
  }
}

void uiMainMouse( int Button,int X,int Y,int RX,int RY )
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
          uiShowMenu( RX,RY );
          itemtype=itPRMButton;
          break;
   case wsRMMouseButton:
          uiHideMenu( RX,RY,0 );
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
              uiEventHandling( item->message,item->value );
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
                 uiMenuMouse( RX,RY );
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

void uiMainKey( int KeyCode,int Type,int Key )
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
	      uiEventHandling( evNormalSize,0 );
	      return;
	     }
      default:          vo_x11_putkey( Key ); return;
     }
   }
 if ( msg != evNone ) uiEventHandling( msg,0 );
}

/* this will be used to handle drag & drop files */
void uiDND(int num,char** files)
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
    if ( guiInfo.Playing == GUI_PLAY ) uiEventHandling( evStop,0 );
    uiEventHandling( evPlay,0 );
  }
  if (subtitles) {
    nfree(guiInfo.SubtitleFilename);
    guiInfo.SubtitleFilename = subtitles;
    mplayerLoadSubtitle(guiInfo.SubtitleFilename);
  }
}
