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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <X11/Xatom.h>

#include "ws.h"
#include "wsxdnd.h"
#include "gui/interface.h"

#include "config.h"
#include "help_mp.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libvo/video_out.h"
#include "libvo/x11_common.h"
#include "mpbswap.h"
#include "mp_core.h"
#include "mp_msg.h"
#include "osdep/timer.h"

#ifdef HAVE_SHM
#include <sys/shm.h>
#endif
#ifdef CONFIG_XSHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef CONFIG_XF86VM
#include <X11/extensions/xf86vmode.h>
#endif

#define MOUSEHIDE_DELAY 1000   // in milliseconds

static wsWindow *mouse_win;
static unsigned int mouse_time;

int wsMaxX;                          // Screen width.
int wsMaxY;                          // Screen height.
int wsOrgX;                          // Screen origin x.
int wsOrgY;                          // Screen origin y.

Display *wsDisplay;
static int wsScreen;
static Window wsRootWin;

static int wsScreenDepth;
static int wsRedMask;
static int wsGreenMask;
static int wsBlueMask;
static int wsNonNativeOrder;

#define wsWLCount 5
static wsWindow *wsWindowList[wsWLCount];

static int wsUseXShm   = True;
static int wsUseXShape = True;

static enum AVPixelFormat out_pix_fmt = PIX_FMT_NONE;

/* --- */

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)

#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

#define MWM_DECOR_ALL           (1L << 0)

/**
 * @brief Update screen width, screen height and screen origin x and y
 *        from xinerama information.
 *
 *        Set wsOrgX, wsOrgY, wsMaxX and wsMaxY as well as
 *        win->X, win->Y, win->Width and win->Height.
 *
 * @param win pointer to a ws window structure or NULL
 */
static void wsWindowUpdateXinerama(wsWindow *win)
{
    if (win) {
        vo_dx      = win->X;
        vo_dy      = win->Y;
        vo_dwidth  = win->Width;
        vo_dheight = win->Height;
    }

    vo_screenwidth  = wsMaxX;
    vo_screenheight = wsMaxY;

    update_xinerama_info();

    wsMaxX = vo_screenwidth;
    wsMaxY = vo_screenheight;
    wsOrgX = xinerama_x;
    wsOrgY = xinerama_y;

    if (win) {
        win->X      = wsOrgX;
        win->Y      = wsOrgY;
        win->Width  = wsMaxX;
        win->Height = wsMaxY;
    }
}

static int wsGetScreenDepth(void)
{
    int depth;
    XImage *mXImage;
    Visual *visual;

    if ((depth = vo_find_depth_from_visuals(wsDisplay, wsScreen, &visual)) > 0) {
        mXImage = XCreateImage(wsDisplay, visual, depth, ZPixmap, 0, NULL,
                               1, 1, 32, 0);
        wsScreenDepth = mXImage->bits_per_pixel;
        wsRedMask     = mXImage->red_mask;
        wsGreenMask   = mXImage->green_mask;
        wsBlueMask    = mXImage->blue_mask;
#if HAVE_BIGENDIAN
        wsNonNativeOrder = mXImage->byte_order == LSBFirst;
#else
        wsNonNativeOrder = mXImage->byte_order == MSBFirst;
#endif
        XDestroyImage(mXImage);
    } else {
        int bpp, ibpp;
        XWindowAttributes attribs;

        mXImage = XGetImage(wsDisplay, wsRootWin, 0, 0, 1, 1, AllPlanes, ZPixmap);
        bpp     = mXImage->bits_per_pixel;

        XGetWindowAttributes(wsDisplay, wsRootWin, &attribs);
        ibpp    = attribs.depth;
        mXImage = XGetImage(wsDisplay, wsRootWin, 0, 0, 1, 1, AllPlanes, ZPixmap);
        bpp     = mXImage->bits_per_pixel;

        if ((ibpp + 7) / 8 != (bpp + 7) / 8)
            ibpp = bpp;

        wsScreenDepth = ibpp;
        wsRedMask     = mXImage->red_mask;
        wsGreenMask   = mXImage->green_mask;
        wsBlueMask    = mXImage->blue_mask;
        XDestroyImage(mXImage);
    }

    return wsScreenDepth;
}

void wsInit(Display *display)
{
    int eventbase;
    int errorbase;

    mp_msg(MSGT_GPLAYER, MSGL_V, "X init.\n");

    wsDisplay = display;

    wsSetErrorHandler();

/* enable DND atoms */
    wsXDNDInitialize();

    { /* on remote display XShm will be disabled - LGB */
        char *dispname = DisplayString(wsDisplay);
        int localdisp  = 1;

        if (dispname && *dispname != ':') {
            localdisp = 0;
            wsUseXShm = False;
        }

        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws] display name: %s => %s display.\n", dispname, localdisp ? "local" : "REMOTE");

        if (!localdisp)
            mp_msg(MSGT_GPLAYER, MSGL_INFO, MSGTR_WS_RemoteDisplay);
    }

#ifdef HAVE_SHM
    if (!XShmQueryExtension(wsDisplay))
#endif
    wsUseXShm = False;

    if (!wsUseXShm)
        mp_msg(MSGT_GPLAYER, MSGL_INFO, MSGTR_WS_NoXshm);

#ifdef CONFIG_XSHAPE
    if (!XShapeQueryExtension(wsDisplay, &eventbase, &errorbase))
#endif
    wsUseXShape = False;

    if (!wsUseXShape)
        mp_msg(MSGT_GPLAYER, MSGL_WARN, MSGTR_WS_NoXshape);

    XSynchronize(wsDisplay, True);

    wsScreen  = DefaultScreen(wsDisplay);
    wsRootWin = RootWindow(wsDisplay, wsScreen);
