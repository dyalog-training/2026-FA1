/* MIT License
 * 
 * Copyright (c) 2022 Serge Zaitsev
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FENSTER_H
#define FENSTER_H

#if defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc-runtime.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#define _DEFAULT_SOURCE 1
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <time.h>
#endif

#include <stdint.h>
#include <stdlib.h>

struct fenster {
  const char *title;
  const int width;
  const int height;
  uint32_t *buf;
  int keys[256]; /* keys are mostly ASCII, but arrows are 17..20 */
  int mod;       /* mod is 4 bits mask, ctrl=1, shift=2, alt=4, meta=8 */
  int x;
  int y;
  int mouse;
#if defined(__APPLE__)
  id wnd;
  int closed; /* set when the user clicks the window's close button */
#elif defined(_WIN32)
  HWND hwnd;
  int display_width;
  int display_height;
  int xorigin;
  int yorigin;
  float scale;
#else
  Display *dpy;
  Window w;
  GC gc;
  XImage *img;
#endif
};

#ifndef FENSTER_API
#define FENSTER_API extern
#endif
FENSTER_API int fenster_open(struct fenster *f);
FENSTER_API int fenster_loop(struct fenster *f);
FENSTER_API void fenster_close(struct fenster *f);
FENSTER_API void fenster_sleep(int64_t ms);
FENSTER_API int64_t fenster_time(void);
#define fenster_pixel(f, x, y) ((f)->buf[((y) * (f)->width) + (x)])

#ifndef FENSTER_HEADER
#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <pthread.h>

#define msg(r, o, s) ((r(*)(id, SEL))objc_msgSend)(o, sel_getUid(s))
#define msg1(r, o, s, A, a)                                                    \
  ((r(*)(id, SEL, A))objc_msgSend)(o, sel_getUid(s), a)
#define msg2(r, o, s, A, a, B, b)                                              \
  ((r(*)(id, SEL, A, B))objc_msgSend)(o, sel_getUid(s), a, b)
#define msg3(r, o, s, A, a, B, b, C, c)                                        \
  ((r(*)(id, SEL, A, B, C))objc_msgSend)(o, sel_getUid(s), a, b, c)
#define msg4(r, o, s, A, a, B, b, C, c, D, d)                                  \
  ((r(*)(id, SEL, A, B, C, D))objc_msgSend)(o, sel_getUid(s), a, b, c, d)

#define cls(x) ((id)objc_getClass(x))

extern id const NSDefaultRunLoopMode;
extern id const NSApp;

/* AppKit may only be used from the main thread. A host may call us from
 * another thread while its main thread runs [NSApp run] -- the Dyalog APL
 * interpreter does this. Then all Cocoa work is handed to the main thread,
 * and the host's run loop delivers events to the view methods below.
 * Otherwise (we are on the main thread) fenster_loop pumps events itself. */
struct fenster_call {
  void *ctx;
  void (*fn)(void *);
};

extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *);

static void fenster_call_main(void *p) {
  struct fenster_call *c = (struct fenster_call *)p;
  void *pool = objc_autoreleasePoolPush();
  c->fn(c->ctx);
  objc_autoreleasePoolPop(pool);
}

static void fenster_on_main(void *ctx, void (*fn)(void *)) {
  struct fenster_call c = {ctx, fn};
  if (pthread_main_np())
    fenster_call_main(&c);
  else
    dispatch_sync_f(dispatch_get_main_queue(), &c, fenster_call_main);
}

/* Off the main thread we depend on the host servicing the main queue; check
 * that it does rather than risk blocking forever in dispatch_sync. */
static dispatch_semaphore_t fenster_ping_sem;
static void fenster_ping_init(void *p) {
  (void)p;
  fenster_ping_sem = dispatch_semaphore_create(0);
}
static void fenster_ping(void *p) {
  (void)p;
  dispatch_semaphore_signal(fenster_ping_sem);
}
static int fenster_main_alive(void) {
  static dispatch_once_t once;
  dispatch_once_f(&once, NULL, fenster_ping_init);
  dispatch_async_f(dispatch_get_main_queue(), NULL, fenster_ping);
  return !dispatch_semaphore_wait(
      fenster_ping_sem, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC));
}

