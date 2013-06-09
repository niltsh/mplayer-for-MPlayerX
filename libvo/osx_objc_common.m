/*
 * CoreVideo video output driver
 * Copyright (c) 2005 Nicolas Plourde <nicolasplourde@gmail.com>
 * Copyright (c) 2012 Reimar Döffinger <Reimar.Doeffinger@gmx.de>
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

#import "osx_objc_common.h"
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
//special workaround for Apple bug #6267445
//(OSServices Power API disabled in OSServices.h for 64bit systems)
#ifndef __POWER__
#include <CoreServices/../Frameworks/OSServices.framework/Headers/Power.h>
#endif

//MPLAYER
#include "config.h"
#include "video_out.h"
#include "aspect.h"
#include "mp_msg.h"
#include "mp_fifo.h"

#include "input/input.h"
#include "input/mouse.h"

#include "osdep/keycodes.h"
#include "osx_common.h"

static float winAlpha = 1;

static BOOL isLeopardOrLater;

static NSAutoreleasePool *autoreleasepool;
static MPCommonOpenGLView *oglv;

void (*vo_osx_redraw_func)(void);

int vo_osx_init(void)
{
    autoreleasepool = [[NSAutoreleasePool alloc] init];
    oglv = [[MPCommonOpenGLView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100) pixelFormat:[MPCommonOpenGLView defaultPixelFormat]];
    [oglv autorelease];
    [oglv display];
    [oglv preinit];
    return 1;
}

void vo_osx_uninit(void)
{
    NSAutoreleasePool *finalPool;
    oglv = nil;
    [autoreleasepool release];
    finalPool = [[NSAutoreleasePool alloc] init];
    [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
    [finalPool release];
}

int vo_osx_config(uint32_t width, uint32_t height, uint32_t flags)
{
    [oglv config:width:height:flags];
    return 1;
}

void vo_osx_ontop(void)
{
    vo_ontop = !vo_ontop;
    [oglv ontop];
}

void vo_osx_fullscreen(void)
{
    vo_fs = !vo_fs;
    [oglv fullscreen:YES];
}

int vo_osx_check_events(void)
{
    return [oglv check_events];
}

void vo_osx_update_xinerama_info(void)
{
    [oglv update_screen_info];
}

void vo_osx_swap_buffers(void)
{
    [oglv->glContext flushBuffer];
}

@implementation MPCommonOpenGLView
- (void) update_screen_info
{
	int screen_id = xinerama_screen;
	NSArray *screen_array = [NSScreen screens];
	NSScreen *screen_handle;
	NSRect screen_frame;

	if(screen_id >= (int)[screen_array count])
	{
		mp_msg(MSGT_VO, MSGL_INFO, "[vo_corevideo] Device ID %d does not exist, falling back to main device\n", screen_id);
		screen_id = -1;
	}
	if (screen_id < 0 && [self window])
		screen_handle = [[self window] screen];
	else
		screen_handle = [screen_array objectAtIndex:(screen_id < 0 ? 0 : screen_id)];

	screen_frame = [screen_handle frame];
	vo_screenwidth = screen_frame.size.width;
	vo_screenheight = screen_frame.size.height;
	xinerama_x = screen_frame.origin.x;
	xinerama_y = screen_frame.origin.y;
	aspect_save_screenres(vo_screenwidth, vo_screenheight);
}

- (void) preinit
{
	GLint swapInterval = 1;

	NSApplicationLoad();
	NSApp = [NSApplication sharedApplication];
	isLeopardOrLater = floor(NSAppKitVersionNumber) > 824;

	osx_foreground_hack();

	// Install an event handler so the Quit menu entry works
	// The proper way using NSApp setDelegate: and
	// applicationShouldTerminate: does not work,
	// probably NSApplication never installs its handler.
	[[NSAppleEventManager sharedAppleEventManager]
		setEventHandler:self
		andSelector:@selector(handleQuitEvent:withReplyEvent:)
		forEventClass:kCoreEventClass
		andEventID:kAEQuitApplication];

	//init menu
	[self initMenu];

	//create window
	window = [[NSWindow alloc]	initWithContentRect:NSMakeRect(0, 0, 100, 100)
								styleMask:NSTitledWindowMask|NSTexturedBackgroundWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask
								backing:NSBackingStoreBuffered defer:NO];

	[window autorelease];
	[window setDelegate:self];
	[window setContentView:self];
	[window setInitialFirstResponder:self];
	[window setAcceptsMouseMovedEvents:YES];
	[window setTitle:@"MPlayer - The Movie Player"];

	winSizeMult = 1;

	//create OpenGL Context
	glContext = [[NSOpenGLContext alloc] initWithFormat:[NSOpenGLView defaultPixelFormat] shareContext:nil];

	[self setOpenGLContext:glContext];
	[glContext setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];
	[glContext setView:self];
	[glContext makeCurrentContext];
}

- (void) dealloc
{
	[glContext release];
	[self setOpenGLContext:nil];
	[super dealloc];
}

- (void) config:(uint32_t)width:(uint32_t)height:(uint32_t)flags
{
	if (flags & VOFLAG_HIDDEN)
		return;
	config_movie_aspect((float)width/height);

	vo_dwidth  = width  *= self->winSizeMult;
	vo_dheight = height *= self->winSizeMult;

	//config window
	[window setContentSize:NSMakeSize(vo_dwidth, vo_dheight)];

	// Do not use visibleFrame - taking the menu bar and dock into account
	// would be nicer in principle, but currently only results in the window
	// being placed strangely off-center since vo_dx/vo_dy calculation is
	// not aware of it.
	// Also flip vo_dy since the screen origin is in the bottom left on OSX.
	[self update_screen_info];
	[window setFrameTopLeftPoint:NSMakePoint(
		vo_dx,
		2*xinerama_y + vo_screenheight - vo_dy)];

	vo_fs = flags & VOFLAG_FULLSCREEN;

	[self rootwin];

	if(vo_fs)
		[self fullscreen: NO];

	[self ontop];

	//show window
	[window makeKeyAndOrderFront:self];
}

- (void) drawRect: (NSRect *) bounds
{
	if (vo_osx_redraw_func) vo_osx_redraw_func();
}

- (void)reshape
{
	NSRect frame = [self frame];
	vo_dwidth  = frame.size.width;
	vo_dheight = frame.size.height;
	event_flags |= VO_EVENT_RESIZE;
}

/*
	Init Menu
*/
- (void)initMenu
{
	NSMenu *mainMenu = [[NSMenu alloc] init];
	NSMenu *menu, *aspectMenu;
	NSMenuItem *menuItem;

	menu = [[NSMenu alloc] init];
	menuItem = [[NSMenuItem alloc] init];
	[menuItem setSubmenu:menu];
	[mainMenu addItem:menuItem];
	// Note: setAppleMenu seems to be unnecessary from 10.6 on,
	// but is needed for all earlier versions or the menu is
	// messed up.
	// Round-about way with performSelector used to avoid compiler
	// warnings.
	[NSApp performSelector:@selector(setAppleMenu:) withObject:menu];

//Create Movie Menu
	menu = [[NSMenu alloc] initWithTitle:@"Movie"];
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Half Size" action:@selector(menuAction:) keyEquivalent:@"0"]; [menu addItem:menuItem];
	kHalfScreenCmd = menuItem;
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Normal Size" action:@selector(menuAction:) keyEquivalent:@"1"]; [menu addItem:menuItem];
	kNormalScreenCmd = menuItem;
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Double Size" action:@selector(menuAction:) keyEquivalent:@"2"]; [menu addItem:menuItem];
	kDoubleScreenCmd = menuItem;
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Full Size" action:@selector(menuAction:) keyEquivalent:@"f"]; [menu addItem:menuItem];
	kFullScreenCmd = menuItem;
	menuItem = [NSMenuItem separatorItem]; [menu addItem:menuItem];

	aspectMenu = [[NSMenu alloc] initWithTitle:@"Aspect Ratio"];
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Keep" action:@selector(menuAction:) keyEquivalent:@""]; [aspectMenu addItem:menuItem];
	if(vo_keepaspect) [menuItem setState:NSOnState];
	kKeepAspectCmd = menuItem;
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Pan-Scan" action:@selector(menuAction:) keyEquivalent:@""]; [aspectMenu addItem:menuItem];
	if(vo_panscan) [menuItem setState:NSOnState];
	kPanScanCmd = menuItem;
	menuItem = [NSMenuItem separatorItem]; [aspectMenu addItem:menuItem];
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Original" action:@selector(menuAction:) keyEquivalent:@""]; [aspectMenu addItem:menuItem];
	kAspectOrgCmd = menuItem;
	menuItem = [[NSMenuItem alloc] initWithTitle:@"4:3" action:@selector(menuAction:) keyEquivalent:@""]; [aspectMenu addItem:menuItem];
	kAspectFullCmd = menuItem;
	menuItem = [[NSMenuItem alloc] initWithTitle:@"16:9" action:@selector(menuAction:) keyEquivalent:@""];	[aspectMenu addItem:menuItem];
	kAspectWideCmd = menuItem;
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Aspect Ratio" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:aspectMenu];
	[menu addItem:menuItem];
	[aspectMenu release];

	//Add to menubar
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Movie" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:menu];
	[mainMenu addItem:menuItem];

//Create Window Menu
	menu = [[NSMenu alloc] initWithTitle:@"Window"];

	menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"]; [menu addItem:menuItem];
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""]; [menu addItem:menuItem];

	//Add to menubar
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:menu];
	[mainMenu addItem:menuItem];
	[NSApp setWindowsMenu:menu];
	[NSApp setMainMenu:mainMenu];

	[menu release];
	[menuItem release];
}