#ifdef CONFIG_XF86VM
    {
        int clock;
        XF86VidModeModeLine modeline;

        XF86VidModeGetModeLine(wsDisplay, wsScreen, &clock, &modeline);
        wsMaxX = modeline.hdisplay;
        wsMaxY = modeline.vdisplay;
    }
#endif
    {
        wsOrgX = wsOrgY = 0;

        if (!wsMaxX)
            wsMaxX = DisplayWidth(wsDisplay, wsScreen);

        if (!wsMaxY)
            wsMaxY = DisplayHeight(wsDisplay, wsScreen);
    }

    wsWindowUpdateXinerama(NULL);

    wsGetScreenDepth();

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws] Screen depth: %d\n", wsScreenDepth);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws]  size: %dx%d\n", wsMaxX, wsMaxY);

#ifdef CONFIG_XINERAMA
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws]  origin: +%d+%d\n", wsOrgX, wsOrgY);
#endif

    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws]  red mask: 0x%x\n", wsRedMask);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws]  green mask: 0x%x\n", wsGreenMask);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws]  blue mask: 0x%x\n", wsBlueMask);

#ifdef HAVE_SHM
    if (wsUseXShm) {
        int minor, major, shp;

        XShmQueryVersion(wsDisplay, &major, &minor, &shp);
        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws] XShm version is %d.%d\n", major, minor);
    }
#endif

#ifdef CONFIG_XSHAPE
    if (wsUseXShape) {
        int minor, major;

        XShapeQueryVersion(wsDisplay, &major, &minor);
        mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws] XShape version is %d.%d\n", major, minor);
    }
#endif

    if (wsScreenDepth == 32 && wsRedMask == 0xff0000 && wsGreenMask == 0x00ff00 && wsBlueMask == 0x0000ff)
        out_pix_fmt = PIX_FMT_RGB32;
    else if (wsScreenDepth == 32 && wsRedMask == 0x0000ff && wsGreenMask == 0x00ff00 && wsBlueMask == 0xff0000)
        out_pix_fmt = PIX_FMT_BGR32;
    else if (wsScreenDepth == 24 && wsRedMask == 0xff0000 && wsGreenMask == 0x00ff00 && wsBlueMask == 0x0000ff)
        out_pix_fmt = PIX_FMT_RGB24;
    else if (wsScreenDepth == 24 && wsRedMask == 0x0000ff && wsGreenMask == 0x00ff00 && wsBlueMask == 0xff0000)
        out_pix_fmt = PIX_FMT_BGR24;
    else if (wsScreenDepth == 16 && wsRedMask == 0xf800 && wsGreenMask == 0x7e0 && wsBlueMask == 0x1f)
        out_pix_fmt = PIX_FMT_RGB565;
    else if (wsScreenDepth == 16 && wsRedMask == 0x1f && wsGreenMask == 0x7e0 && wsBlueMask == 0xf800)
        out_pix_fmt = PIX_FMT_BGR565;
    else if (wsScreenDepth == 15 && wsRedMask == 0x7c00 && wsGreenMask == 0x3e0 && wsBlueMask == 0x1f)
        out_pix_fmt = PIX_FMT_RGB555;
    else if (wsScreenDepth == 15 && wsRedMask == 0x1f && wsGreenMask == 0x3e0 && wsBlueMask == 0x7c00)
        out_pix_fmt = PIX_FMT_BGR555;
}

void wsDone(void)
{
    // NOTE TO MYSELF: uninit wsInit stuff?
    //XCloseDisplay(wsDisplay);
}

/**
 * @brief Inform about an X error that has occurred.
 *
 * @param display display
 * @param event pointer to an X error event structure
 *
 * @return 0
 */
static int wsErrorHandler(Display *display, XErrorEvent *event)
{
    char type[128];

    XGetErrorText(display, event->error_code, type, sizeof(type));

    mp_msg(MSGT_GPLAYER, MSGL_ERR, "[ws] " MSGTR_WS_XError);
    mp_msg(MSGT_GPLAYER, MSGL_ERR, "[ws]  Error code: %d - %s\n", event->error_code, type);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws]  Request code: %d (minor code: %d)\n", event->request_code, event->minor_code);
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws]  MPlayer module: %s\n", current_module ? current_module : "(none)");

    return 0;
}

/**
 * @brief Set the X error handler.
 */
void wsSetErrorHandler(void)
{
    XSetErrorHandler(wsErrorHandler);
}

// ----------------------------------------------------------------------------------------------
//   Handle events.
// ----------------------------------------------------------------------------------------------

static int wsWindowListSearch(Window win)
{
    int i;

    for (i = 0; i < wsWLCount; i++)
        if (wsWindowList[i] && wsWindowList[i]->WindowID == win)
            return i;

    return -1;
}

