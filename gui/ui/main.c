/*
 * main window
 *
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "gmplayer.h"
#include "gui/app.h"
#include "gui/interface.h"
#include "gui/skin/font.h"
#include "gui/skin/skin.h"
#include "gui/util/list.h"
#include "gui/util/mem.h"
#include "gui/util/string.h"
#include "gui/wm/ws.h"

#include "help_mp.h"
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

#define GUI_REDRAW_WAIT 375

#include "actions.h"
#include "widgets.h"

unsigned int GetTimerMS( void );

unsigned char * mainDrawBuffer = NULL;
int             uiMainRender = 1;

int             uiMainAutoPlay = 0;
int             uiMiddleMenu = 0;

int             mainVisible = 1;

int             boxMoved = 0;
int             sx = 0,sy = 0;
int             i,pot = 0;

#include "render.h"

void uiMainDraw( void )
{

 if ( guiApp.mainWindow.State == wsWindowClosed ) guiExit( EXIT_QUIT );

 if ( guiApp.mainWindow.Visible == wsWindowNotVisible ||
      !mainVisible ) return;
//      !guiApp.mainWindow.Mapped ) return;

 if ( uiMainRender && guiApp.mainWindow.State == wsWindowExpose )
  {
   btnModify( evSetMoviePosition,guiInfo.Position );
   btnModify( evSetVolume,guiInfo.Volume );

   fast_memcpy( mainDrawBuffer,guiApp.main.Bitmap.Image,guiApp.main.Bitmap.ImageSize );
   RenderAll( &guiApp.mainWindow,guiApp.mainItems,guiApp.IndexOfMainItems,mainDrawBuffer );
   uiMainRender=0;
  }
 wsPutImage( &guiApp.mainWindow );
// XFlush( wsDisplay );
}

static unsigned last_redraw_time = 0;

void uiEventHandling( int msg,float param )
{
 int iparam = (int)param;
 mixer_t *mixer = mpctx_get_mixer(guiInfo.mpcontext);

 switch( msg )
  {
// --- user events
   case evExit:
        guiExit( EXIT_QUIT );
        break;

   case evPlayNetwork:
        nfree( guiInfo.Subtitlename );
	nfree( guiInfo.AudioFile );
	guiInfo.StreamType=STREAMTYPE_STREAM;
        goto play;
   case evSetURL:
        gtkShow( evPlayNetwork,NULL );
	break;

   case evSetAudio:
        if ( !mpctx_get_demuxer(guiInfo.mpcontext) || audio_id == iparam ) break;
	audio_id=iparam;
	goto play;

   case evSetVideo:
        if ( !mpctx_get_demuxer(guiInfo.mpcontext) || video_id == iparam ) break;
	video_id=iparam;
	goto play;

   case evSetSubtitle:
        mp_property_do("sub",M_PROPERTY_SET,&iparam,guiInfo.mpcontext);
	break;

#ifdef CONFIG_VCD
   case evSetVCDTrack:
        guiInfo.Track=iparam;
   case evPlayVCD:
 	gtkSet( gtkClearStruct,0,(void *)guiALL );
	guiInfo.StreamType=STREAMTYPE_VCD;
	goto play;
#endif
#ifdef CONFIG_DVDREAD
   case evPlayDVD:
        guiInfo.DVD.current_title=1;
        guiInfo.DVD.current_chapter=1;
        guiInfo.DVD.current_angle=1;
play_dvd_2:
 	gtkSet( gtkClearStruct,0,(void *)(guiALL - guiDVD) );
        guiInfo.StreamType=STREAMTYPE_DVD;
	goto play;
#endif
   case evPlay:
   case evPlaySwitchToPause:
play:

        if ( ( msg == evPlaySwitchToPause )&&( guiInfo.Playing == GUI_PAUSE ) ) goto NoPause;

	if ( listSet( gtkGetCurrPlItem,NULL ) &&( guiInfo.StreamType == STREAMTYPE_FILE ) )
	 {
	  plItem * next = listSet( gtkGetCurrPlItem,NULL );
	  plLastPlayed=next;
	  uiSetFileName( next->path,next->name,STREAMTYPE_FILE );
	 }

        switch ( guiInfo.StreamType )
         {
	  case STREAMTYPE_STREAM:
	  case STREAMTYPE_FILE:
	       gtkSet( gtkClearStruct,0,(void *)(guiALL - guiFilenames) );
	       break;
#ifdef CONFIG_VCD
          case STREAMTYPE_VCD:
	       gtkSet( gtkClearStruct,0,(void *)(guiALL - guiVCD - guiFilenames) );
	       if ( !cdrom_device ) cdrom_device=gstrdup( DEFAULT_CDROM_DEVICE );
	       uiSetFileName( NULL,cdrom_device,STREAMTYPE_VCD );
	       if ( guiInfo.Playing != GUI_PAUSE )
	        {
		 if ( !guiInfo.Track )
                   guiInfo.Track=1;
                 guiInfo.DiskChanged=1;
		}
	       break;
#endif
#ifdef CONFIG_DVDREAD
          case STREAMTYPE_DVD:
	       gtkSet( gtkClearStruct,0,(void *)(guiALL - guiDVD - guiFilenames) );
	       if ( !dvd_device ) dvd_device=gstrdup( DEFAULT_DVD_DEVICE );
	       uiSetFileName( NULL,dvd_device,STREAMTYPE_DVD );
	       if ( guiInfo.Playing != GUI_PAUSE )
	        {
		 guiInfo.Title=guiInfo.DVD.current_title;
		 guiInfo.Chapter=guiInfo.DVD.current_chapter;
		 guiInfo.Angle=guiInfo.DVD.current_angle;
                 guiInfo.DiskChanged=1;
		}
               break;
#endif
         }
	guiInfo.NewPlay=1;
        uiPlay();
        break;
#ifdef CONFIG_DVDREAD
   case evSetDVDSubtitle:
        dvdsub_id=iparam;
        goto play_dvd_2;
        break;
   case evSetDVDAudio:
        audio_id=iparam;
        goto play_dvd_2;
        break;
   case evSetDVDChapter:
        guiInfo.DVD.current_chapter=iparam;
        goto play_dvd_2;
        break;
   case evSetDVDTitle:
        guiInfo.DVD.current_title=iparam;
	guiInfo.DVD.current_chapter=1;
	guiInfo.DVD.current_angle=1;
        goto play_dvd_2;
        break;
#endif

   case evPause:
   case evPauseSwitchToPlay:
NoPause:
        uiPause();
        break;

   case evStop:
	guiInfo.Playing=GUI_STOP;
	uiState();
	guiInfo.MovieWindow=True;
	break;

   case evLoadPlay:
        uiMainAutoPlay=1;
//	guiInfo.StreamType=STREAMTYPE_FILE;
   case evLoad:
	listSet( gtkDelPl,NULL );
        gtkShow( evLoad,NULL );
        break;
   case evLoadSubtitle:  gtkShow( evLoadSubtitle,NULL );  break;
   case evDropSubtitle:
	nfree( guiInfo.Subtitlename );
	guiLoadSubtitle( NULL );
	break;
   case evLoadAudioFile: gtkShow( evLoadAudioFile,NULL ); break;
   case evPrev: uiPrev(); break;
   case evNext: uiNext(); break;

   case evPlayList:    gtkShow( evPlayList,NULL );        break;
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
	goto set_volume;
   case evSetBalance:
        guiInfo.Balance=param;
set_volume:
        {
	 float l = guiInfo.Volume * ( ( 100.0 - guiInfo.Balance ) / 50.0 );
	 float r = guiInfo.Volume * ( ( guiInfo.Balance ) / 50.0 );
	 if ( l > guiInfo.Volume ) l=guiInfo.Volume;
	 if ( r > guiInfo.Volume ) r=guiInfo.Volume;
//	 printf( "!!! v: %.2f b: %.2f -> %.2f x %.2f\n",guiInfo.Volume,guiInfo.Balance,l,r );
         mixer_setvolume( mixer,l,r );
	}
	if ( osd_level )
	 {
	  osd_visible=(GetTimerMS() + 1000) | 1;
	  vo_osd_progbar_type=OSD_VOLUME;
	  vo_osd_progbar_value=( ( guiInfo.Volume ) * 256.0 ) / 100.0;
	  vo_osd_changed( OSDTYPE_PROGBAR );
	 }
        break;


   case evIconify:
        switch ( iparam )
         {
          case 0: wsIconify( guiApp.mainWindow ); break;
          case 1: wsIconify( guiApp.subWindow ); break;
         }
        break;
   case evHalfSize:
        btnSet( evFullScreen,btnReleased );
        if ( guiInfo.Playing )
         {
          if ( guiApp.subWindow.isFullScreen )
           {
            uiFullScreen();
           }
          wsResizeWindow( &guiApp.subWindow, guiInfo.MovieWidth / 2, guiInfo.MovieHeight / 2 );
          wsMoveWindow( &guiApp.subWindow, False,
                        ( wsMaxX - guiInfo.MovieWidth/2  )/2 + wsOrgX,
                        ( wsMaxY - guiInfo.MovieHeight/2 )/2 + wsOrgY  );
         }
        break;
   case evDoubleSize:
    	btnSet( evFullScreen,btnReleased );
        if ( guiInfo.Playing )
         {
          if ( guiApp.subWindow.isFullScreen )
           {
            uiFullScreen();
           }
          wsResizeWindow( &guiApp.subWindow, guiInfo.MovieWidth * 2, guiInfo.MovieHeight * 2 );
          wsMoveWindow( &guiApp.subWindow, False,
                        ( wsMaxX - guiInfo.MovieWidth*2  )/2 + wsOrgX,
                        ( wsMaxY - guiInfo.MovieHeight*2 )/2 + wsOrgY  );
         }
        break;
   case evNormalSize:
	btnSet( evFullScreen,btnReleased );
        if ( guiInfo.Playing )
         {
          if ( guiApp.subWindow.isFullScreen )
           {
            uiFullScreen();
           }
          wsResizeWindow( &guiApp.subWindow, guiInfo.MovieWidth, guiInfo.MovieHeight );
          wsMoveWindow( &guiApp.subWindow, False,
                        ( wsMaxX - guiInfo.MovieWidth  )/2 + wsOrgX,
                        ( wsMaxY - guiInfo.MovieHeight )/2 + wsOrgY  );
	  break;
         } else if ( !guiApp.subWindow.isFullScreen ) break;
   case evFullScreen:
        if ( !guiInfo.Playing && !gtkShowVideoWindow ) break;
        uiFullScreen();
	if ( guiApp.subWindow.isFullScreen ) btnSet( evFullScreen,btnPressed );
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
	wsClearWindow( guiApp.subWindow );
#ifdef CONFIG_DVDREAD
	if ( guiInfo.StreamType == STREAMTYPE_DVD || guiInfo.StreamType == STREAMTYPE_VCD ) goto play_dvd_2;
	 else
#endif
	 guiInfo.NewPlay=1;
	break;

// --- timer events
   case evRedraw:
        {
          unsigned now = GetTimerMS();
          if ((now > last_redraw_time) &&
              (now < last_redraw_time + GUI_REDRAW_WAIT) &&
              !uiPlaybarFade)
            break;
          last_redraw_time = now;
        }
        uiMainRender=1;
        wsPostRedisplay( &guiApp.mainWindow );
	wsPostRedisplay( &guiApp.playbarWindow );
        break;
// --- system events
#ifdef MP_DEBUG
   case evNone:
        mp_msg( MSGT_GPLAYER,MSGL_STATUS,"[mw] event none received.\n" );
        break;
   default:
        mp_msg( MSGT_GPLAYER,MSGL_STATUS,"[mw] unknown event received ( %d,%.2f ).\n",msg,param );
        break;
#endif
  }
}

void uiMainMouseHandle( int Button,int X,int Y,int RX,int RY )
{
 static int     itemtype = 0;
        int     i;
        wItem * item = NULL;
        float   value = 0.0f;

 static int     SelectedItem = -1;
        int     currentselected = -1;

 for ( i=0;i <= guiApp.IndexOfMainItems;i++ )
  if ( ( guiApp.mainItems[i].pressed != btnDisabled )&&
       ( wgIsRect( X,Y,guiApp.mainItems[i].x,guiApp.mainItems[i].y,guiApp.mainItems[i].x+guiApp.mainItems[i].width,guiApp.mainItems[i].y+guiApp.mainItems[i].height ) ) )
   { currentselected=i; break; }

 switch ( Button )
  {
   case wsPMMouseButton:
	  gtkShow( evHidePopUpMenu,NULL );
          uiShowMenu( RX,RY );
          itemtype=itPRMButton;
          break;
   case wsRMMouseButton:
          uiHideMenu( RX,RY,0 );
          break;

   case wsPLMouseButton:
	  gtkShow( evHidePopUpMenu,NULL );
          sx=X; sy=Y; boxMoved=1; itemtype=itPLMButton;
          SelectedItem=currentselected;
          if ( SelectedItem == -1 ) break;
          boxMoved=0;
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
          boxMoved=0;
          if ( SelectedItem != -1 )   // NOTE TO MYSELF: only if itButton, itHPotmeter or itVPotmeter
           {
            item=&guiApp.mainItems[SelectedItem];
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

   case wsRRMouseButton:
        gtkShow( evShowPopUpMenu,NULL );
        break;

// --- rolled mouse ... de szar :)))
   case wsP5MouseButton: value=-2.5f; goto rollerhandled;
   case wsP4MouseButton: value= 2.5f;
rollerhandled:
          item=&guiApp.mainItems[currentselected];
          if ( ( item->type == itHPotmeter )||( item->type == itVPotmeter )||( item->type == itPotmeter ) )
           {
            item->value+=value;
            btnModify( item->message,item->value );
            uiEventHandling( item->message,item->value );
           }
          break;

// --- moving
   case wsMoveMouse:
          item=&guiApp.mainItems[SelectedItem];
          switch ( itemtype )
           {
            case itPLMButton:
                 wsMoveWindow( &guiApp.mainWindow,False,RX - abs( sx ),RY - abs( sy ) );
                 uiMainRender=0;
                 break;
            case itPRMButton:
                 uiMenuMouseHandle( X,Y,RX,RY );
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

int keyPressed = 0;

void uiMainKeyHandle( int KeyCode,int Type,int Key )
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
    	    if ( guiApp.subWindow.isFullScreen )
	     {
	      if ( guiInfo.event_struct ) ((XEvent *)guiInfo.event_struct)->type=None;
	      uiEventHandling( evNormalSize,0 );
	      return;
	     }
      default:          vo_x11_putkey( Key ); return;
     }
   }
 if ( msg != evNone ) uiEventHandling( msg,0 );
}

/* this will be used to handle Drag&Drop files */
void uiDandDHandler(int num,char** files)
{
  struct stat buf;
  int f = 0;

  char* subtitles = NULL;
  char* filename = NULL;

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
      if (filename == NULL) {
	filename = files[f];
	listSet(gtkDelPl,NULL);
      }

      item = calloc(1,sizeof(plItem));

      /* FIXME: decompose file name ? */
      /* yes -- Pontscho */
      if ( strrchr( str,'/' ) ) {
	char * s = strrchr( str,'/' ); *s=0; s++;
	item->name = gstrdup( s );
	item->path = gstrdup( str );
      } else {
	item->name = strdup(str);
	item->path = strdup("");
      }
      listSet(gtkAddPlItem,item);
    } else {
      mp_msg( MSGT_GPLAYER,MSGL_WARN,MSGTR_NotAFile,str );
    }
    free( str );
  }

  if (filename) {
    uiSetFileName( NULL,filename,STREAMTYPE_FILE );
    if ( guiInfo.Playing == GUI_PLAY ) uiEventHandling( evStop,0 );
    uiEventHandling( evPlay,0 );
  }
  if (subtitles) {
    nfree(guiInfo.Subtitlename);
    guiInfo.Subtitlename = subtitles;
    guiLoadSubtitle(guiInfo.Subtitlename);
  }
}
