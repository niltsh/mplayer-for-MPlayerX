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
#include <string.h>
#include <inttypes.h>

#include "skin.h"
#include "font.h"
#include "cut.h"
#include "mp_msg.h"
#include "libavutil/avstring.h"

bmpFont * Fonts[MAX_FONTS] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL };

int fntAddNewFont( char * name )
{
 int id;
 int i;

 for( id=0;id<MAX_FONTS;id++ )
   if ( !Fonts[id] ) break;

 if ( id == MAX_FONTS ) return -2;

 if ( ( Fonts[id]=calloc( 1,sizeof( bmpFont ) ) ) == NULL ) return -1;

 av_strlcpy( Fonts[id]->name,name,MAX_FONT_NAME );
 for ( i=0;i<ASCII_CHRS+EXTRA_CHRS;i++ )
   Fonts[id]->Fnt[i].x=Fonts[id]->Fnt[i].y=Fonts[id]->Fnt[i].sx=Fonts[id]->Fnt[i].sy=-1;

 return id;
}

void fntFreeFont( void )
{
 int i;
 for( i=0;i < MAX_FONTS;i++ )
  {
   if ( Fonts[i] )
    {
     free( Fonts[i]->Bitmap.Image );
     free( Fonts[i] );
     Fonts[i]=NULL;
    }
  }
}

int fntRead( char * path,char * fname )
{
 FILE * f;
 unsigned char   tmp[512];
 unsigned char * ptmp;
 unsigned char   command[32];
 unsigned char   param[256];
 int             c;
 int             id = fntAddNewFont( fname );

 if ( id < 0 ) return id;

 av_strlcpy( tmp,path,sizeof( tmp ) );
 av_strlcat( tmp,fname,sizeof( tmp ) ); av_strlcat( tmp,".fnt",sizeof( tmp ) );
 if ( ( f=fopen( tmp,"rt" ) ) == NULL )
   { free( Fonts[id] ); return -3; }

 while ( fgets( tmp,255,f ) )
  {
   // remove any kind of newline, if any
   tmp[strcspn(tmp, "\n\r")] = 0;
   for ( c=0;c < (int)strlen( tmp );c++ )
     if ( tmp[c] == ';' ) { tmp[c]=0; break; }
   if ( !tmp[0] ) continue;
   ptmp=trimleft( tmp );
   if ( !tmp[0] ) continue;
   ptmp=strswap( ptmp,'\t',' ' );
   ptmp=trim( ptmp );
   cutItem( ptmp,command,'=',0 ); cutItem( ptmp,param,'=',1 );
   if ( command[0] == '"' )
    {
     int i;
     cutItem( command,command,'"',1 );
     if ( !command[0] ) i='"';
     else if ( command[0] & 0x80 )
      {
       for ( i = 0; i < EXTRA_CHRS; i++ )
        {
         if ( !Fonts[id]->nonASCIIidx[i][0] )
          {
           strncpy( Fonts[id]->nonASCIIidx[i], command, 4 );
           break;
          }
        }
       if ( i == EXTRA_CHRS ) continue;
       i += ASCII_CHRS;
      }
     else i=command[0];
     cutItem( param,tmp,',',0 ); Fonts[id]->Fnt[i].x=atoi( tmp );
     cutItem( param,tmp,',',1 ); Fonts[id]->Fnt[i].y=atoi( tmp );
     cutItem( param,tmp,',',2 ); Fonts[id]->Fnt[i].sx=atoi( tmp );
     cutItem( param,tmp,',',3 ); Fonts[id]->Fnt[i].sy=atoi( tmp );
     mp_dbg( MSGT_GPLAYER,MSGL_DBG2,"[font]  char: '%s' params: %d,%d %dx%d\n",command,Fonts[id]->Fnt[i].x,Fonts[id]->Fnt[i].y,Fonts[id]->Fnt[i].sx,Fonts[id]->Fnt[i].sy );
    }
    else
     {
      if ( !strcmp( command,"image" ) )
       {
        av_strlcpy( tmp,path,sizeof( tmp )  ); av_strlcat( tmp,param,sizeof( tmp ) );
        mp_dbg( MSGT_GPLAYER,MSGL_DBG2,"[font] font imagefile: %s\n",tmp );
        if ( skinBPRead( tmp,&Fonts[id]->Bitmap ) )
         {
          fclose(f);
          return -4;
         }
       }
     }
   }

 fclose(f);
 return 0;
}

int fntFindID( char * name )
{
 int i;
 for ( i=0;i < MAX_FONTS;i++ )
   if ( Fonts[i] )
     if ( !strcmp( name,Fonts[i]->name ) ) return i;
 return -1;
}

