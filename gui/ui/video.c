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

#include "libvo/x11_common.h"
#include "help_mp.h"
#include "mp_msg.h"
#include "mp_core.h"

#include "ui.h"
#include "gui/app/app.h"
#include "gui/app/gui.h"
#include "gui/interface.h"
#include "gui/dialog/dialog.h"
#include "gui/wm/ws.h"
#include "gui/wm/wsxdnd.h"

int             videoVisible = 0;

static void uiVideoDraw( void )
{
 if ( guiApp.videoWindow.State == wsWindowClosed ) mplayer( MPLAYER_EXIT_GUI, EXIT_QUIT, 0 );

 if ( guiApp.videoWindow.State == wsWindowFocusIn ) videoVisible++;
 if ( guiApp.videoWindow.State == wsWindowFocusOut && metacity_hack != 3 ) videoVisible--;

 if ( !guiApp.videoWindow.Mapped ||
      guiApp.videoWindow.Visible == wsWindowNotVisible ||
      guiInfo.Playing) return;

 if ( guiApp.videoWindow.State == wsWindowExpose )
  {
   wsWindowBackground(&guiApp.videoWindow, guiApp.video.R, guiApp.video.G, guiApp.video.B);
   if ( guiApp.video.Bitmap.Image ) wsImageDraw( &guiApp.videoWindow );
  }
}

static void uiVideoMouse( int Button,int X,int Y,int RX,int RY )
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
          uiMenuShow( RX,RY );
          msButton=wsPMMouseButton;
          break;
   case wsRMMouseButton:
          uiMenuHide( RX,RY,1 );
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
                     wsWindowMove( &guiApp.videoWindow,True,RX - sx,RY - sy );
                    }
                   break;
            case wsPMMouseButton:
                   if (guiApp.menuIsPresent) guiApp.menuWindow.MouseHandler( 0,RX,RY,0,0 );
                   break;
	    default: uiPlaybarShow( Y ); break;
           }
          break;
   case wsRLMouseButton:
          if ( ( !mplVideoMoved )&&( guiApp.videoWindow.isFullScreen ) )
           {
            // NOTE TO MYSELF: this doesn't work, fix later with wsWindowLayer()?
            if( videoVisible++%2 ) wsWindowRaiseTop( wsDisplay,guiApp.mainWindow.WindowID );
             else wsWindowRaiseTop( wsDisplay,guiApp.videoWindow.WindowID );
	   }
          msButton=0;
          mplVideoMoved=0;
          break;
  }
}

void uiVideoInit (void)
{
  wsWindowCreate(&guiApp.videoWindow, guiApp.video.x, guiApp.video.y, guiApp.video.width, guiApp.video.height, wsShowFrame | wsHideWindow | wsWaitMap | wsAspect, wsShowMouseCursor | wsHandleMouseButton | wsHandleMouseMove, "MPlayer - Video");
  mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[video] videoWindow ID: 0x%x\n", (int) guiApp.videoWindow.WindowID);
  wsWindowIcon(wsDisplay, guiApp.videoWindow.WindowID, &guiIcon);
  if (guiApp.video.Bitmap.Image)
  {
    wsImageResize(&guiApp.videoWindow, guiApp.video.Bitmap.Width, guiApp.video.Bitmap.Height);
    wsImageRender(&guiApp.videoWindow, guiApp.video.Bitmap.Image);
  }
  wsXDNDMakeAwareness(&guiApp.videoWindow);
  guiApp.videoWindow.DrawHandler = uiVideoDraw;
  guiApp.videoWindow.MouseHandler = uiVideoMouse;
  guiApp.videoWindow.KeyHandler = guiApp.mainWindow.KeyHandler;
  guiApp.videoWindow.DNDHandler = guiApp.mainWindow.DNDHandler;
}

void uiVideoDone (void)
{
  wsWindowDestroy(&guiApp.videoWindow);
}
