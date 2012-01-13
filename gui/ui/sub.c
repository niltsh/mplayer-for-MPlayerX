/*
 * sub window
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

#include <string.h>

#include "config.h"
#include "libvo/x11_common.h"
#include "help_mp.h"
#include "mp_core.h"

#include "gmplayer.h"
#include "gui/app.h"
#include "gui/interface.h"
#include "widgets.h"

int             uiSubRender = 0;
int             subVisible = 0;

void uiSubDraw( void )
{
 if ( guiApp.subWindow.State == wsWindowClosed ) mplayer( MPLAYER_EXIT_GUI, EXIT_QUIT, 0 );

 if ( guiApp.subWindow.State == wsWindowFocusIn ) subVisible++;
 if ( guiApp.subWindow.State == wsWindowFocusOut && metacity_hack != 3 ) subVisible--;

 if ( !guiApp.subWindow.Mapped ||
      guiApp.subWindow.Visible == wsWindowNotVisible ) return;

 if ( guiInfo.Playing ) uiSubRender=0;

 if ( uiSubRender && guiApp.subWindow.State == wsWindowExpose )
  {
   if ( guiApp.sub.Bitmap.Image ) wsPutImage( &guiApp.subWindow );
  }
 guiApp.subWindow.State=0;
}

void uiSubMouseHandle( int Button,int X,int Y,int RX,int RY )
{
 static int mplSubMoved = 0;
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
// ---
   case wsPLMouseButton:
          gtkShow( ivHidePopUpMenu,NULL );
          sx=X; sy=Y;
          msButton=wsPLMouseButton;
          mplSubMoved=0;
          break;
   case wsMoveMouse:
          switch ( msButton )
           {
            case wsPLMouseButton:
                   mplSubMoved=1;
                   if ( !guiApp.subWindow.isFullScreen )
                    {
                     wsMoveWindow( &guiApp.subWindow,True,RX - sx,RY - sy );
                     guiApp.sub.x = guiApp.subWindow.X;
                     guiApp.sub.y = guiApp.subWindow.Y;
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
          if ( ( !mplSubMoved )&&( guiApp.subWindow.isFullScreen ) )
           {
            if( subVisible++%2 ) wsRaiseWindowTop( wsDisplay,guiApp.mainWindow.WindowID );
             else wsRaiseWindowTop( wsDisplay,guiApp.subWindow.WindowID );
	   }
          msButton=0;
          mplSubMoved=0;
          break;
  }
}