// get Fnt index of character (utf8 or normal one) *str points to,
// then move pointer to next/previous character
int fntGetCharIndex( int id, unsigned char **str, gboolean utf8, int direction )
{
 unsigned char *p, uchar[4] = { 0, 0, 0, 0 };
 int i, c = -1;

 if ( **str & 0x80 )
  {
   if ( utf8 )
    {
     p = *str;
     *str = g_utf8_next_char( *str );
     strncpy( uchar, p, *str - p );

     if ( direction < 0 ) *str = g_utf8_prev_char( p );
    }
   else
    {
     uchar[0] = **str;
     *str += direction;
    }

   for ( i = 0; ( i < EXTRA_CHRS ) && Fonts[id]->nonASCIIidx[i][0]; i++ )
    {
     if ( strncmp( Fonts[id]->nonASCIIidx[i], uchar, 4 ) == 0 ) return i + ASCII_CHRS;
     if ( !utf8 && ( Fonts[id]->nonASCIIidx[i][0] == (*uchar >> 6 | 0xc0) && Fonts[id]->nonASCIIidx[i][1] == (*uchar & 0x3f | 0x80) && Fonts[id]->nonASCIIidx[i][2] == 0 ) ) c = i + ASCII_CHRS;
    }
  }
 else
  {
   c = **str;

   if ( utf8 && ( direction < 0 ) ) *str = g_utf8_prev_char( *str );
   else *str += direction;
  }

 return c;
}

int fntTextWidth( int id,char * str )
{
 int size = 0;
 gboolean utf8;
 unsigned char *p;

 utf8 = g_utf8_validate( str, -1, NULL);
 p = (unsigned char *) str;

 while ( *p )
  {
   int c = fntGetCharIndex( id, &p, utf8, 1 );
   if ( c == -1 || Fonts[id]->Fnt[c].sx == -1 ) c = ' ';
   size+= Fonts[id]->Fnt[ c ].sx;
  }
 return size;
}

int fntTextHeight( int id,char * str )
{
 int max = 0;
 gboolean utf8;
 unsigned char *p;

 utf8 = g_utf8_validate( str, -1, NULL);
 p = (unsigned char *) str;

 while ( *p )
  {
   int h;
   int c = fntGetCharIndex( id, &p, utf8, 1 );
   if ( c == -1 || Fonts[id]->Fnt[c].sx == -1 ) c = ' ';
   h = Fonts[id]->Fnt[c].sy;
   if ( h > max ) max=h;
  }
 return max;
}

txSample * fntRender( wItem * item,int px,char * txt )
{
 unsigned char * u;
 int 	         c, i, dx = 0, tw, fbw, iw, id, ofs;
 int 		 x,y,fh,fw,fyc,yc;
 uint32_t      * ibuf;
 uint32_t      * obuf;
 gboolean        utf8;

 id=item->fontid;
 tw=fntTextWidth( id,txt );

 if ( !tw ) return NULL;

 iw=item->width;
 fbw=Fonts[id]->Bitmap.Width;

 if ( item->Bitmap.Image == NULL )
  {
   item->Bitmap.Height=item->height=fntTextHeight( id,txt );
   item->Bitmap.Width=item->width=iw;
   item->Bitmap.ImageSize=item->height * iw * 4;
   if ( !item->Bitmap.ImageSize ) return NULL;
   item->Bitmap.BPP=32;
   item->Bitmap.Image=malloc( item->Bitmap.ImageSize );
  }

 obuf=(uint32_t *)item->Bitmap.Image;
 ibuf=(uint32_t *)Fonts[id]->Bitmap.Image;

 for ( i=0;i < item->Bitmap.ImageSize / 4;i++ ) obuf[i]=0xff00ff;

 if ( tw <= iw )
  {
   switch ( item->align )
    {
     default:
     case fntAlignLeft:   dx=0; break;
     case fntAlignCenter: dx=( iw - tw ) / 2; break;
     case fntAlignRight:  dx=iw - tw; break;
    }

  } else dx+=px;

 ofs=dx;

 utf8 = g_utf8_validate( txt, -1, NULL);
 u = txt;

 while ( *u )
  {
   c = fntGetCharIndex( id, &u, utf8, 1 );

   if ( c != -1 ) fw=Fonts[id]->Fnt[c].sx;

   if ( c == -1 || fw == -1 ) { c=' '; fw=Fonts[id]->Fnt[c].sx; }

   fh=Fonts[id]->Fnt[c].sy;
   fyc=Fonts[id]->Fnt[c].y * fbw + Fonts[id]->Fnt[c].x;
   yc=dx;

   if ( dx >= 0 )
    for ( y=0;y < fh;y++ )
     {
      for ( x=0; x < fw;x++ )
       if ( dx + x >= 0 && dx + x < iw ) obuf[yc + x]=ibuf[ fyc + x ];
      fyc+=fbw;
      yc+=iw;
     }
   dx+=fw;
  }

 if ( ofs > 0 && tw > item->width )
  {
   dx=ofs;
   u = txt + strlen( txt );

   while ( u > (unsigned char *) txt )
    {
     c = fntGetCharIndex( id, &u, utf8, -1 );

     if ( c != -1) fw=Fonts[id]->Fnt[c].sx;

     if ( c == -1 || fw == -1 ) { c=' '; fw=Fonts[id]->Fnt[c].sx; }

     fh=Fonts[id]->Fnt[c].sy;
     fyc=Fonts[id]->Fnt[c].y * fbw + Fonts[id]->Fnt[c].x;

     dx-=fw; yc=dx;
     if ( dx >= 0 )
      for ( y=0;y < fh;y++ )
       {
        for ( x=fw - 1;x >= 0;x-- )
         if ( dx + x >= 0 && dx + x < iw ) obuf[yc + x]=ibuf[fyc + x];
        fyc+=fbw;
	yc+=iw;
       }
    }
  }

 return &item->Bitmap;
}
