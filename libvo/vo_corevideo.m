/*
 * CoreVideo video output driver
 * Copyright (c) 2005 Nicolas Plourde <nicolasplourde@gmail.com>
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

#import "vo_corevideo.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <unistd.h>
#include <CoreServices/CoreServices.h>
//special workaround for Apple bug #6267445
//(OSServices Power API disabled in OSServices.h for 64bit systems)
#ifndef __POWER__
#include <CoreServices/../Frameworks/OSServices.framework/Headers/Power.h>
#endif

//MPLAYER
#include "config.h"
#include "fastmemcpy.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "aspect.h"
#include "mp_msg.h"
#include "m_option.h"
#include "mp_fifo.h"
#include "sub/sub.h"
#include "subopt-helper.h"

#include "input/input.h"
#include "input/mouse.h"

#include "osdep/keycodes.h"
#include "osx_common.h"

//Cocoa
static NSDistantObject *mplayerosxProxy;
static id <MPlayerOSXVOProto> mplayerosxProto;
static MPlayerOpenGLView *mpGLView;
static NSAutoreleasePool *autoreleasepool;
static OSType pixelFormat;

//shared memory
static BOOL shared_buffer = false;
#define DEFAULT_BUFFER_NAME "mplayerosx"
static char *buffer_name;

//Screen
static int screen_id = -1;

//image
static unsigned char *image_data;
// For double buffering
static uint8_t image_page = 0;
static unsigned char *image_datas[2];

static uint32_t image_width;
static uint32_t image_height;
static uint32_t image_bytes;
static uint32_t image_stride;
static uint32_t image_format;

static vo_info_t info =
{
	"Mac OS X Core Video",
	"corevideo",
	"Nicolas Plourde <nicolas.plourde@gmail.com>",
	""
};

LIBVO_EXTERN(corevideo)

static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
{
	unsigned char *dst = image_data + image_bytes * (y0 * image_width + x0);
	vo_draw_alpha_func draw = vo_get_draw_alpha(image_format);
	if (!draw) return;
	draw(w,h,src,srca,stride,dst,image_stride);
}

static void free_file_specific(void)
{
	if(shared_buffer)
	{
		[mplayerosxProto stop];
		mplayerosxProto = nil;
		[mplayerosxProxy release];
		mplayerosxProxy = nil;

		if (munmap(image_data, image_height*image_stride) == -1)
			mp_msg(MSGT_VO, MSGL_FATAL, "[vo_corevideo] uninit: munmap failed. Error: %s\n", strerror(errno));

		if (shm_unlink(buffer_name) == -1)
			mp_msg(MSGT_VO, MSGL_FATAL, "[vo_corevideo] uninit: shm_unlink failed. Error: %s\n", strerror(errno));
    } else {
        free(image_datas[0]);
        free(image_datas[1]);
        image_datas[0] = NULL;
        image_datas[1] = NULL;
        image_data = NULL;
    }
}

static void update_screen_info_shared_buffer(void)
{
	NSRect rc = [[NSScreen mainScreen] frame];
	vo_screenwidth  = rc.size.width;
	vo_screenheight = rc.size.height;
	xinerama_x = rc.origin.x;
	xinerama_y = rc.origin.y;
	aspect_save_screenres(vo_screenwidth, vo_screenheight);
}

static int config(uint32_t width, uint32_t height, uint32_t d_width, uint32_t d_height, uint32_t flags, char *title, uint32_t format)
{
	free_file_specific();

	//misc mplayer setup
	image_width = width;
	image_height = height;
	switch (image_format)
	{
		case IMGFMT_RGB24:
			image_bytes = 3;
			break;
		case IMGFMT_ARGB:
		case IMGFMT_BGRA:
			image_bytes = 4;
			break;
		case IMGFMT_YUY2:
		case IMGFMT_UYVY:
			image_bytes = 2;
			break;
	}
	// should be aligned, but that would break the shared buffer
	image_stride = image_width * image_bytes;

	if(!shared_buffer)
	{
		image_data = malloc(image_height*image_stride);
		image_datas[0] = image_data;
		if (vo_doublebuffering)
			image_datas[1] = malloc(image_height*image_stride);
		image_page = 0;

		//config OpenGL View
		[mpGLView configWidth:d_width height:d_height flags:flags];
		[mpGLView reshape];
		[[mpGLView window] setTitle:[NSString stringWithUTF8String:vo_wintitle ? vo_wintitle : title]];
	}
	else
	{
		int shm_fd;
		mp_msg(MSGT_VO, MSGL_INFO, "[vo_corevideo] writing output to a shared buffer "
				"named \"%s\"\n",buffer_name);

		// create shared memory
		shm_fd = shm_open(buffer_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if (shm_fd == -1)
		{
			mp_msg(MSGT_VO, MSGL_FATAL,
				   "[vo_corevideo] failed to open shared memory. Error: %s\n", strerror(errno));
			return 1;
		}


		if (ftruncate(shm_fd, image_height*image_stride) == -1)
		{
			mp_msg(MSGT_VO, MSGL_FATAL,
				   "[vo_corevideo] failed to size shared memory, possibly already in use. Error: %s\n", strerror(errno));
			close(shm_fd);
			shm_unlink(buffer_name);
			return 1;
		}

		image_data = mmap(NULL, image_height*image_stride,
					PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		close(shm_fd);

		if (image_data == MAP_FAILED)
		{
			mp_msg(MSGT_VO, MSGL_FATAL,
				   "[vo_corevideo] failed to map shared memory. Error: %s\n", strerror(errno));
			shm_unlink(buffer_name);
			return 1;
		}

		//connect to mplayerosx
		mplayerosxProxy=[NSConnection rootProxyForConnectionWithRegisteredName:[NSString stringWithUTF8String:buffer_name] host:nil];
		if ([mplayerosxProxy conformsToProtocol:@protocol(MPlayerOSXVOProto)]) {
			[mplayerosxProxy setProtocolForProxy:@protocol(MPlayerOSXVOProto)];
			mplayerosxProto = (id <MPlayerOSXVOProto>)mplayerosxProxy;
			[mplayerosxProto startWithWidth: image_width withHeight: image_height withBytes: image_bytes withAspect:d_width*100/d_height];
		}
		else {
			[mplayerosxProxy release];
			mplayerosxProxy = nil;
			mplayerosxProto = nil;
		}
	}
	return 0;
}

static void check_events(void)
{
	if (mpGLView)
		[mpGLView check_events];
}

static void draw_osd(void)
{
	vo_draw_text(image_width, image_height, draw_alpha);
}

static void flip_page(void)
{
	if(shared_buffer) {
		NSAutoreleasePool *pool = [NSAutoreleasePool new];
		[mplayerosxProto render];
		[pool release];
	} else {
		[mpGLView setCurrentTexture];
		[mpGLView render];
		if (vo_doublebuffering) {
			image_page = 1 - image_page;
			image_data = image_datas[image_page];
		}
	}
}

static int draw_slice(uint8_t *src[], int stride[], int w,int h,int x,int y)
{
	return 0;
}


static int draw_frame(uint8_t *src[])
{
	return 0;
}

static uint32_t draw_image(mp_image_t *mpi)
{
	if (!(mpi->flags & MP_IMGFLAG_DIRECT))
	memcpy_pic(image_data, mpi->planes[0], image_width*image_bytes, image_height, image_stride, mpi->stride[0]);

	return 0;
}

static int query_format(uint32_t format)
{
    const int supportflags = VFCAP_CSP_SUPPORTED | VFCAP_CSP_SUPPORTED_BY_HW | VFCAP_OSD | VFCAP_HWSCALE_UP | VFCAP_HWSCALE_DOWN;
	image_format = format;

    switch(format)
	{
		case IMGFMT_YUY2:
			pixelFormat = kYUVSPixelFormat;
			return supportflags;

		case IMGFMT_UYVY:
			pixelFormat = k2vuyPixelFormat;
			return supportflags;

		case IMGFMT_RGB24:
			pixelFormat = k24RGBPixelFormat;
			return supportflags;

		case IMGFMT_ARGB:
			pixelFormat = k32ARGBPixelFormat;
			return supportflags;

		case IMGFMT_BGRA:
			pixelFormat = k32BGRAPixelFormat;
			return supportflags;
    }
    return 0;
}

static int get_image(mp_image_t *mpi)
{
    if (!(mpi->flags & (MP_IMGFLAG_ACCEPT_STRIDE | MP_IMGFLAG_ACCEPT_WIDTH)) ||
            (mpi->type != MP_IMGTYPE_TEMP && mpi->type != MP_IMGTYPE_STATIC))
        return VO_FALSE;

	// mpi should not be planar format here
	mpi->planes[0] = image_data;
	mpi->stride[0] = image_stride;
	mpi->flags |=  MP_IMGFLAG_DIRECT;
	mpi->flags &= ~MP_IMGFLAG_DRAW_CALLBACK;
	return VO_TRUE;
}

static void uninit(void)
{
    SetSystemUIMode( kUIModeNormal, 0);
    CGDisplayShowCursor(kCGDirectMainDisplay);

    free_file_specific();

    if(mpGLView)
    {
        NSAutoreleasePool *finalPool;
        mpGLView = nil;
        [autoreleasepool release];
        finalPool = [[NSAutoreleasePool alloc] init];
        [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
        [finalPool release];
    }

    free(buffer_name);
    buffer_name = NULL;
}

static const opt_t subopts[] = {
{"device_id",     OPT_ARG_INT,  &screen_id,     NULL},
{"shared_buffer", OPT_ARG_BOOL, &shared_buffer, NULL},
{"buffer_name",   OPT_ARG_MSTRZ,&buffer_name,   NULL},
{NULL}
};

static int preinit(const char *arg)
{

	// set defaults
	screen_id = -1;
	shared_buffer = false;
	buffer_name = NULL;

	if (subopt_parse(arg, subopts) != 0) {
		mp_msg(MSGT_VO, MSGL_FATAL,
				"\n-vo corevideo command line help:\n"
				"Example: mplayer -vo corevideo:device_id=1:shared_buffer:buffer_name=mybuff\n"
				"\nOptions:\n"
				"  device_id=<0-...>\n"
				"    DEPRECATED, use -xineramascreen instead.\n"
				"    Set screen device ID for fullscreen.\n"
				"  shared_buffer\n"
				"    Write output to a shared memory buffer instead of displaying it.\n"
				"  buffer_name=<name>\n"
				"    Name of the shared buffer created with shm_open() as well as\n"
				"    the name of the NSConnection MPlayer will try to open.\n"
				"    Setting buffer_name implicitly enables shared_buffer.\n"
				"\n" );
		return -1;
	}

	autoreleasepool = [[NSAutoreleasePool alloc] init];

	if (screen_id != -1)
		xinerama_screen = screen_id;

	if (!buffer_name)
		buffer_name = strdup(DEFAULT_BUFFER_NAME);
	else
		shared_buffer = true;

	if(!shared_buffer)
	{
		if(!mpGLView)
		{
			mpGLView = [[MPlayerOpenGLView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100) pixelFormat:[MPlayerOpenGLView defaultPixelFormat]];
			[mpGLView autorelease];
		}
		[mpGLView display];
		[mpGLView preinit];
	}

    return 0;
}

static int control(uint32_t request, void *data)
{
	switch (request)
	{
		case VOCTRL_DRAW_IMAGE: return draw_image(data);
		case VOCTRL_QUERY_FORMAT: return query_format(*(uint32_t*)data);
		case VOCTRL_GET_IMAGE: return get_image(data);
		case VOCTRL_ONTOP: vo_ontop = !vo_ontop; if(!shared_buffer){ [mpGLView ontop]; } else { [mplayerosxProto ontop]; } return VO_TRUE;
		case VOCTRL_ROOTWIN: vo_rootwin = !vo_rootwin; [mpGLView rootwin]; return VO_TRUE;
		case VOCTRL_FULLSCREEN: vo_fs = !vo_fs; if(!shared_buffer){ [mpGLView fullscreen: NO]; } else { [mplayerosxProto toggleFullscreen]; } return VO_TRUE;
		case VOCTRL_GET_PANSCAN: return VO_TRUE;
		case VOCTRL_SET_PANSCAN: panscan_calc(); return VO_TRUE;
		case VOCTRL_UPDATE_SCREENINFO:
			if (shared_buffer)
				update_screen_info_shared_buffer();
			else
				[mpGLView update_screen_info];
			return VO_TRUE;
	}
	return VO_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
// NSOpenGLView Subclass
//////////////////////////////////////////////////////////////////////////
@implementation MPlayerOpenGLView
- (void) preinit
{
	CVReturn error;

	[super preinit];

	error = CVOpenGLTextureCacheCreate(NULL, 0, [[self openGLContext] CGLContextObj], [[self pixelFormat] CGLPixelFormatObj], 0, &textureCache);
	if(error != kCVReturnSuccess)
		mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create OpenGL texture Cache(%d)\n", error);
}

- (void) releaseVideoSpecific
{
	CVPixelBufferRelease(frameBuffers[0]);
	frameBuffers[0] = NULL;
	CVPixelBufferRelease(frameBuffers[1]);
	frameBuffers[1] = NULL;
	CVOpenGLTextureRelease(texture);
	texture = NULL;
}

- (void) dealloc
{
	[self releaseVideoSpecific];
	CVOpenGLTextureCacheRelease(textureCache);
	textureCache = NULL;
	[super dealloc];
}

- (void) configWidth:(uint32_t)width height:(uint32_t)height flags:(uint32_t)flags
{
	CVReturn error = kCVReturnSuccess;

	[super configWidth:width height:height flags:flags];

	[self releaseVideoSpecific];
	error = CVPixelBufferCreateWithBytes(NULL, image_width, image_height, pixelFormat, image_datas[0], image_stride, NULL, NULL, NULL, &frameBuffers[0]);
	if(error != kCVReturnSuccess)
		mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create Pixel Buffer(%d)\n", error);
	if (vo_doublebuffering) {
		error = CVPixelBufferCreateWithBytes(NULL, image_width, image_height, pixelFormat, image_datas[1], image_stride, NULL, NULL, NULL, &frameBuffers[1]);
		if(error != kCVReturnSuccess)
			mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create Pixel Double Buffer(%d)\n", error);
	}

	error = CVOpenGLTextureCacheCreateTextureFromImage(NULL, textureCache, frameBuffers[image_page], 0, &texture);
	if(error != kCVReturnSuccess)
		mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create OpenGL texture(%d)\n", error);
}

/*
	Setup OpenGL
*/
- (void)prepareOpenGL
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	[self reshape];
}