void wsEvent(XEvent *event)
{
    unsigned long i = 0;
    int l;
    int x, y;
    Window child_window = 0;

    l = wsWindowListSearch(event->xany.window);

    if (l == -1)
        return;

    wsWindowList[l]->State = wsNone;

    switch (event->type) {
    case ClientMessage:

        if (event->xclient.message_type == wsWindowList[l]->AtomProtocols) {
            if ((Atom)event->xclient.data.l[0] == wsWindowList[l]->AtomDeleteWindow) {
                i = wsWindowClosed;
                goto expose;
            }

            if ((Atom)event->xclient.data.l[0] == wsWindowList[l]->AtomTakeFocus) {
                i = wsWindowFocusIn;
                wsWindowList[l]->Focused = wsFocused;
                goto expose;
            }

            if ((Atom)event->xclient.data.l[0] == wsWindowList[l]->AtomRolle) {
                mp_msg(MSGT_GPLAYER, MSGL_V, "[ws] role set.\n");
            }
        } else {
            /* try to process DND events */
            wsXDNDProcessClientMessage(&event->xclient);
        }

        break;

    case MapNotify:

        i = wsWindowMapped;
        wsWindowList[l]->Mapped = wsMapped;
        goto expose;

    case UnmapNotify:

        i = wsWindowUnmapped;
        wsWindowList[l]->Mapped = wsNo;
        goto expose;

    case FocusIn:

        if (wsWindowList[l]->Focused == wsFocused)
            break;

        i = wsWindowFocusIn;
        wsWindowList[l]->Focused = wsFocused;
        goto expose;

    case FocusOut:

        if (wsWindowList[l]->Focused == wsNo)
            break;

        i = wsWindowFocusOut;
        wsWindowList[l]->Focused = wsNo;
        goto expose;

    case VisibilityNotify:

        switch (event->xvisibility.state) {
        case VisibilityUnobscured:
            i = wsWindowVisible;
            wsWindowList[l]->Visible = wsVisible;
            goto expose;

        case VisibilityFullyObscured:
            i = wsWindowNotVisible;
            wsWindowList[l]->Visible = wsNotVisible;
            goto expose;

        case VisibilityPartiallyObscured:
            i = wsWindowPartialVisible;
            wsWindowList[l]->Visible = wsPVisible;
            goto expose;
        }

expose:
        wsWindowList[l]->State = i;

        if (wsWindowList[l]->ReDraw)
            wsWindowList[l]->ReDraw();

        break;

    case Expose:

        wsWindowList[l]->State = wsWindowExpose;

        if ((wsWindowList[l]->ReDraw) && (!event->xexpose.count))
            wsWindowList[l]->ReDraw();

        break;

    case ConfigureNotify:

        XTranslateCoordinates(wsDisplay, wsWindowList[l]->WindowID, wsRootWin, 0, 0, &x, &y, &child_window);

        if ((wsWindowList[l]->X != x) || (wsWindowList[l]->Y != y) || (wsWindowList[l]->Width != event->xconfigure.width) || (wsWindowList[l]->Height != event->xconfigure.height)) {
            wsWindowList[l]->X      = x;
            wsWindowList[l]->Y      = y;
            wsWindowList[l]->Width  = event->xconfigure.width;
            wsWindowList[l]->Height = event->xconfigure.height;
        }

        wsWindowList[l]->Rolled = wsNo;

        if (event->xconfigure.y < 0) {
            i = wsWindowRolled;
            wsWindowList[l]->Rolled = wsRolled;
            goto expose;
        }

        break;

    case KeyPress:

        i = wsKeyPressed;
        goto keypressed;

    case KeyRelease:

        i = wsKeyReleased;
keypressed:
        wsWindowList[l]->Alt      = False;
        wsWindowList[l]->Shift    = False;
        wsWindowList[l]->NumLock  = False;
        wsWindowList[l]->Control  = False;
        wsWindowList[l]->CapsLock = False;

        if (event->xkey.state & Mod1Mask)
            wsWindowList[l]->Alt = True;

        if (event->xkey.state & Mod2Mask)
            wsWindowList[l]->NumLock = True;

        if (event->xkey.state & ControlMask)
            wsWindowList[l]->Control = True;

        if (event->xkey.state & ShiftMask)
            wsWindowList[l]->Shift = True;

        if (event->xkey.state & LockMask)
            wsWindowList[l]->CapsLock = True;

#if 0
        {
            KeySym keySym;
            keySym = XKeycodeToKeysym(wsDisplay, event->xkey.keycode, 0);

            if (keySym != NoSymbol) {
                keySym = ((keySym & 0xff00) != 0 ? ((keySym & 0x00ff) + 256) : (keySym));

                if (wsWindowList[l]->KeyHandler)
                    wsWindowList[l]->KeyHandler(event->xkey.state, i, keySym);
            }
        }
#else
        {
            int key;
            char buf[100];
            KeySym keySym;
            static XComposeStatus stat;

            XLookupString(&event->xkey, buf, sizeof(buf), &keySym, &stat);
            key = ((keySym & 0xff00) != 0 ? ((keySym & 0x00ff) + 256) : (keySym));

            if (wsWindowList[l]->KeyHandler)
                wsWindowList[l]->KeyHandler(event->xkey.keycode, i, key);
        }
#endif
        break;

    case MotionNotify:

        i = wsMoveMouse;
        {
            /* pump all motion events from the display queue:
             * this way it works faster when moving the window */
            static XEvent e;

            if (event->xmotion.state) {
                while (XCheckTypedWindowEvent(wsDisplay, event->xany.window, MotionNotify, &e)) {
                    /* FIXME: need to make sure we didn't release/press the button in between...*/
                    /* FIXME: do we need some timeout here to make sure we don't spend too much time
                     * removing events from the queue? */
                    event = &e;
                }
            }
        }

        if (wsWindowList[l]->wsCursor != None) {
            wsMouseVisibility(wsWindowList[l], wsShowMouseCursor);
            mouse_win  = wsWindowList[l];
            mouse_time = GetTimerMS();
        }

        goto buttonreleased;

    case ButtonRelease:

        i = event->xbutton.button + 128;

        if (wsWindowList[l]->wsCursor != None) {
            wsMouseVisibility(wsWindowList[l], wsShowMouseCursor);
            mouse_win  = wsWindowList[l];
            mouse_time = GetTimerMS();
        }

        goto buttonreleased;

    case ButtonPress:

        i = event->xbutton.button;

        if (wsWindowList[l]->wsCursor != None) {
            wsMouseVisibility(wsWindowList[l], wsShowMouseCursor);
            mouse_win  = wsWindowList[l];
            mouse_time = GetTimerMS();
        }

        goto buttonreleased;

    case EnterNotify:

        i = wsEnterWindow;
        goto buttonreleased;

    case LeaveNotify:

        i = wsLeaveWindow;
buttonreleased:

        if (wsWindowList[l]->MouseHandler)
            wsWindowList[l]->MouseHandler(i, event->xbutton.x, event->xbutton.y, event->xmotion.x_root, event->xmotion.y_root);

        break;

    case SelectionNotify:

        /* Handle DandD */
        wsXDNDProcessSelection(wsWindowList[l], event);
        break;
    }

    XFlush(wsDisplay);
    XSync(wsDisplay, False);
}

