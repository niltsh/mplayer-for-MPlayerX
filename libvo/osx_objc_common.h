/*
 * CoreVideo video output driver
 *
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

#ifndef MPLAYER_OSX_OBJC_COMMON_H
#define MPLAYER_OSX_OBJC_COMMON_H

#import <Cocoa/Cocoa.h>

@interface MPCommonOpenGLView : NSOpenGLView
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
<NSWindowDelegate>
#endif
{
	//Cocoa
	NSWindow *window;

	BOOL mouseHide;

	//menu command id
	NSMenuItem *kHalfScreenCmd;
	NSMenuItem *kNormalScreenCmd;
	NSMenuItem *kDoubleScreenCmd;
	NSMenuItem *kFullScreenCmd;
	NSMenuItem *kKeepAspectCmd;
	NSMenuItem *kAspectOrgCmd;
	NSMenuItem *kAspectFullCmd;
	NSMenuItem *kAspectWideCmd;
	NSMenuItem *kPanScanCmd;

	//timestamps for disabling screensaver and mouse hiding
	int lastMouseHide;
	int lastScreensaverUpdate;

	int event_flags;
@public
	float winSizeMult;
	NSOpenGLContext *glContext;
}

- (BOOL) acceptsFirstResponder;
- (BOOL) becomeFirstResponder;
- (BOOL) resignFirstResponder;

//window & rendering
- (void) preinit;
- (void) configWidth: (uint32_t) width height: (uint32_t) height flags: (uint32_t)flags;
- (void) drawRect: (NSRect *) bounds;
- (void) reshape;

//vo control
- (void) fullscreen: (BOOL) animate;
- (void) ontop;
- (void) rootwin;

//menu
- (void) initMenu;
- (void) menuAction:(id)sender;

//event
- (void) keyDown: (NSEvent *) theEvent;
- (void) mouseMoved: (NSEvent *) theEvent;
- (void) mouseDown: (NSEvent *) theEvent;
- (void) mouseUp: (NSEvent *) theEvent;
- (void) rightMouseDown: (NSEvent *) theEvent;
- (void) rightMouseUp: (NSEvent *) theEvent;
- (void) otherMouseDown: (NSEvent *) theEvent;
- (void) otherMouseUp: (NSEvent *) theEvent;
- (void) scrollWheel: (NSEvent *) theEvent;
- (void) mouseEvent: (NSEvent *) theEvent;
- (int) check_events;

- (void) update_screen_info;
@end

#endif /* MPLAYER_OSX_OBJC_COMMON_H */