static char fenster_key; /* associated-object key: view -> struct fenster */

static struct fenster *fenster_of(id v) {
  return (struct fenster *)objc_getAssociatedObject(v, &fenster_key);
}

// clang-format off
static const uint8_t FENSTER_KEYCODES[128] = {65,83,68,70,72,71,90,88,67,86,0,66,81,87,69,82,89,84,49,50,51,52,54,53,61,57,55,45,56,48,93,79,85,91,73,80,10,76,74,39,75,59,92,44,47,78,77,46,9,32,96,8,0,27,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,2,3,127,0,5,0,4,0,20,19,18,17,0};
// clang-format on

static void fenster_draw_rect(id v, SEL s, CGRect r) {
  (void)r, (void)s;
  struct fenster *f = fenster_of(v);
  if (!f)
    return;
  CGContextRef context =
      msg(CGContextRef, msg(id, cls("NSGraphicsContext"), "currentContext"),
          "CGContext");
  CGContextSetInterpolationQuality(context, kCGInterpolationNone);
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef provider = CGDataProviderCreateWithData(
      NULL, f->buf, f->width * f->height * 4, NULL);
  CGImageRef img =
      CGImageCreate(f->width, f->height, 8, 32, f->width * 4, space,
                    kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little,
                    provider, NULL, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(space);
  CGDataProviderRelease(provider);
  CGContextDrawImage(context, CGRectMake(0, 0, f->width, f->height), img);
  CGImageRelease(img);
}

static void fenster_mouse_event(id v, SEL s, id ev) {
  (void)s;
  struct fenster *f = fenster_of(v);
  if (!f)
    return;
  CGPoint xy = msg(CGPoint, ev, "locationInWindow");
  f->x = (int)xy.x;
  f->y = (int)(f->height - xy.y);
  switch (msg(NSUInteger, ev, "type")) {
  case 1: /* NSEventTypeLeftMouseDown */
    f->mouse |= 1;
    break;
  case 2: /* NSEventTypeLeftMouseUp */
    f->mouse &= ~1;
    break;
  }
}

static void fenster_key_event(id v, SEL s, id ev) {
  (void)s;
  struct fenster *f = fenster_of(v);
  if (!f)
    return;
  NSUInteger evtype = msg(NSUInteger, ev, "type");
  if (evtype == 10 || evtype == 11) { /* NSEventTypeKeyDown, KeyUp */
    NSUInteger k = msg(NSUInteger, ev, "keyCode");
    f->keys[k < 127 ? FENSTER_KEYCODES[k] : 0] = evtype == 10;
  }
  NSUInteger mod = msg(NSUInteger, ev, "modifierFlags") >> 17;
  f->mod = (mod & 0xc) | ((mod & 1) << 1) | ((mod >> 1) & 1);
}

static BOOL fenster_yes(id v, SEL s) { return (void)v, (void)s, YES; }
static BOOL fenster_yes1(id v, SEL s, id a) {
  return (void)v, (void)s, (void)a, YES;
}

/* Closing the window must not terminate the host process (e.g. an APL
 * interpreter), so just flag it and let fenster_loop report it. */
static BOOL fenster_should_close(id v, SEL s, id w) {
  (void)v, (void)s;
  struct fenster *f = fenster_of(msg(id, w, "contentView"));
  if (f)
    f->closed = 1;
  return NO;
}

/* The classes below are registered with the Objective-C runtime and outlive
 * this image, so if we are a shared library a host must never unload us (as
 * Dyalog APL does once no external function refers to us). Pin the image. */
__attribute__((constructor)) static void fenster_pin(void) {
  Dl_info info;
  if (dladdr((void *)fenster_pin, &info) && info.dli_fname)
    dlopen(info.dli_fname, RTLD_NOLOAD | RTLD_NODELETE);
}

/* Classes can only be registered once per process, but a host may open
 * several windows over its lifetime. */
static Class fenster_classes(Class *view) {
  Class d = (Class)cls("FensterDelegate"), c = (Class)cls("FensterView");
  if (!d) {
    d = objc_allocateClassPair((Class)cls("NSObject"), "FensterDelegate", 0);
    class_addMethod(d, sel_getUid("windowShouldClose:"),
                    (IMP)fenster_should_close, "c@:@");
    objc_registerClassPair(d);
  }
  if (!c) {
    static const char *mouse[] = {"mouseDown:", "mouseUp:", "mouseMoved:",
                                  "mouseDragged:"};
    static const char *keys[] = {"keyDown:", "keyUp:", "flagsChanged:"};
    c = objc_allocateClassPair((Class)cls("NSView"), "FensterView", 0);
    class_addMethod(c, sel_getUid("drawRect:"), (IMP)fenster_draw_rect,
                    "v@:{CGRect={CGPoint=dd}{CGSize=dd}}");
    class_addMethod(c, sel_getUid("acceptsFirstResponder"), (IMP)fenster_yes,
                    "c@:");
    class_addMethod(c, sel_getUid("acceptsFirstMouse:"), (IMP)fenster_yes1,
                    "c@:@");
    for (int i = 0; i < 4; i++)
      class_addMethod(c, sel_getUid(mouse[i]), (IMP)fenster_mouse_event,
                      "v@:@");
    for (int i = 0; i < 3; i++)
      class_addMethod(c, sel_getUid(keys[i]), (IMP)fenster_key_event, "v@:@");
    objc_registerClassPair(c);
  }
  *view = c;
  return d;
}

/* Dispatch pending events without blocking. Only for when the caller owns
 * the main thread: otherwise the host's run loop does this. */
static void fenster_pump(void) {
  id ev;
  while ((ev = msg4(id, NSApp,
                    "nextEventMatchingMask:untilDate:inMode:dequeue:",
                    NSUInteger, NSUIntegerMax, id, NULL, id,
                    NSDefaultRunLoopMode, BOOL, YES)))
    msg1(void, NSApp, "sendEvent:", id, ev);
}

static void fenster_open_main(void *p) {
  struct fenster *f = (struct fenster *)p;
  Class c;
  msg(id, cls("NSApplication"), "sharedApplication");
  msg1(void, NSApp, "setActivationPolicy:", NSInteger, 0);
  f->closed = 0;
  f->wnd = msg4(id, msg(id, cls("NSWindow"), "alloc"),
                "initWithContentRect:styleMask:backing:defer:", CGRect,
                CGRectMake(0, 0, f->width, f->height), NSUInteger, 3,
                NSUInteger, 2, BOOL, NO);
  if (!f->wnd)
    return;
  msg1(void, f->wnd, "setReleasedWhenClosed:", BOOL, NO);
  Class d = fenster_classes(&c);
  msg1(void, f->wnd, "setDelegate:", id,
       msg(id, msg(id, (id)d, "alloc"), "init"));

  id v = msg(id, msg(id, (id)c, "alloc"), "init");
  objc_setAssociatedObject(v, &fenster_key, (id)f, OBJC_ASSOCIATION_ASSIGN);
  msg1(void, f->wnd, "setContentView:", id, v);
  msg1(BOOL, f->wnd, "makeFirstResponder:", id, v);
  /* Report mouse movement even before the window is focused. Options are
   * NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect */
  id ta = msg(id, cls("NSTrackingArea"), "alloc");
  ta = ((id(*)(id, SEL, CGRect, NSUInteger, id, id))objc_msgSend)(
      ta, sel_getUid("initWithRect:options:owner:userInfo:"), CGRectZero,
      0x02 | 0x80 | 0x200, v, nil);
  msg1(void, v, "addTrackingArea:", id, ta);
  msg(void, ta, "release");
  msg(void, v, "release");

  id title = msg1(id, cls("NSString"), "stringWithUTF8String:", const char *,
                  f->title);
  msg1(void, f->wnd, "setTitle:", id, title);
  msg1(void, f->wnd, "makeKeyAndOrderFront:", id, nil);
  msg(void, f->wnd, "center");
  /* macOS 14+ ignores activateIgnoringOtherApps:, so at least make sure the
   * window is visible in front even if our process is not activated. */
  msg(void, f->wnd, "orderFrontRegardless");
  if (msg1(BOOL, NSApp, "respondsToSelector:", SEL, sel_getUid("activate")))
    msg(void, NSApp, "activate");
  else
    msg1(void, NSApp, "activateIgnoringOtherApps:", BOOL, YES);
}

FENSTER_API int fenster_open(struct fenster *f) {
  f->wnd = nil;
  if (!pthread_main_np() && !fenster_main_alive())
    return -1;
  fenster_on_main(f, fenster_open_main);
  return f->wnd ? 0 : -1;
}

static void fenster_close_main(void *p) {
  struct fenster *f = (struct fenster *)p;
  id d = msg(id, f->wnd, "delegate");
  objc_setAssociatedObject(msg(id, f->wnd, "contentView"), &fenster_key, nil,
                           OBJC_ASSOCIATION_ASSIGN);
  msg1(void, f->wnd, "setDelegate:", id, nil);
  msg(void, f->wnd, "close");
  msg(void, f->wnd, "release");
  msg(void, d, "release");
  f->wnd = nil;
}

FENSTER_API void fenster_close(struct fenster *f) {
  if (!f->wnd)
    return;
  fenster_on_main(f, fenster_close_main);
  if (pthread_main_np())
    fenster_pump(); /* so the window actually disappears */
}

static void fenster_display_main(void *p) {
  struct fenster *f = (struct fenster *)p;
  msg(void, msg(id, f->wnd, "contentView"), "display");
}

FENSTER_API int fenster_loop(struct fenster *f) {
  if (f->closed || !f->wnd)
    return -1;
  fenster_on_main(f, fenster_display_main);
  if (pthread_main_np())
    fenster_pump();
  return f->closed ? -1 : 0;
}
#elif defined(_WIN32)
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
// clang-format off
static const uint8_t FENSTER_KEYCODES[] = {0,27,49,50,51,52,53,54,55,56,57,48,45,61,8,9,81,87,69,82,84,89,85,73,79,80,91,93,10,0,65,83,68,70,71,72,74,75,76,59,39,96,0,92,90,88,67,86,66,78,77,44,46,47,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,17,3,0,20,0,19,0,5,18,4,26,127};
// clang-format on
typedef struct BINFO{
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[3];
}BINFO;
static LRESULT CALLBACK fenster_wndproc(HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam) {
  struct fenster *f = (struct fenster *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (msg) {
  case WM_GETMINMAXINFO:
  {
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    if (f!=NULL)
    {
      RECT wr = {0, 0, 0, 0};
      wr.right = f->width;
      wr.bottom = f->height;
      AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CLIENTEDGE);
      
      lpMMI->ptMinTrackSize.x = wr.right-wr.left;
      lpMMI->ptMinTrackSize.y = wr.bottom-wr.top;
    }
  } break;
  case WM_PAINT: {
    BITMAPINFO bmi = {
      .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
      .bmiHeader.biBitCount = 32,
      .bmiHeader.biCompression = BI_RGB,
      .bmiHeader.biPlanes = 1,
      .bmiHeader.biWidth = f->width,
      .bmiHeader.biHeight = -f->height
    };
    f->scale = MIN((float)f->display_width/f->width, (float)f->display_height/f->height);
    int draw_width = f->width*f->scale;
    int draw_height = f->height*f->scale;
    f->xorigin=(f->display_width-draw_width)/2;
    f->yorigin=(f->display_height-draw_height)/2;
    HDC hDC = GetDC(hwnd);
    StretchDIBits(hDC,
      f->xorigin, f->yorigin, draw_width, draw_height,
      0, 0, f->width, f->height,
      f->buf, &bmi, DIB_RGB_COLORS, SRCCOPY);
    ValidateRect(hwnd,0);  
    ReleaseDC(hwnd, hDC);
  } break;
  case WM_SIZE:{
    f->display_width = LOWORD(lParam);
    f->display_height = HIWORD(lParam);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &ps.rcPaint, brush);
    DeleteObject(brush);
    EndPaint(hwnd, &ps);
    RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
  } break;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    f->hwnd = NULL;
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
    f->mouse = (msg == WM_LBUTTONDOWN);
    break;
  case WM_MOUSEMOVE:
    f->y = ((float)HIWORD(lParam) - (float)f->yorigin)/f->scale;
    f->x = ((float)LOWORD(lParam) - (float)f->xorigin)/f->scale;
    break;
  case WM_KEYDOWN:
  case WM_KEYUP: {
    f->mod = ((GetKeyState(VK_CONTROL) & 0x8000) >> 15) |
             ((GetKeyState(VK_SHIFT) & 0x8000) >> 14) |
             ((GetKeyState(VK_MENU) & 0x8000) >> 13) |
             (((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) >> 12);
    f->keys[FENSTER_KEYCODES[HIWORD(lParam) & 0x1ff]] = !((lParam >> 31) & 1);
  } break;
  case WM_DESTROY:
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

FENSTER_API int fenster_open(struct fenster *f) {
  HINSTANCE hInstance = GetModuleHandle(NULL);
  WNDCLASSEX wc = {0};
  RECT wr = {0, 0, 0, 0};
  wr.right = f->width;
  wr.bottom = f->height;
  AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CLIENTEDGE);
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.lpfnWndProc = fenster_wndproc;
  wc.hInstance = hInstance;
  wc.lpszClassName = f->title;
  RegisterClassEx(&wc);
  f->hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, f->title, f->title,
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, hInstance, NULL);

  if (f->hwnd == NULL)
    return -1;
  SetWindowLongPtr(f->hwnd, GWLP_USERDATA, (LONG_PTR)f);
  ShowWindow(f->hwnd, SW_NORMAL);
  UpdateWindow(f->hwnd);
  return 0;
}

FENSTER_API void fenster_close(struct fenster *f) { 
	if (f->hwnd != NULL) {
		DestroyWindow(f->hwnd);
		f->hwnd = NULL;
	}
}

FENSTER_API int fenster_loop(struct fenster *f) {
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (f->hwnd == NULL)
      return -1;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  InvalidateRect(f->hwnd, NULL, TRUE);
  return 0;
}
#else
// clang-format off
static int FENSTER_KEYCODES[124] = {XK_BackSpace,8,XK_Delete,127,XK_Down,18,XK_End,5,XK_Escape,27,XK_Home,2,XK_Insert,26,XK_Left,20,XK_Page_Down,4,XK_Page_Up,3,XK_Return,10,XK_Right,19,XK_Tab,9,XK_Up,17,XK_apostrophe,39,XK_backslash,92,XK_bracketleft,91,XK_bracketright,93,XK_comma,44,XK_equal,61,XK_grave,96,XK_minus,45,XK_period,46,XK_semicolon,59,XK_slash,47,XK_space,32,XK_a,65,XK_b,66,XK_c,67,XK_d,68,XK_e,69,XK_f,70,XK_g,71,XK_h,72,XK_i,73,XK_j,74,XK_k,75,XK_l,76,XK_m,77,XK_n,78,XK_o,79,XK_p,80,XK_q,81,XK_r,82,XK_s,83,XK_t,84,XK_u,85,XK_v,86,XK_w,87,XK_x,88,XK_y,89,XK_z,90,XK_0,48,XK_1,49,XK_2,50,XK_3,51,XK_4,52,XK_5,53,XK_6,54,XK_7,55,XK_8,56,XK_9,57};
// clang-format on
FENSTER_API int fenster_open(struct fenster *f) {
  f->dpy = XOpenDisplay(NULL);
  int screen = DefaultScreen(f->dpy);
  f->w = XCreateSimpleWindow(f->dpy, RootWindow(f->dpy, screen), 0, 0, f->width,
                             f->height, 0, BlackPixel(f->dpy, screen),
                             WhitePixel(f->dpy, screen));
  f->gc = XCreateGC(f->dpy, f->w, 0, 0);
  XSelectInput(f->dpy, f->w,
               ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
                   ButtonReleaseMask | PointerMotionMask);
  XStoreName(f->dpy, f->w, f->title);
  XMapWindow(f->dpy, f->w);
  XSync(f->dpy, f->w);
  f->img = XCreateImage(f->dpy, DefaultVisual(f->dpy, 0), 24, ZPixmap, 0,
                        (char *)f->buf, f->width, f->height, 32, 0);
  return 0;
}
FENSTER_API void fenster_close(struct fenster *f) { XCloseDisplay(f->dpy); }
FENSTER_API int fenster_loop(struct fenster *f) {
  XEvent ev;
  XPutImage(f->dpy, f->w, f->gc, f->img, 0, 0, 0, 0, f->width, f->height);
  XFlush(f->dpy);
  while (XPending(f->dpy)) {
    XNextEvent(f->dpy, &ev);
    switch (ev.type) {
    case ButtonPress:
    case ButtonRelease:
      f->mouse = (ev.type == ButtonPress);
      break;
    case MotionNotify:
      f->x = ev.xmotion.x, f->y = ev.xmotion.y;
      break;
    case KeyPress:
    case KeyRelease: {
      int m = ev.xkey.state;
      int k = XkbKeycodeToKeysym(f->dpy, ev.xkey.keycode, 0, 0);
      for (unsigned int i = 0; i < 124; i += 2) {
        if (FENSTER_KEYCODES[i] == k) {
          f->keys[FENSTER_KEYCODES[i + 1]] = (ev.type == KeyPress);
          break;
        }
      }
      f->mod = (!!(m & ControlMask)) | (!!(m & ShiftMask) << 1) |
               (!!(m & Mod1Mask) << 2) | (!!(m & Mod4Mask) << 3);
    } break;
    }
  }
  return 0;
}
#endif

#ifdef _WIN32
FENSTER_API void fenster_sleep(int64_t ms) { Sleep(ms); }
FENSTER_API int64_t fenster_time() {
  LARGE_INTEGER freq, count;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&count);
  return (int64_t)(count.QuadPart * 1000.0 / freq.QuadPart);
}
#else
FENSTER_API void fenster_sleep(int64_t ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
}
FENSTER_API int64_t fenster_time(void) {
  struct timespec time;
  clock_gettime(CLOCK_REALTIME, &time);
  return time.tv_sec * 1000 + (time.tv_nsec / 1000000);
}
#endif

#ifdef __cplusplus
class Fenster {
  struct fenster f;
  int64_t now;

public:
  Fenster(const int w, const int h, const char *title)
      : f{.title = title, .width = w, .height = h} {
    this->f.buf = new uint32_t[w * h];
    this->now = fenster_time();
    fenster_open(&this->f);
  }
  ~Fenster() {
    fenster_close(&this->f);
    delete[] this->f.buf;
  }
  bool loop(const int fps) {
    int64_t t = fenster_time();
    if (t - this->now < 1000 / fps) {
      fenster_sleep(t - now);
    }
    this->now = t;
    return fenster_loop(&this->f) == 0;
  }
  inline uint32_t &px(const int x, const int y) {
    return fenster_pixel(&this->f, x, y);
  }
  bool key(int c) { return c >= 0 && c < 128 ? this->f.keys[c] : false; }
  int x() { return this->f.x; }
  int y() { return this->f.y; }
  int mouse() { return this->f.mouse; }
  int mod() { return this->f.mod; }
};
#endif /* __cplusplus */

#endif /* !FENSTER_HEADER */
#endif /* FENSTER_H */