void wsEvents(void)
{
    XEvent xev;
    /* handle pending events */
    while (XPending(wsDisplay)) {
        XNextEvent(wsDisplay, &xev);
//   printf("### X event: %d  [%d]\n",xev.type,delay);
        wsEvent(&xev);
    }
}

/**
 * @brief Calculate and store the x and y position for a window.
 *
 * @param win pointer to a ws window structure
 * @param x x position of the window (real/absolute or mock)
 * @param y y position of the window (real/absolute or mock)
 * @param width width of the area to place the window in
 * @param height height of the area to place the window in
 */
static void wsWindowUpdatePosition(wsWindow *win, int x, int y, int width, int height)
{
    switch (x) {
    case -1:
        win->X = wsOrgX + (wsMaxX - width) / 2;
        break;

    case -2:
        win->X = wsOrgX + wsMaxX - width;
        break;

    default:
        win->X = x;
        break;
    }

    switch (y) {
    case -1:
        win->Y = wsOrgY + (wsMaxY - height) / 2;
        break;

    case -2:
        win->Y = wsOrgY + wsMaxY - height;
        break;

    default:
        win->Y = y;
        break;
    }
}

/**
 * @brief Replace the size hints for the WM_NORMAL_HINTS property of a window.
 *
 * @param win pointer to a ws window structure
 */
static void wsWindowSizeHint(wsWindow *win)
{
    XSizeHints size;

    size.flags = 0;

    /* obsolete, solely for compatibility reasons */
    size.flags |= PPosition;
    size.x      = win->X;
    size.y      = win->Y;

    /* obsolete, solely for compatibility reasons */
    size.flags |= PSize;
    size.width  = win->Width;
    size.height = win->Height;

    /* a minimum of 4 is said to avoid off-by-one errors and be required by mga_vid */
    size.flags     |= PMinSize;
    size.min_width  = 4;
    size.min_height = 4;

    if (win->Property & wsMinSize) {
        size.min_width  = win->Width;
        size.min_height = win->Height;
    }

    if (win->Property & wsMaxSize) {
        size.flags     |= PMaxSize;
        size.max_width  = win->Width;
        size.max_height = win->Height;
    }

    if (vo_keepaspect && (win->Property & wsAspect)) {
        size.flags |= PAspect;
        size.min_aspect.x = win->Width;
        size.min_aspect.y = win->Height;
        size.max_aspect.x = win->Width;
        size.max_aspect.y = win->Height;
    }

    size.flags      |= PBaseSize;
    size.base_width  = 0;
    size.base_height = 0;

    size.flags      |= PWinGravity;
    size.win_gravity = StaticGravity;

    XSetWMNormalHints(wsDisplay, win->WindowID, &size);
}

static void wsWindowDecoration(wsWindow *win)
{
    Atom wsMotifHints;
    struct {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long input_mode;
        unsigned long status;
    } wsMotifWmHints;

    wsMotifHints = XInternAtom(wsDisplay, "_MOTIF_WM_HINTS", 0);

    if (wsMotifHints == None)
        return;

    memset(&wsMotifWmHints, 0, sizeof(wsMotifWmHints));
    wsMotifWmHints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;

    if (win->Decoration) {
        wsMotifWmHints.functions = MWM_FUNC_MOVE | MWM_FUNC_CLOSE | MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE;

        if (!(win->Property & wsMinSize) || !(win->Property & wsMaxSize))
            wsMotifWmHints.functions |= MWM_FUNC_RESIZE;

        wsMotifWmHints.decorations = MWM_DECOR_ALL;
    }

    XChangeProperty(wsDisplay, win->WindowID, wsMotifHints, wsMotifHints, 32,
                    PropModeReplace, (unsigned char *)&wsMotifWmHints, 5);
}

/**
 * @brief Wait until a window is mapped if its property requires it.
 *
 * @param win pointer to a ws window structure
 */
static void wsWindowMapWait(wsWindow *win)
{
    XEvent xev;

    if (win->Property & wsWaitMap) {
        do
            XNextEvent(wsDisplay, &xev);
        while (xev.type != MapNotify || xev.xmap.event != win->WindowID);

        win->Mapped = wsMapped;
    }
}

// ----------------------------------------------------------------------------------------------
//   Create window.
//     X,Y   : window position
//     wX,wY : size of window
//     bW    : border width
//     cV    : visible mouse cursor on window
//     D     : visible frame, title, etc.
//     sR    : screen ratio
// ----------------------------------------------------------------------------------------------

XClassHint wsClassHint;
XTextProperty wsTextProperty;
Window LeaderWindow;

