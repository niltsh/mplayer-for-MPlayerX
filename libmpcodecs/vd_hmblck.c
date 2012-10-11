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

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "mp_msg.h"

#include "vd_internal.h"

static const vd_info_t info = {
	"Hauppauge Macroblock Decoder",
	"hmblck",
	"Alex <d18c7db@hotmail.com>, A'rpi, Alex Beregszaszi",
	"Alex <d18c7db@hotmail.com>",
	"uncompressed"
};

LIBVD_EXTERN(hmblck)

static void de_macro_y(unsigned char* dst,unsigned char* src,int dstride,int w,int h){
    int y,x,i;
    // descramble Y plane
    for (y=0; y<h; y+=16) {
        for (x=0; x<w; x+=16) {
            for (i=0; i<16; i++) {
                memcpy(dst + x + (y+i)*dstride, src, 16);
                src+=16;
            }
        }
    }
}

static void de_macro_uv(unsigned char* dstu,unsigned char* dstv,unsigned char* src,int dstride,int w,int h){
    int y,x,i;
    // descramble U/V plane
    for (y=0; y<h; y+=16) {
        for (x=0; x<w; x+=8) {
            for (i=0; i<16; i++) {
		int idx=x + (y+i)*dstride;
		dstu[idx+0]=src[0]; dstv[idx+0]=src[1];
		dstu[idx+1]=src[2]; dstv[idx+1]=src[3];
		dstu[idx+2]=src[4]; dstv[idx+2]=src[5];
		dstu[idx+3]=src[6]; dstv[idx+3]=src[7];
		dstu[idx+4]=src[8]; dstv[idx+4]=src[9];
		dstu[idx+5]=src[10]; dstv[idx+5]=src[11];
		dstu[idx+6]=src[12]; dstv[idx+6]=src[13];
		dstu[idx+7]=src[14]; dstv[idx+7]=src[15];
                src+=16;
            }
        }
    }
}

/*************************************************************************
 * set/get/query special features/parameters
 */
static int control(sh_video_t *sh,int cmd, void *arg,...){
    return CONTROL_UNKNOWN;
}
/*************************************************************************
 * init driver
 */
static int init(sh_video_t *sh){

    if(sh->format != IMGFMT_HM12) return 0;

    if((sh->disp_w&31) != 0 || (sh->disp_h&31) != 0) {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR,
               "hmblck: Image size must be multiple of 32.\n");
        return 0;
    }
    return mpcodecs_config_vo(sh,sh->disp_w,sh->disp_h,sh->format);
}
/*************************************************************************
 * uninit driver
 */
static void uninit(sh_video_t *sh){
}
/*************************************************************************
 * decode a frame
 */
static mp_image_t* decode(sh_video_t *sh,void* data,int len,int flags){
    mp_image_t* mpi;

    if(len<=0) return NULL; // skipped frame

    mpi=mpcodecs_get_image(sh, MP_IMGTYPE_TEMP, MP_IMGFLAG_ACCEPT_STRIDE,
        sh->disp_w, sh->disp_h);
    if(!mpi) return NULL;

    de_macro_y(mpi->planes[0],data,mpi->stride[0],mpi->w,mpi->h);
    de_macro_uv(mpi->planes[1],mpi->planes[2],
                    (unsigned char *)data+mpi->w*mpi->h,mpi->stride[1],
                    mpi->w/2,mpi->h/2);

    return mpi;
}
/*************************************************************************/
