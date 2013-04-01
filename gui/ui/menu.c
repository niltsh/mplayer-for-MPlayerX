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

/* menu window */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "help_mp.h"
#include "mp_msg.h"
#include "gui/app/app.h"
#include "gui/app/gui.h"
#include "actions.h"
#include "ui.h"
#include "gui/util/mem.h"

#include "gui/dialog/dialog.h"

unsigned char * menuDrawBuffer = NULL;
static int      uiMenuRender = True;
int             menuItem = -1;
int             oldMenuItem = -1;
int             menuX,menuY;

static void uiMenuDraw( void )
{
 uint32_t * buf = NULL;
 uint32_t * drw = NULL;
 int             x,y,tmp;

 if ( !guiApp.menuIsPresent ) return;
 if ( !uiMenuRender && !guiApp.menuWindow.Visible ) return;

 if ( uiMenuRender || menuItem != oldMenuItem )
  {
   memcpy( menuDrawBuffer,guiApp.menu.Bitmap.Image,guiApp.menu.Bitmap.ImageSize );
/* --- */
   if ( menuItem != -1 )
    {
     buf=(uint32_t *)menuDrawBuffer;
     drw=(uint32_t *)guiApp.menuSelected.Bitmap.Image;
     for ( y=guiApp.menuItems[ menuItem ].y; y < guiApp.menuItems[ menuItem ].y + guiApp.menuItems[ menuItem ].height; y++ )
       for ( x=guiApp.menuItems[ menuItem ].x; x < guiApp.menuItems[ menuItem ].x + guiApp.menuItems[ menuItem ].width; x++ )
         {
          tmp=drw[ y * guiApp.menuSelected.width + x ];
          if ( !IS_TRANSPARENT ( tmp ) ) buf[ y * guiApp.menu.width + x ]=tmp;
         }
    }
   oldMenuItem=menuItem;
/* --- */
   wsImageRender( &guiApp.menuWindow,menuDrawBuffer );
   uiMenuRender=False;
  }
 wsImageDraw( &guiApp.menuWindow );
}

static void uiMenuMouse( int Button,int X,int Y,int RX,int RY )
{
 int x,y,i;

 (void) Button;
 (void) RX;
 (void) RY;

 menuItem=-1;
 x=X - guiApp.menuWindow.X;
 y=Y - guiApp.menuWindow.Y;
 if ( ( x < 0 ) || ( y < 0  ) || ( x > guiApp.menu.width ) || ( y > guiApp.menu.height ) )
  {
   wsWindowRedraw( &guiApp.menuWindow );
   return;
  }

 for( i=0;i<=guiApp.IndexOfMenuItems;i++ )
  {
   if ( isInside( x,y,
         guiApp.menuItems[i].x,guiApp.menuItems[i].y,
         guiApp.menuItems[i].x+guiApp.menuItems[i].width,guiApp.menuItems[i].y+guiApp.menuItems[i].height ) ) { menuItem=i; break; }
  }
 wsWindowRedraw( &guiApp.menuWindow );
}

void uiMenuInit( void )
{

 if ( !guiApp.menuIsPresent ) return;

 guiApp.menu.x=0;
 guiApp.menu.y=0;

 if ( ( menuDrawBuffer = calloc( 1,guiApp.menu.Bitmap.ImageSize ) ) == NULL )
  {
    mp_msg( MSGT_GPLAYER,MSGL_DBG2,MSGTR_NEMFMR );
   gtkMessageBox( MSGBOX_FATAL,MSGTR_NEMFMR );
   return;
  }

 wsWindowCreate( &guiApp.menuWindow,
 guiApp.menu.x,guiApp.menu.y,guiApp.menu.width,guiApp.menu.height,
 wsOverredirect|wsHideFrame|wsMaxSize|wsMinSize|wsHideWindow,wsShowMouseCursor|wsHandleMouseButton|wsHandleMouseMove,"MPlayer menu" );

 mp_msg( MSGT_GPLAYER,MSGL_DBG2,"[menu] menuWindow ID: 0x%x\n",(int)guiApp.menuWindow.WindowID );

 wsWindowShape( &guiApp.menuWindow,guiApp.menu.Mask.Image );

 guiApp.menuWindow.DrawHandler=uiMenuDraw;
 guiApp.menuWindow.MouseHandler=uiMenuMouse;
// guiApp.menuWindow.KeyHandler=guiApp.mainWindow.KeyHandler;
 uiMenuRender=True; wsWindowRedraw( &guiApp.menuWindow );
}

void uiMenuDone( void )
{
  nfree(menuDrawBuffer);
  wsWindowDestroy(&guiApp.menuWindow);
}

void uiMenuShow( int mx,int my )
{
 int x,y;

 if ( !guiApp.menuIsPresent ) return;

 x=mx;
 if ( x + guiApp.menuWindow.Width > wsMaxX ) x=wsMaxX - guiApp.menuWindow.Width - 1 + wsOrgX;
 y=my;
 if ( y + guiApp.menuWindow.Height > wsMaxY ) y=wsMaxY - guiApp.menuWindow.Height - 1 + wsOrgY;

 menuX=x; menuY=y;

 menuItem = 0;

 wsWindowMove( &guiApp.menuWindow,True,x,y );
 wsWindowRaiseTop( wsDisplay,guiApp.menuWindow.WindowID );
 wsWindowLayer( wsDisplay,guiApp.menuWindow.WindowID,1 );
 uiMenuRender=True;
 wsWindowVisibility( &guiApp.menuWindow,wsShowWindow );
 wsWindowRedraw( &guiApp.menuWindow );
}

void uiMenuHide( int mx,int my,int w )
{
 int x,y,i=menuItem;

 if ( !guiApp.menuIsPresent ) return;

 x=mx-menuX;
 y=my-menuY;
// x=RX - guiApp.menuWindow.X;
// y=RY - guiApp.menuWindow.Y;

 wsWindowVisibility( &guiApp.menuWindow,wsHideWindow );

 if ( ( x < 0 ) || ( y < 0 ) ) return;

// printf( "---------> %d %d,%d\n",i,x,y );
// printf( "--------> mi: %d,%d %dx%d\n",guiApp.menuItems[i].x,guiApp.menuItems[i].y,guiApp.menuItems[i].width,guiApp.menuItems[i].height );
 if ( isInside( x,y,
        guiApp.menuItems[i].x,guiApp.menuItems[i].y,
        guiApp.menuItems[i].x+guiApp.menuItems[i].width,
        guiApp.menuItems[i].y+guiApp.menuItems[i].height ) )
   {
    uiEvent( guiApp.menuItems[i].message,(float)w );
   }
}