// ----------------------------------------------------------------------------------------------
//  wsWindowCreate: create a new window on the screen.
//   x,y   : window position
//   w,h   : window size
//   c     : mouse cursor visible
//   p     : properties - "decoration", visible titlebar, etc ...
// ----------------------------------------------------------------------------------------------
void wsWindowCreate(wsWindow *win, int x, int y, int w, int h, int p, int c, char *label)
{
    int depth;

    win->Property = p;

    win->Decoration = ((p & wsShowFrame) != 0);

    wsWindowUpdatePosition(win, x, y, w, h);

    win->Width     = w;
    win->Height    = h;
    win->OldX      = win->X;
    win->OldY      = win->Y;
    win->OldWidth  = win->Width;
    win->OldHeight = win->Height;

/* Hide Mouse Cursor */
    win->wsCursor = None;
    win->wsMouseEventType = c;
    win->wsCursorData[0]  = 0;
    win->wsCursorPixmap   = XCreateBitmapFromData(wsDisplay, wsRootWin, win->wsCursorData, 1, 1);

    if (!(c & wsShowMouseCursor))
        win->wsCursor = XCreatePixmapCursor(wsDisplay, win->wsCursorPixmap, win->wsCursorPixmap, &win->wsColor, &win->wsColor, 0, 0);

    depth = vo_find_depth_from_visuals(wsDisplay, wsScreen, NULL);

    if (depth < 15) {
        mp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_WS_ColorDepthTooLow);
        mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
    }

    XMatchVisualInfo(wsDisplay, wsScreen, depth, TrueColor, &win->VisualInfo);

/* --- */
    win->AtomLeaderClient = XInternAtom(wsDisplay, "WM_CLIENT_LEADER", False);
    win->AtomDeleteWindow = XInternAtom(wsDisplay, "WM_DELETE_WINDOW", False);
    win->AtomTakeFocus    = XInternAtom(wsDisplay, "WM_TAKE_FOCUS", False);
    win->AtomRolle         = XInternAtom(wsDisplay, "WM_WINDOW_ROLE", False);
    win->AtomWMSizeHint    = XInternAtom(wsDisplay, "WM_SIZE_HINT", False);
    win->AtomWMNormalHint  = XInternAtom(wsDisplay, "WM_NORMAL_HINT", False);
    win->AtomProtocols     = XInternAtom(wsDisplay, "WM_PROTOCOLS", False);
    win->AtomsProtocols[0] = win->AtomDeleteWindow;
    win->AtomsProtocols[1] = win->AtomTakeFocus;
    win->AtomsProtocols[2] = win->AtomRolle;
/* --- */

    win->WindowAttrib.border_pixel = WhitePixel(wsDisplay, wsScreen);
    win->WindowAttrib.colormap     = XCreateColormap(wsDisplay, wsRootWin, win->VisualInfo.visual, AllocNone);
    win->WindowAttrib.event_mask   = StructureNotifyMask | FocusChangeMask |
                                     ExposureMask | PropertyChangeMask |
                                     EnterWindowMask | LeaveWindowMask |
                                     VisibilityChangeMask |
                                     KeyPressMask | KeyReleaseMask;

    if ((c & wsHandleMouseButton))
        win->WindowAttrib.event_mask |= ButtonPressMask | ButtonReleaseMask;

    if ((c & wsHandleMouseMove))
        win->WindowAttrib.event_mask |= PointerMotionMask;

    win->WindowAttrib.cursor = win->wsCursor;
    win->WindowAttrib.override_redirect = False;

    if (p & wsOverredirect)
        win->WindowAttrib.override_redirect = True;

    win->WindowMask = CWBorderPixel |
                      CWColormap | CWEventMask | CWCursor |
                      CWOverrideRedirect;

    win->WindowID = XCreateWindow(wsDisplay,
                                  (win->Parent != 0 ? win->Parent : wsRootWin),
                                  win->X, win->Y, win->Width, win->Height, 0,
                                  win->VisualInfo.depth,
                                  InputOutput,
                                  win->VisualInfo.visual,
                                  win->WindowMask, &win->WindowAttrib);

    wsClassHint.res_name = "MPlayer";

    wsClassHint.res_class = "MPlayer";
    XSetClassHint(wsDisplay, win->WindowID, &wsClassHint);

    wsWindowSizeHint(win);

    win->WMHints.flags = InputHint | StateHint;
    win->WMHints.input = True;
    win->WMHints.initial_state = NormalState;
    XSetWMHints(wsDisplay, win->WindowID, &win->WMHints);

    wsWindowDecoration(win);
    XStoreName(wsDisplay, win->WindowID, label);
    XmbSetWMProperties(wsDisplay, win->WindowID, label, label, NULL, 0, NULL, NULL, NULL);

    XSetWMProtocols(wsDisplay, win->WindowID, win->AtomsProtocols, 3);
    XChangeProperty(wsDisplay, win->WindowID,
                    win->AtomLeaderClient,
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&LeaderWindow, 1);

    wsTextProperty.value    = label;
    wsTextProperty.encoding = XA_STRING;
    wsTextProperty.format   = 8;
    wsTextProperty.nitems   = strlen(label);
    XSetWMIconName(wsDisplay, win->WindowID, &wsTextProperty);

    win->wGC = XCreateGC(wsDisplay, win->WindowID,
                         0,
                         NULL);

    win->Visible = wsNo;
    win->Focused = wsNo;
    win->Mapped  = wsNo;
    win->Rolled  = wsNo;

    if (p & wsShowWindow) {
        XMapWindow(wsDisplay, win->WindowID);
        wsWindowMapWait(win);
    }

    wsImageCreate(win, win->Width, win->Height);
/* End of creating -------------------------------------------------------------------------- */

    {
        int i;

        for (i = 0; i < wsWLCount; i++)
            if (wsWindowList[i] == NULL)
                break;

        if (i == wsWLCount) {
            mp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_WS_TooManyOpenWindows);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }

        wsWindowList[i] = win;
    }

    XFlush(wsDisplay);
    XSync(wsDisplay, False);

    win->ReDraw       = NULL;
    win->MouseHandler = NULL;
    win->KeyHandler   = NULL;
    mp_msg(MSGT_GPLAYER, MSGL_DBG2, "[ws] window is created. ( %s ).\n", label);
}