- (void)set_winSizeMult:(float)mult
{
    NSRect frame;
    int d_width, d_height;
    aspect(&d_width, &d_height, A_NOZOOM);

    if (vo_fs) {
        vo_fs = !vo_fs;
        [self fullscreen:NO];
    }

    winSizeMult = mult;
    frame.size.width  = d_width  * mult;
    frame.size.height = d_height * mult;
    [window setContentSize: frame.size];
    [self reshape];
}

/*
	Menu Action
 */
- (void)menuAction:(id)sender
{
	if(sender == kHalfScreenCmd)
		[self set_winSizeMult: 0.5];
	if(sender == kNormalScreenCmd)
		[self set_winSizeMult: 1];
	if(sender == kDoubleScreenCmd)
		[self set_winSizeMult: 2];
	if(sender == kFullScreenCmd)
	{
		vo_fs = !vo_fs;
		[self fullscreen:NO];
	}

	if(sender == kKeepAspectCmd)
	{
		vo_keepaspect = !vo_keepaspect;
		if(vo_keepaspect)
			[kKeepAspectCmd setState:NSOnState];
		else
			[kKeepAspectCmd setState:NSOffState];

		[self reshape];
	}

	if(sender == kPanScanCmd)
	{
		vo_panscan = !vo_panscan;
		if(vo_panscan)
			[kPanScanCmd setState:NSOnState];
		else
			[kPanScanCmd setState:NSOffState];

		panscan_calc();
	}

	if(sender == kAspectOrgCmd)
		change_movie_aspect(-1);

	if(sender == kAspectFullCmd)
		change_movie_aspect(4.0f/3.0f);

	if(sender == kAspectWideCmd)
		change_movie_aspect(16.0f/9.0f);
}

