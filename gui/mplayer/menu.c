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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "help_mp.h"
#include "mp_msg.h"
#include "gui/app.h"
#include "gmplayer.h"

#include "widgets.h"

unsigned char * mplMenuDrawBuffer = NULL;
int             mplMenuRender = 1;
int             mplMenuItem = -1;
int             mplOldMenuItem = -1;
int             mplMenuX,mplMenuY;
static int      mplMenuIsInitialized = 0;

static void mplMenuDraw( void )
{
 uint32_t * buf = NULL;
 uint32_t * drw = NULL;
 int             x,y,tmp;

 if ( !appMPlayer.menuIsPresent || !appMPlayer.menu.Bitmap.Image ) return;
 if ( !mplMenuRender && !appMPlayer.menuWindow.Visible ) return;

 if ( mplMenuRender || mplMenuItem != mplOldMenuItem )
  {
   memcpy( mplMenuDrawBuffer,appMPlayer.menu.Bitmap.Image,appMPlayer.menu.Bitmap.ImageSize );
// ---
   if ( mplMenuItem != -1 )
    {
     buf=(uint32_t *)mplMenuDrawBuffer;
     drw=(uint32_t *)appMPlayer.menuSelected.Bitmap.Image;
     for ( y=appMPlayer.menuItems[ mplMenuItem ].y; y < appMPlayer.menuItems[ mplMenuItem ].y + appMPlayer.menuItems[ mplMenuItem ].height; y++ )
       for ( x=appMPlayer.menuItems[ mplMenuItem ].x; x < appMPlayer.menuItems[ mplMenuItem ].x + appMPlayer.menuItems[ mplMenuItem ].width; x++ )
         {
          tmp=drw[ y * appMPlayer.menuSelected.width + x ];
          if ( !IS_TRANSPARENT ( tmp ) ) buf[ y * appMPlayer.menu.width + x ]=tmp;
         }
    }
   mplOldMenuItem=mplMenuItem;
// ---
   wsConvert( &appMPlayer.menuWindow,mplMenuDrawBuffer );
   mplMenuRender=0;
  }
 wsPutImage( &appMPlayer.menuWindow );
}

void mplMenuMouseHandle( int X,int Y,int RX,int RY )
{
 int x,y,i;

 if ( !appMPlayer.menu.Bitmap.Image ) return;

 mplMenuItem=-1;
 x=RX - appMPlayer.menuWindow.X;
 y=RY - appMPlayer.menuWindow.Y;
 if ( ( x < 0 ) || ( y < 0  ) || ( x > appMPlayer.menu.width ) || ( y > appMPlayer.menu.height ) )
  {
   wsPostRedisplay( &appMPlayer.menuWindow );
   return;
  }

 for( i=0;i<=appMPlayer.IndexOfMenuItems;i++ )
  {
   if ( wgIsRect( x,y,
         appMPlayer.menuItems[i].x,appMPlayer.menuItems[i].y,
         appMPlayer.menuItems[i].x+appMPlayer.menuItems[i].width,appMPlayer.menuItems[i].y+appMPlayer.menuItems[i].height ) ) { mplMenuItem=i; break; }
  }
 wsPostRedisplay( &appMPlayer.menuWindow );
}

void mplShowMenu( int mx,int my )
{
 int x,y;

 if ( !appMPlayer.menuIsPresent || !appMPlayer.menu.Bitmap.Image ) return;

 x=mx;
 if ( x + appMPlayer.menuWindow.Width > wsMaxX ) x=wsMaxX - appMPlayer.menuWindow.Width - 1 + wsOrgX;
 y=my;
 if ( y + appMPlayer.menuWindow.Height > wsMaxY ) y=wsMaxY - appMPlayer.menuWindow.Height - 1 + wsOrgY;

 mplMenuX=x; mplMenuY=y;

 mplMenuItem = 0;

 wsMoveWindow( &appMPlayer.menuWindow,False,x,y );
 wsMoveTopWindow( wsDisplay,appMPlayer.menuWindow.WindowID );
 wsSetLayer( wsDisplay,appMPlayer.menuWindow.WindowID,1 );
 mplMenuRender=1;
 wsVisibleWindow( &appMPlayer.menuWindow,wsShowWindow );
 wsPostRedisplay( &appMPlayer.menuWindow );
}

void mplHideMenu( int mx,int my,int w )
{
 int x,y,i=mplMenuItem;

 if ( !appMPlayer.menuIsPresent || !appMPlayer.menu.Bitmap.Image ) return;

 x=mx-mplMenuX;
 y=my-mplMenuY;
// x=RX - appMPlayer.menuWindow.X;
// y=RY - appMPlayer.menuWindow.Y;

 wsVisibleWindow( &appMPlayer.menuWindow,wsHideWindow );

 if ( ( x < 0 ) || ( y < 0 ) ) return;

// printf( "---------> %d %d,%d\n",i,x,y );
// printf( "--------> mi: %d,%d %dx%d\n",appMPlayer.menuItems[i].x,appMPlayer.menuItems[i].y,appMPlayer.menuItems[i].width,appMPlayer.menuItems[i].height );
 if ( wgIsRect( x,y,
        appMPlayer.menuItems[i].x,appMPlayer.menuItems[i].y,
        appMPlayer.menuItems[i].x+appMPlayer.menuItems[i].width,
        appMPlayer.menuItems[i].y+appMPlayer.menuItems[i].height ) )
   {
    mplEventHandling( appMPlayer.menuItems[i].message,(float)w );
   }
}

void mplMenuInit( void )
{

 if ( mplMenuIsInitialized || !appMPlayer.menuIsPresent || !appMPlayer.menu.Bitmap.Image ) return;

 appMPlayer.menu.x=0;
 appMPlayer.menu.y=0;

 if ( ( mplMenuDrawBuffer = calloc( 1,appMPlayer.menu.Bitmap.ImageSize ) ) == NULL )
  {
#ifdef DEBUG
    mp_msg( MSGT_GPLAYER,MSGL_DBG2,MSGTR_NEMFMR );
#endif
   gtkMessageBox( GTK_MB_FATAL,MSGTR_NEMFMR );
   return;
  }

 wsCreateWindow( &appMPlayer.menuWindow,
 appMPlayer.menu.x,appMPlayer.menu.y,appMPlayer.menu.width,appMPlayer.menu.height,
 wsNoBorder,wsShowMouseCursor|wsHandleMouseButton|wsHandleMouseMove,wsOverredirect|wsHideFrame|wsMaxSize|wsMinSize|wsHideWindow,"MPlayer menu" );

 wsSetShape( &appMPlayer.menuWindow,appMPlayer.menu.Mask.Image );

#ifdef DEBUG
  mp_msg( MSGT_GPLAYER,MSGL_DBG2,"menu: 0x%x\n",(int)appMPlayer.menuWindow.WindowID );
#endif

 mplMenuIsInitialized=1;
 appMPlayer.menuWindow.ReDraw=mplMenuDraw;
// appMPlayer.menuWindow.MouseHandler=mplMenuMouseHandle;
// appMPlayer.menuWindow.KeyHandler=mplMainKeyHandle;
 mplMenuRender=1; wsPostRedisplay( &appMPlayer.menuWindow );
}