void wsWindowDestroy(wsWindow *win)
{
    int l;

    l = wsWindowListSearch(win->WindowID);

    if (l != -1)
        wsWindowList[l] = NULL;

    win->ReDraw       = NULL;
    win->MouseHandler = NULL;
    win->KeyHandler   = NULL;
    win->DandDHandler = NULL;

    if (win->wsCursor != None) {
        XFreeCursor(wsDisplay, win->wsCursor);
        win->wsCursor = None;
    }

    XFreeGC(wsDisplay, win->wGC);
    XUnmapWindow(wsDisplay, win->WindowID);
    wsImageDestroy(win);
    XDestroyWindow(wsDisplay, win->WindowID);
#if 0
    win->Visible = wsNo;
    win->Focused = wsNo;
    win->Mapped  = wsNo;
    win->Rolled  = wsNo;
#endif
}

void wsWindowShape(wsWindow *win, char *data)
{
#ifdef CONFIG_XSHAPE
    if (!wsUseXShape)
        return;

    if (data) {
        win->Mask = XCreateBitmapFromData(wsDisplay, win->WindowID, data, win->Width, win->Height);
        XShapeCombineMask(wsDisplay, win->WindowID, ShapeBounding, 0, 0, win->Mask, ShapeSet);
        XFreePixmap(wsDisplay, win->Mask);
    } else
        XShapeCombineMask(wsDisplay, win->WindowID, ShapeBounding, 0, 0, None, ShapeSet);
#endif
}

/**
 * @brief Set differently sized icons to a window.
 *
 *        This function sets the X icon hint as well as
 *        the properties KWM_WIN_ICON and _NET_WM_ICON.
 *
 * @param display display
 * @param Win window
 * @param icon pointer to the icons
 */
void wsWindowIcon(Display *display, Window Win, guiIcon_t *icon)
{
    XWMHints *wm;
    Atom iconatom;
    long data[2];

    if (icon->normal) {
        wm = XGetWMHints(display, Win);

        if (!wm)
            wm = XAllocWMHints();

        wm->icon_pixmap = icon->normal;
        wm->icon_mask   = icon->normal_mask;
        wm->flags      |= IconPixmapHint | IconMaskHint;

        XSetWMHints(display, Win, wm);
        XFree(wm);
    }

    if (icon->small || icon->normal) {
        iconatom = XInternAtom(display, "KWM_WIN_ICON", False);
        data[0]  = (icon->small ? icon->small : icon->normal);
        data[1]  = (icon->small ? icon->small_mask : icon->normal_mask);

        XChangeProperty(display, Win, iconatom, iconatom, 32, PropModeReplace, (unsigned char *)data, 2);
    }

    if (icon->collection) {
        iconatom = XInternAtom(display, "_NET_WM_ICON", False);
        XChangeProperty(display, Win, iconatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)icon->collection, icon->collection_size);
    }
}

// ----------------------------------------------------------------------------------------------
//    Set window background to 'color'.
// ----------------------------------------------------------------------------------------------
/**
 * @brief Pack color components @a r, @a g and @a b into 15 bits.
 *
 * @param r red (0 - 255)
 * @param g green (0 - 255)
 * @param b blue (0 - 255)
 *
 * @return pixel suitable for a RGB 15 bits color format
 */
static int pack_rgb15(int r, int g, int b)
{
    int pixel;

    pixel   = r >> 3;
    pixel <<= 5;
    pixel  |= g >> 3;
    pixel <<= 5;
    pixel  |= b >> 3;

    return pixel;
}

/**
 * @brief Pack color components @a r, @a g and @a b into 16 bits.
 *
 * @param r red (0 - 255)
 * @param g green (0 - 255)
 * @param b blue (0 - 255)
 *
 * @return pixel suitable for a RGB 16 bits color format
 */
static int pack_rgb16(int r, int g, int b)
{
    int pixel;

    pixel   = r >> 3;
    pixel <<= 6;
    pixel  |= g >> 2;
    pixel <<= 5;
    pixel  |= b >> 3;

    return pixel;
}

void wsWindowBackground(wsWindow *win, int r, int g, int b)
{
    int color = 0;

    switch (out_pix_fmt) {
    case PIX_FMT_RGB32:
    case PIX_FMT_RGB24:
        color = (r << 16) + (g << 8) + b;
        break;

    case PIX_FMT_BGR32:
    case PIX_FMT_BGR24:
        color = (b << 16) + (g << 8) + r;
        break;

    case PIX_FMT_RGB565:
        color = pack_rgb16(r, g, b);
        break;

    case PIX_FMT_BGR565:
        color = pack_rgb16(b, g, r);
        break;

    case PIX_FMT_RGB555:
        color = pack_rgb15(r, g, b);
        break;

    case PIX_FMT_BGR555:
        color = pack_rgb15(b, g, r);
        break;

    default:
        ;
    }

    XSetWindowBackground(wsDisplay, win->WindowID, color);
    XClearWindow(wsDisplay, win->WindowID);
}

// ----------------------------------------------------------------------------------------------
//    Move window to x, y.
// ----------------------------------------------------------------------------------------------
void wsWindowMove(wsWindow *win, Bool abs, int x, int y)
{
    if (abs) {
        win->X = x;
        win->Y = y;
    } else
        wsWindowUpdatePosition(win, x, y, win->Width, win->Height);

    wsWindowSizeHint(win);
    XMoveWindow(wsDisplay, win->WindowID, win->X, win->Y);
}

/**
 * @brief Move the window to the x and y position, but if it no longer fits
 *        into the screen, reposition it towards the upper left.
 *
 * @param win pointer to a ws window structure
 * @param abs flag whether the position is real/absolute (True) or mock (False)
 * @param x x position of the window (real/absolute or mock)
 * @param y y position of the window (real/absolute or mock)
 */