/*
	reshape OpenGL viewport
*/
- (void)reshape
{
	int d_width, d_height;

	[super reshape];

	glViewport(0, 0, vo_dwidth, vo_dheight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, vo_dwidth, vo_dheight, 0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	d_width  = vo_dwidth;
	d_height = vo_dheight;
	//set texture frame
	if(aspect_scaling())
	{
		aspect(&d_width, &d_height, A_WINZOOM);
	}
	textureFrame = NSMakeRect((vo_dwidth - d_width) / 2, (vo_dheight - d_height) / 2, d_width, d_height);
}

/*
	Render frame
*/
- (void) render
{
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(CVOpenGLTextureGetTarget(texture));
	glBindTexture(CVOpenGLTextureGetTarget(texture), CVOpenGLTextureGetName(texture));

	glColor3f(1,1,1);
	glBegin(GL_QUADS);
	glTexCoord2f(upperLeft[0], upperLeft[1]); glVertex2i(	textureFrame.origin.x-(vo_panscan_x >> 1), textureFrame.origin.y-(vo_panscan_y >> 1));
	glTexCoord2f(lowerLeft[0], lowerLeft[1]); glVertex2i(textureFrame.origin.x-(vo_panscan_x >> 1), NSMaxY(textureFrame)+(vo_panscan_y >> 1));
	glTexCoord2f(lowerRight[0], lowerRight[1]); glVertex2i(NSMaxX(textureFrame)+(vo_panscan_x >> 1), NSMaxY(textureFrame)+(vo_panscan_y >> 1));
	glTexCoord2f(upperRight[0], upperRight[1]); glVertex2i(NSMaxX(textureFrame)+(vo_panscan_x >> 1), textureFrame.origin.y-(vo_panscan_y >> 1));
	glEnd();
	glDisable(CVOpenGLTextureGetTarget(texture));

	//render resize box
	if(!vo_fs)
	{
		NSRect frame = [self frame];

		glBegin(GL_LINES);
		glColor4f(0.2, 0.2, 0.2, 0.5);
		glVertex2i(frame.size.width-1, frame.size.height-1); glVertex2i(frame.size.width-1, frame.size.height-1);
		glVertex2i(frame.size.width-1, frame.size.height-5); glVertex2i(frame.size.width-5, frame.size.height-1);
		glVertex2i(frame.size.width-1, frame.size.height-9); glVertex2i(frame.size.width-9, frame.size.height-1);

		glColor4f(0.4, 0.4, 0.4, 0.5);
		glVertex2i(frame.size.width-1, frame.size.height-2); glVertex2i(frame.size.width-2, frame.size.height-1);
		glVertex2i(frame.size.width-1, frame.size.height-6); glVertex2i(frame.size.width-6, frame.size.height-1);
		glVertex2i(frame.size.width-1, frame.size.height-10); glVertex2i(frame.size.width-10, frame.size.height-1);

		glColor4f(0.6, 0.6, 0.6, 0.5);
		glVertex2i(frame.size.width-1, frame.size.height-3); glVertex2i(frame.size.width-3, frame.size.height-1);
		glVertex2i(frame.size.width-1, frame.size.height-7); glVertex2i(frame.size.width-7, frame.size.height-1);
		glVertex2i(frame.size.width-1, frame.size.height-11); glVertex2i(frame.size.width-11, frame.size.height-1);
		glEnd();
	}

	glFlush();
}

/*
	Create OpenGL texture from current frame & set texco
*/
- (void) setCurrentTexture
{
	CVReturn error = kCVReturnSuccess;

	CVOpenGLTextureRelease(texture);
	error = CVOpenGLTextureCacheCreateTextureFromImage(NULL, textureCache, frameBuffers[image_page], 0, &texture);
	if(error != kCVReturnSuccess)
		mp_msg(MSGT_VO, MSGL_ERR,"[vo_corevideo] Failed to create OpenGL texture(%d)\n", error);

    CVOpenGLTextureGetCleanTexCoords(texture, lowerLeft, lowerRight, upperRight, upperLeft);
}

/*
	redraw win rect
*/
- (void) drawRect: (NSRect *) bounds
{
	[self render];
}
@end