/*
	Toggle Fullscreen
*/
- (void) fullscreen: (BOOL) animate
{
	static NSRect old_frame;
	static NSRect old_view_frame;

	panscan_calc();

	//go fullscreen
	if(vo_fs)
	{
		if(!vo_rootwin)
		{
			SetSystemUIMode( kUIModeAllHidden, kUIOptionAutoShowMenuBar);
			CGDisplayHideCursor(kCGDirectMainDisplay);
			mouseHide = YES;
		}

		old_frame = [window frame];	//save main window size & position
		[self update_screen_info];

		[window setFrame:NSMakeRect(xinerama_x, xinerama_y, vo_screenwidth, vo_screenheight) display:YES animate:animate]; //zoom-in window with nice useless sfx
		old_view_frame = [self bounds];

		[self setFrame:NSMakeRect(0, 0, vo_screenwidth, vo_screenheight)];
		[self setNeedsDisplay:YES];
		[window setHasShadow:NO];
	}
	else
	{
		SetSystemUIMode( kUIModeNormal, 0);

		CGDisplayShowCursor(kCGDirectMainDisplay);
		mouseHide = NO;

		//revert window to previous setting
		[self setFrame:old_view_frame];
		[self setNeedsDisplay:YES];
		[window setHasShadow:YES];
		[window setFrame:old_frame display:YES animate:animate];//zoom-out window with nice useless sfx
	}
}

/*
	Toggle ontop
*/
- (void) ontop
{
	if(vo_ontop)
	{
		[window setLevel:NSScreenSaverWindowLevel];
	}
	else
	{
		[window setLevel:NSNormalWindowLevel];
	}
}

/*
	Toggle rootwin
 */
- (void) rootwin
{
	if(vo_rootwin)
	{
		[window setLevel:CGWindowLevelForKey(kCGDesktopWindowLevelKey)];
		[window orderBack:self];
	}
	else
	{
		[window setLevel:NSNormalWindowLevel];
	}
}