void wsWindowMoveWithin(wsWindow *win, Bool abs, int x, int y)
{
    Bool fitting = True;

    wsWindowMove(win, abs, x, y);

    if (win->X + win->Width + 1 > wsMaxX) {
        fitting = False;
        win->X  = wsMaxX - win->Width;

        if (win->X < 0)
            win->X = 0;
    }

    if (win->Y + win->Height + 1 > wsMaxY) {
        fitting = False;
        win->Y  = wsMaxY - win->Height;

        if (win->Y < 0)
            win->Y = 0;
    }

    if (!fitting)
        wsWindowMove(win, True, win->X, win->Y);
}

// ----------------------------------------------------------------------------------------------
//    Resize window to sx, sy.
// ----------------------------------------------------------------------------------------------
void wsWindowResize(wsWindow *win, int w, int h)
{
    win->Width  = w;
    win->Height = h;

    if (vo_wm_type == 0)
        XUnmapWindow(wsDisplay, win->WindowID);

    wsWindowSizeHint(win);
    XResizeWindow(wsDisplay, win->WindowID, w, h);

    if (vo_wm_type == 0)
        XMapWindow(wsDisplay, win->WindowID);
}

/**
 * @brief Switch window fullscreen state.
 *
 *        Switch normal window to fullscreen and fullscreen window to normal.
 *
 * @param win pointer to a ws window structure
 */
void wsWindowFullscreen(wsWindow *win)
{
    if (win->isFullScreen) {
        if (vo_fs_type & vo_wm_FULLSCREEN)
            /* window manager supports EWMH */
            vo_x11_ewmh_fullscreen(win->WindowID, _NET_WM_STATE_REMOVE);
        else {
            win->X      = win->OldX;
            win->Y      = win->OldY;
            win->Width  = win->OldWidth;
            win->Height = win->OldHeight;
        }

        win->isFullScreen = False;
    } else {
        if (vo_fs_type & vo_wm_FULLSCREEN)
            /* window manager supports EWMH */
            vo_x11_ewmh_fullscreen(win->WindowID, _NET_WM_STATE_ADD);
        else {
            win->OldX      = win->X;
            win->OldY      = win->Y;
            win->OldWidth  = win->Width;
            win->OldHeight = win->Height;
        }

        win->isFullScreen = True;

        wsWindowUpdateXinerama(win);
    }

    /* unknown window manager and obsolete option -fsmode used */
    if (vo_wm_type == 0 && !(vo_fsmode & 16)) {
        XUnmapWindow(wsDisplay, win->WindowID); // required for MWM
        XWithdrawWindow(wsDisplay, win->WindowID, wsScreen);
    }

    /* restore window if window manager doesn't support EWMH */
    if (!(vo_fs_type & vo_wm_FULLSCREEN)) {
        if (!win->isFullScreen)
            wsWindowDecoration(win);

        wsWindowSizeHint(win);
        wsWindowLayer(wsDisplay, win->WindowID, win->isFullScreen);
        XMoveResizeWindow(wsDisplay, win->WindowID, win->X, win->Y, win->Width, win->Height);
    }

    /* some window managers lose ontop after fullscreen */
    if (!win->isFullScreen & vo_ontop)
        wsWindowLayer(wsDisplay, win->WindowID, vo_ontop);

    wsWindowRaiseTop(wsDisplay, win->WindowID);
    XFlush(wsDisplay);
}

/**
 * @brief Iconify a window.
 *
 * @param win pointer to a ws window structure
 */
void wsWindowIconify(wsWindow *win)
{
    XIconifyWindow(wsDisplay, win->WindowID, 0);
}

void wsWindowVisibility(wsWindow *win, int vis)
{
    switch (vis) {
    case wsShowWindow:

        XMapRaised(wsDisplay, win->WindowID);
        wsWindowMapWait(win);

        if (vo_fs_type & vo_wm_FULLSCREEN)
            win->isFullScreen = False;

        break;

    case wsHideWindow:

        XUnmapWindow(wsDisplay, win->WindowID);
        break;
    }

    XFlush(wsDisplay);
}

/**
 * @brief Map a window and raise it to the top.
 *
 * @param display display
 * @param Win window
 */
void wsWindowRaiseTop(Display *display, Window Win)
{
    XMapRaised(display, Win);     // NOTE TO MYSELF: is that really enough?
    XRaiseWindow(display, Win);   // NOTE TO MYSELF: is that really enough?
}

// ----------------------------------------------------------------------------------------------
//    Move window to selected layer
// ----------------------------------------------------------------------------------------------

/**
 * @brief Set the layer for a window.
 *
 * @param display display
 * @param Win window
 * @param fullscreen whether to set fullscreen or normal layer
 */
void wsWindowLayer(Display *display, Window Win, Bool fullscreen)
{
    vo_x11_setlayer(display, Win, fullscreen);
}

// ----------------------------------------------------------------------------------------------
//    Redraw screen.
// ----------------------------------------------------------------------------------------------
void wsWindowRedraw(wsWindow *win)
{
    if (win->ReDraw) {
        win->State = wsWindowExpose;
        win->ReDraw();
        XFlush(wsDisplay);
    }
}

