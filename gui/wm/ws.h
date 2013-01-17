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
 *
 * based on: AutoSpace Window System for Linux/Win32,
 *           written by pontscho/fresh!mindworkz
 */

#ifndef MPLAYER_GUI_WS_H
#define MPLAYER_GUI_WS_H

#include "gui/dialog/dialog.h"
#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#ifdef HAVE_SHM
#include <X11/extensions/XShm.h>
#endif

#define  wsKeyReleased   0
#define  wsKeyPressed    1

#define  wsPLMouseButton 1
#define  wsPMMouseButton 2
#define  wsPRMouseButton 3
#define  wsP4MouseButton 4
#define  wsP5MouseButton 5
#define  wsRLMouseButton (1 + 128)
#define  wsRMMouseButton (2 + 128)
#define  wsRRMouseButton (3 + 128)
#define  wsR4MouseButton (4 + 128)
#define  wsR5MouseButton (5 + 128)
#define  wsEnterWindow   253
#define  wsLeaveWindow   254
#define  wsMoveMouse     255

#define  wsShowMouseCursor   1
#define  wsHideMouseCursor   0
#define  wsHandleMouseButton 2
#define  wsHandleMouseMove   4

#define  wsHideFrame    0
#define  wsShowFrame    1
#define  wsMaxSize      2
#define  wsMinSize      4
#define  wsShowWindow   8
#define  wsHideWindow   16
#define  wsOverredirect 32
#define  wsWaitMap      64
#define  wsAspect       128

#define wsRGB32 1
#define wsBGR32 2
#define wsRGB24 3
#define wsBGR24 4
#define wsRGB16 5
#define wsBGR16 6
#define wsRGB15 7
#define wsBGR15 8

#define wsNone                   0
#define wsWindowVisible          1
#define wsWindowPartialVisible   2
#define wsWindowNotVisible       4
#define wsWindowMapped           8
#define wsWindowUnmapped        16
#define wsWindowFocusIn         32
#define wsWindowFocusOut        64
#define wsWindowExpose         128
#define wsWindowRolled         256
#define wsWindowClosed         512

#define wsNo         0
#define wsMapped     1
#define wsFocused    2
#define wsVisible    3
#define wsNotVisible 4
#define wsPVisible   5
#define wsRolled     6

typedef   void (*wsTReDraw)(void);
typedef   void (*wsTKeyHandler)(int KeyCode, int Type, int Key);
typedef   void (*wsTMouseHandler)(int Button, int X, int Y, int RX, int RY);
typedef   void (*wsTDNDHandler)(int num, char **str);

typedef struct {
    Window WindowID;
    Window Parent;
    int X, Y, Width, Height;
    int OldX, OldY, OldWidth, OldHeight;
    int MaxX, MaxY;
    int isFullScreen;
    int BorderWidth;
    int Property;
    unsigned char *bImage;
    XImage *xImage;
    Pixmap Mask;
    int Decorations;

    int State;
    int Visible;
    int Mapped;
    int Focused;
    int Rolled;

    wsTReDraw ReDraw;
    wsTKeyHandler KeyHandler;
    wsTMouseHandler MouseHandler;
    wsTDNDHandler DandDHandler;

    int Alt;
    int Shift;
    int Control;
    int NumLock;
    int CapsLock;
/* Misc ------------------------------------------------------------------------------------- */

    Atom AtomDeleteWindow;
    Atom AtomTakeFocus;
    Atom AtomRolle;
    Atom AtomProtocols;
    Atom AtomsProtocols[3];
    Atom AtomLeaderClient;
    Atom AtomRemote;
    Atom AtomWMSizeHint;
    Atom AtomWMNormalHint;

#ifdef HAVE_SHM
    XShmSegmentInfo Shminfo;
#endif
    unsigned char *ImageData;
    unsigned short int *ImageDataw;
    unsigned int *ImageDatadw;
    GC wGC;
    XGCValues wGCV;
    unsigned long WindowMask;
    XVisualInfo VisualInfo;
    XSetWindowAttributes WindowAttrib;
    XWMHints WMHints;

    XFontStruct *Font;
    int FontHeight;

    Cursor wsCursor;
    char wsCursorData[1];
    Pixmap wsCursorPixmap;
    int wsMouseEventType;
    XColor wsColor;
} wsWindow;

extern int wsMaxX;
extern int wsMaxY;
extern int wsOrgX;
extern int wsOrgY;

extern Display *wsDisplay;

extern int wsDepthOnScreen;

// ----------------------------------------------------------------------------------------------
void wsXDone(void);
void wsXInit(Display *display);
void wsSetErrorHandler(void);

void wsAutohideCursor(void);
void wsEvents(XEvent *event);
void wsHandleEvents(void);

void wsCreateWindow(wsWindow *win, int x, int y, int w, int h, int b, int c, unsigned char p, char *label);
void wsDestroyWindow(wsWindow *win);
void wsMoveWindow(wsWindow *win, Bool abs, int x, int y);
void wsMoveWindowWithin(wsWindow *win, Bool abs, int x, int y);
void wsResizeWindow(wsWindow *win, int sx, int sy);
void wsIconify(wsWindow *win);
void wsRaiseWindowTop(Display *display, Window Win);
void wsSetBackgroundRGB(wsWindow *win, int r, int g, int b);
void wsClearWindow(wsWindow *win);
void wsVisibleWindow(wsWindow *win, int show);
void wsWindowDecoration(wsWindow *win, Bool decor);
void wsSetLayer(Display *display, Window Win, Bool fullscreen);
void wsFullScreen(wsWindow *win);
void wsPostRedisplay(wsWindow *win);
void wsSetShape(wsWindow *win, char *data);
void wsSetIcon(Display *display, Window Win, guiIcon_t *icon);

// ----------------------------------------------------------------------------------------------
//    Show / hide mouse cursor.
// ----------------------------------------------------------------------------------------------
void wsVisibleMouse(wsWindow *win, int m);

// ----------------------------------------------------------------------------------------------
// Image handling
// ----------------------------------------------------------------------------------------------
void wsCreateImage(wsWindow *win, int Width, int Height);
void wsConvert(wsWindow *win, unsigned char *Image);
void wsPutImage(wsWindow *win);
void wsResizeImage(wsWindow *win, int Width, int Height);
void wsDestroyImage(wsWindow *win);

#endif /* MPLAYER_GUI_WS_H */