/*
	Check event for new event
*/
- (int) check_events
{
	NSEvent *event;
	int curTime = TickCount()/60;

	//automatically hide mouse cursor (and future on-screen control?)
	if(vo_fs && !mouseHide && !vo_rootwin)
	{
		if(curTime - lastMouseHide >= 5 || lastMouseHide == 0)
		{
			CGDisplayHideCursor(kCGDirectMainDisplay);
			mouseHide = TRUE;
			lastMouseHide = curTime;
		}
	}

	//update activity every 30 seconds to prevent
	//screensaver from starting up.
	if(curTime - lastScreensaverUpdate >= 30 || lastScreensaverUpdate == 0)
	{
		UpdateSystemActivity(UsrActivity);
		lastScreensaverUpdate = curTime;
	}

	event_flags = 0;
	event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSEventTrackingRunLoopMode dequeue:YES];
	if (event == nil)
		return event_flags;
	[NSApp sendEvent:event];
	// Without SDL's bootstrap code (include SDL.h in mplayer.c),
	// on Leopard, we have trouble to get the play window automatically focused
	// when the app is actived. The Following code fix this problem.
#ifndef CONFIG_SDL
	if (isLeopardOrLater && [event type] == NSAppKitDefined
			&& [event subtype] == NSApplicationActivatedEventType) {
		[window makeMainWindow];
		[window makeKeyAndOrderFront:self];
	}
#endif
	return event_flags;
}

/*
	From NSView, respond to key equivalents.
*/
- (BOOL)performKeyEquivalent:(NSEvent *)theEvent
{
	switch([theEvent keyCode])
	{
		case 0x21: [window setAlphaValue: winAlpha-=0.05]; return YES;
		case 0x1e: [window setAlphaValue: winAlpha+=0.05]; return YES;
	}
	return NO;
}

/*
	Process key event
*/
- (void) keyDown: (NSEvent *) theEvent
{
	int key = convert_key([theEvent keyCode], *[[theEvent characters] UTF8String]);
	if (key != -1)
	mplayer_put_key(key);
}

/*
	Process mouse button event
*/
- (void) mouseMoved: (NSEvent *) theEvent
{
	if(vo_fs && !vo_rootwin)
	{
		CGDisplayShowCursor(kCGDirectMainDisplay);
		mouseHide = NO;
	}
	if (enable_mouse_movements && !vo_rootwin) {
		NSPoint p =[self convertPoint:[theEvent locationInWindow] fromView:nil];
		if ([self mouse:p inRect:[self frame]]) {
			vo_mouse_movement(p.x, [self frame].size.height - p.y);
		}
	}
}

- (void) mouseDown: (NSEvent *) theEvent
{
	[self mouseEvent: theEvent];
}

- (void) mouseUp: (NSEvent *) theEvent
{
	[self mouseEvent: theEvent];
}

- (void) rightMouseDown: (NSEvent *) theEvent
{
	[self mouseEvent: theEvent];
}

- (void) rightMouseUp: (NSEvent *) theEvent
{
	[self mouseEvent: theEvent];
}

- (void) otherMouseDown: (NSEvent *) theEvent
{
	[self mouseEvent: theEvent];
}

- (void) otherMouseUp: (NSEvent *) theEvent
{
	[self mouseEvent: theEvent];
}

- (void) scrollWheel: (NSEvent *) theEvent
{
	if([theEvent deltaY] > 0)
		mplayer_put_key(MOUSE_BTN3);
	else
		mplayer_put_key(MOUSE_BTN4);
}

- (void) mouseEvent: (NSEvent *) theEvent
{
	if ( [theEvent buttonNumber] >= 0 && [theEvent buttonNumber] <= 9 )
	{
		int buttonNumber = [theEvent buttonNumber];
		// Fix to mplayer defined button order: left, middle, right
		if (buttonNumber == 1)
			buttonNumber = 2;
		else if (buttonNumber == 2)
			buttonNumber = 1;
		switch([theEvent type])
		{
			case NSLeftMouseDown:
			case NSRightMouseDown:
			case NSOtherMouseDown:
				mplayer_put_key((MOUSE_BTN0 + buttonNumber) | MP_KEY_DOWN);
				break;
			case NSLeftMouseUp:
			case NSRightMouseUp:
			case NSOtherMouseUp:
				mplayer_put_key(MOUSE_BTN0 + buttonNumber);
				break;
		}
	}
}

/*
	NSResponder
*/
- (BOOL) acceptsFirstResponder
{
	return YES;
}

- (BOOL) becomeFirstResponder
{
	return YES;
}

- (BOOL) resignFirstResponder
{
	return YES;
}

- (BOOL)windowShouldClose:(id)sender
{
	mplayer_put_key(KEY_CLOSE_WIN);
	// We have to wait for MPlayer to handle this,
	// otherwise we are in trouble if the
	// KEY_CLOSE_WIN handler is disabled
	return NO;
}

- (void)handleQuitEvent:(NSAppleEventDescriptor*)e withReplyEvent:(NSAppleEventDescriptor*)r
{
	mplayer_put_key(KEY_CLOSE_WIN);
}
@end