// ----------------------------------------------------------------------------------------------
//    Put 'Image' to window.
// ----------------------------------------------------------------------------------------------
void wsImageCreate(wsWindow *win, int w, int h)
{
#ifdef HAVE_SHM
    if (wsUseXShm) {
        win->xImage = XShmCreateImage(wsDisplay, win->VisualInfo.visual,
                                      win->VisualInfo.depth, ZPixmap, NULL, &win->Shminfo, w, h);

        if (win->xImage == NULL) {
            mp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_WS_ShmError);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }

        win->Shminfo.shmid = shmget(IPC_PRIVATE, win->xImage->bytes_per_line * win->xImage->height, IPC_CREAT | 0777);

        if (win->Shminfo.shmid < 0) {
            XDestroyImage(win->xImage);
            mp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_WS_ShmError);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }

        win->Shminfo.shmaddr = (char *)shmat(win->Shminfo.shmid, 0, 0);

        if (win->Shminfo.shmaddr == ((char *)-1)) {
            XDestroyImage(win->xImage);

            if (win->Shminfo.shmaddr != ((char *)-1))
                shmdt(win->Shminfo.shmaddr);

            mp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_WS_ShmError);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }

        win->xImage->data     = win->Shminfo.shmaddr;
        win->Shminfo.readOnly = False;
        XShmAttach(wsDisplay, &win->Shminfo);
        XSync(wsDisplay, False);
        shmctl(win->Shminfo.shmid, IPC_RMID, 0);
    } else
#endif
    {
        win->xImage = XCreateImage(wsDisplay, win->VisualInfo.visual, win->VisualInfo.depth,
                                   ZPixmap, 0, 0, w, h,
                                   (wsScreenDepth == 3) ? 32 : wsScreenDepth,
                                   0);

        if ((win->xImage->data = malloc(win->xImage->bytes_per_line * win->xImage->height)) == NULL) {
            mp_msg(MSGT_GPLAYER, MSGL_FATAL, MSGTR_WS_NotEnoughMemoryDrawBuffer);
            mplayer(MPLAYER_EXIT_GUI, EXIT_ERROR, 0);
        }
    }

    win->ImageData   = (unsigned char *)win->xImage->data;
    win->ImageDataw  = (unsigned short int *)win->xImage->data;
    win->ImageDatadw = (unsigned int *)win->xImage->data;
}

void wsImageDestroy(wsWindow *win)
{
    if (win->xImage) {
        XDestroyImage(win->xImage);

#ifdef HAVE_SHM
        if (wsUseXShm) {
            XShmDetach(wsDisplay, &win->Shminfo);
            shmdt(win->Shminfo.shmaddr);
        }
#endif
    }

    win->xImage = NULL;
}

void wsImageRender(wsWindow *win, unsigned char *img)
{
    static struct SwsContext *sws_ctx;
    const uint8_t *src[4] = { img, NULL, NULL, NULL };
    int src_stride[4]     = { 4 * win->xImage->width, 0, 0, 0 };
    uint8_t *dst[4]       = { win->ImageData, NULL, NULL, NULL };
    int dst_stride[4];
    int i;

    sws_ctx = sws_getCachedContext(sws_ctx, win->xImage->width, win->xImage->height, PIX_FMT_RGB32,
                                   win->xImage->width, win->xImage->height, out_pix_fmt,
                                   SWS_POINT, NULL, NULL, NULL);
    av_image_fill_linesizes(dst_stride, out_pix_fmt, win->xImage->width);
    sws_scale(sws_ctx, src, src_stride, 0, win->xImage->height, dst, dst_stride);

    if (!wsNonNativeOrder)
        return;

    switch (win->xImage->bits_per_pixel) {
    case 32:
    {
        uint32_t *d = (uint32_t *)win->ImageData;

        for (i = 0; i < win->xImage->width * win->xImage->height; i++)
            d[i] = bswap_32(d[i]);

        break;
    }

    case 16:
    case 15:
    {
        uint16_t *d = (uint16_t *)win->ImageData;

        for (i = 0; i < win->xImage->width * win->xImage->height; i++)
            d[i] = bswap_16(d[i]);

        break;
    }
    }
}

void wsImageDraw(wsWindow *win)
{
#ifdef HAVE_SHM
    if (wsUseXShm) {
        XShmPutImage(wsDisplay, win->WindowID, win->wGC, win->xImage,
                     0, 0,
                     (win->Width - win->xImage->width) / 2, (win->Height - win->xImage->height) / 2,
                     win->xImage->width, win->xImage->height, 0);
    } else
#endif
    {
        XPutImage(wsDisplay, win->WindowID, win->wGC, win->xImage,
                  0, 0,
                  (win->Width - win->xImage->width) / 2, (win->Height - win->xImage->height) / 2,
                  win->xImage->width, win->xImage->height);
    }
}

void wsImageResize(wsWindow *win, int w, int h)
{
    wsImageDestroy(win);
    wsImageCreate(win, w, h);
}

// ----------------------------------------------------------------------------------------------
//    Show / hide mouse cursor.
// ----------------------------------------------------------------------------------------------
void wsMouseVisibility(wsWindow *win, int vis)
{
    switch (vis) {
    case wsShowMouseCursor:

        if (win->wsCursor != None) {
            XFreeCursor(wsDisplay, win->wsCursor);
            win->wsCursor = None;
        }

        XDefineCursor(wsDisplay, win->WindowID, 0);
        break;

    case wsHideMouseCursor:

        win->wsCursor = XCreatePixmapCursor(wsDisplay, win->wsCursorPixmap, win->wsCursorPixmap, &win->wsColor, &win->wsColor, 0, 0);
        XDefineCursor(wsDisplay, win->WindowID, win->wsCursor);
        break;
    }

    XFlush(wsDisplay);
}

/**
 * @brief Handle automatic hiding of the cursor.
 */
void wsMouseAutohide(void)
{
    if (mouse_win && (GetTimerMS() - mouse_time >= MOUSEHIDE_DELAY)) {
        wsMouseVisibility(mouse_win, wsHideMouseCursor);
        mouse_win = NULL;
    }
}
