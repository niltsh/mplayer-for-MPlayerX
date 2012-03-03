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

/* video window */

#include <string.h>

#include "config.h"
#include "libvo/x11_common.h"
#include "help_mp.h"
#include "mp_core.h"

#include "gmplayer.h"
#include "gui/app.h"
#include "gui/interface.h"
#include "widgets.h"

int             uiVideoRender = 0;
int             videoVisible = 0;

void uiVideoDraw( void )
{
 if ( guiApp.videoWindow.State == wsWindowClosed ) mplayer( MPLAYER_EXIT_GUI, EXIT_QUIT, 0 );

 if ( guiApp.videoWindow.State == wsWindowFocusIn ) videoVisible++;
 if ( guiApp.videoWindow.State == wsWindowFocusOut && metacity_hack != 3 ) videoVisible--;

 if ( !guiApp.videoWindow.Mapped ||
      guiApp.videoWindow.Visible == wsWindowNotVisible ) return;

 if ( guiInfo.Playing ) uiVideoRender=0;

 if ( uiVideoRender && guiApp.videoWindow.State == wsWindowExpose )
  {
   if ( guiApp.video.Bitmap.Image ) wsPutImage( &guiApp.videoWindow );
  }
 guiApp.videoWindow.State=0;
}

void uiVideoMouseHandle( int Button,int X,int Y,int RX,int RY )
{
 static int mplVideoMoved = 0;
 static int msButton = 0;

 uiPlaybarShow( Y );

 switch( Button )
  {
   case wsRRMouseButton:
          gtkShow( ivShowPopUpMenu,NULL );
          break;
   case wsPMMouseButton:
          gtkShow( ivHidePopUpMenu,NULL );
          uiShowMenu( RX,RY );
          msButton=wsPMMouseButton;
          break;
   case wsRMMouseButton:
          uiHideMenu( RX,RY,1 );
          msButton=0;
          break;
/* --- */
   case wsPLMouseButton:
          gtkShow( ivHidePopUpMenu,NULL );
          sx=X; sy=Y;
          msButton=wsPLMouseButton;
          mplVideoMoved=0;
          break;
   case wsMoveMouse:
          switch ( msButton )
           {
            case wsPLMouseButton:
                   mplVideoMoved=1;
                   if ( !guiApp.videoWindow.isFullScreen )
                    {
                     wsMoveWindow( &guiApp.videoWindow,True,RX - sx,RY - sy );
                     guiApp.video.x = guiApp.videoWindow.X;
                     guiApp.video.y = guiApp.videoWindow.Y;
                     // NOTE TO MYSELF: dragging the title bar goes unnoticed?
                    }
                   break;
            case wsPMMouseButton:
                   uiMenuMouseHandle( RX,RY );
                   break;
	    default: uiPlaybarShow( Y ); break;
           }
          break;
   case wsRLMouseButton:
          if ( ( !mplVideoMoved )&&( guiApp.videoWindow.isFullScreen ) )
           {
            if( videoVisible++%2 ) wsRaiseWindowTop( wsDisplay,guiApp.mainWindow.WindowID );
             else wsRaiseWindowTop( wsDisplay,guiApp.videoWindow.WindowID );
	   }
          msButton=0;
          mplVideoMoved=0;
          break;
  }
}
