/* See LICENSE file for copyright and license details. */
#include <sys/types.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include <Imlib2.h>

#include "arg.h"
#include "util.h"
#include "drw.h"

char *argv0;

/* macros */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(a)         (sizeof(a) / sizeof(a)[0])
#define TEXTW(X)          (drw_fontset_getwidth(drw, (X)) + lrpad)

enum { SchemeNorm, SchemeSel, SchemeBar }; /* color schemes */
enum { LEFT, RIGHT, UP, DOWN }; /* 2d directions */
enum { WMDelete, WMName, WMFullscreen, WMState,
	WMLast }; /* atoms */

/* Purely graphic info */
typedef struct {
	Display *dpy;
	//Colormap cmap;
	Window win;
	Atom wmdeletewin, netwmname;
	//Atom xembed, wmdeletewin, netwmname, netwmpid;
	//Visual *vis;
	XSetWindowAttributes attrs;
	//int depth; /* bit depth */
	int scr;
	int scrw, scrh;
	//int x, y;
	int w, h;
	//pixmap needed?
	//Pixmap pm;
	//unsigned int pmw, pmh;
	//int gm; /* geometry mask */
} XWindow;

typedef struct {
 	//Imlib_Image *im;
 	unsigned char re; /* rendered */
	int w, h; /* position */
	int x, y; /* dimeniton */
} Image;

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int b;
	void (*func)(const Arg *);
	const Arg arg;
} Mousekey;

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Shortcut;


static void cleanup(void);
static void quit(const Arg *arg);
static void run(void);
static void usage(void);
static void xhints(void);
//static void xinit();
static void setup(void);
static void drawbar(void);
static void update_title(void);

/* X events */
static void bpress(XEvent *);
static void cmessage(XEvent *);
static void expose(XEvent *);
static void kpress(XEvent *);
static void configurenotify(XEvent *);

//image
static void imlib_init(void);
static void im_clear(void);
static void imlib_destroy();
static int img_load(Image *img, const char *filename);
static void img_render(Image *img, XWindow *win);
static int img_zoom(Image *img, float z);
static void img_check_pan(Image *img, XWindow *win);

/* commands */
static void togglebar(const Arg *arg);
//static void next_img(const Arg *arg);
//static void prev_img(const Arg *arg);
static void advance(const Arg *arg);
static void printfile(const Arg *arg);
static void zoom(const Arg *arg);
static void togglefullscreen(const Arg *arg);

/* config.h for applying patches and the configuration. */
#include "config.h"

enum {
	SCALE_DOWN,
	SCALE_FIT,
	SCALE_ZOOM
} scalemode;


/* Globals */
static unsigned int numlockmask = 0; //should this be handled at all? (updatenumlockmask)

static Atom atom[WMLast];
static char stext1[256], stext2[256];
static const char **filenames;
static unsigned int fileidx = 0, filecnt = 0;

static XWindow xw;
static Image image;
static Drw *drw;
static Clr **scheme;
static int running = 1;
static int bh = 0;      /* bar geometry */
static int by;		/* bar y */
static int lrpad;       /* sum of left and right padding for text */

/* zoom */
static float zoomlvl;	//this variable is global since functionally it's a global feature (applies to all images)
static int zl_cnt;	//idx of the zoom[]
static float zoom_min, zoom_max;

/* x init*/
static Colormap cmap;
//static Drawable xpix = 0;
static int depth;
static Visual *visual;

static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress] = bpress,
	[ClientMessage] = cmessage,
	[ConfigureNotify] = configurenotify,
	[Expose] = expose,
	[KeyPress] = kpress,
};

#include "image.c"
#include "cmd.c"

void
cleanup(void)
{
	unsigned int i;

	imlib_destroy();
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(filenames); //this should be free'ed?
	free(scheme);
	drw_free(drw);
	XDestroyWindow(xw.dpy, xw.win);
	XSync(xw.dpy, False);
	XCloseDisplay(xw.dpy);
}


void
updatebarpos()
{
	//img.h -= bh;
	//img.y -= bh;
	//bh = showbar ? drw->fonts->h + 2 : 0;

	//TODO

	if (bh != 0) {
		xw.h += bh;
		bh = 0;
	} else {
		bh = drw->fonts->h + 2;
		xw.h -= bh;
	}

}
void
update_title()
{
	char title[512];

	snprintf(title, LENGTH(title), "mage: [%d/%d] <%d%%> %s", fileidx + 1,
	             filecnt, (int) (zoomlvl * 100.0), filenames[fileidx]);

	XChangeProperty(xw.dpy, xw.win, atom[WMName],
	                XInternAtom(xw.dpy, "UTF8_STRING", False), 8,
	                PropModeReplace, (unsigned char *) title, strlen(title));
}

void
drawbar(void)
{
	int y, tw = 0;
	char right[256], left[256];
	/* bar elements */
	//int bzoom = (int) zoom * 100.0;
	//int bidx = fileidx + 1;

	if (showbar) //hack? to not drawbar
		return;

	tw = TEXTW(stext2) - lrpad + 2; /* 2px right padding */

	/* currently topbar is not supported */
	//y = topbar ? 0 : xw.h - bh;
	y = xw.h - bh;

	drw_setscheme(drw, scheme[SchemeBar]);
	/* left text */
	drw_text(drw, 0, y, xw.w/2, bh, lrpad / 2, stext1, 0);
	snprintf(left, LENGTH(left), "%s", filenames[fileidx]);
	strcpy(stext1, left);

	/* right text */
	drw_text(drw, xw.w/2, y, xw.w/2, bh, xw.w/2 - (tw + lrpad / 2), stext2, 0);
	snprintf(right, LENGTH(right), "<%d%%> [%d/%d]", (int)(zoomlvl * 100.0), fileidx + 1, filecnt);
	strcpy(stext2, right);

	drw_map(drw, xw.win, 0, y, xw.w, bh);
}

void
quit(const Arg *arg)
{
	running = 0;
}

void
run(void)
{
	XEvent ev;

	while (running) {
		XNextEvent(xw.dpy, &ev);
		if (handler[ev.type])
			(handler[ev.type])(&ev);
	}
}

void
xhints()
{
	XClassHint class = {.res_name = "mage", .res_class = "mage"};
	XWMHints wm = {.flags = InputHint, .input = True};
	XSizeHints *sizeh = NULL;

	if (!(sizeh = XAllocSizeHints()))
		die("mage: Unable to allocate size hints");

	sizeh->flags = PSize;
	sizeh->height = xw.h;
	sizeh->width = xw.w;

	XSetWMProperties(xw.dpy, xw.win, NULL, NULL, NULL, 0, sizeh, &wm, &class);
	XFree(sizeh);
}

void
bpress(XEvent *e)
{
	unsigned int i;

	for (i = 0; i < LENGTH(mshortcuts); i++)
		if (e->xbutton.button == mshortcuts[i].b && mshortcuts[i].func)
			mshortcuts[i].func(&(mshortcuts[i].arg));
}

void
cmessage(XEvent *e)
{
	if (e->xclient.data.l[0] == xw.wmdeletewin)
		running = 0;
}

//to expose or not
void
expose(XEvent *e)
{
	//img_render doesn't update position
	//img_render(&img, &xw);

	//img_load
	//img_load(&img, filenames[++fileidx]);

	//img_display might be the right one, it's slow on big images
	//img_display(&img, &xw);
	img_render(&image, &xw);
	drawbar();

	//if (0 == e->xexpose.count) {
	//	//printf("expose\n");
	//img_render(&img, &xw, e->xexpose.x, e->xexpose.y, xw.w, xw.h);
	//	img_render(&img, &xw);
	//	//drw_map(drw, xw.win, e->xexpose.x, e->xexpose.y, e->xexpose.width, e->xexpose.height);
	//	//drawbar();
	//}
}
void
kpress(XEvent *e)
{
	const XKeyEvent *ev = &e->xkey;
	unsigned int i;
	KeySym keysym;

	keysym = XkbKeycodeToKeysym(xw.dpy, (KeyCode)ev->keycode, 0, 0);
	for (i = 0; i < LENGTH(shortcuts); i++)
		if (keysym == shortcuts[i].keysym
		&& CLEANMASK(shortcuts[i].mod) == CLEANMASK(ev->state)
		&& shortcuts[i].func)
			shortcuts[i].func(&(shortcuts[i].arg));
}

void
configurenotify(XEvent *e)
{
	XConfigureEvent *ev = &e->xconfigure;
	if (xw.w != ev->width || xw.h != ev->height) {
		xw.w = ev->width;
		xw.h = ev->height;
		//scalemode = SCALE_DOWN;
		drw_resize(drw, xw.w, xw.h);
	}
}

void
usage()
{
	die("usage: %s [-hv] file...", argv0);
}

void
setup(void)
{
	int i;
	XTextProperty prop;

	if (!(xw.dpy = XOpenDisplay(NULL)))
		die("mage: Unable to open display");

	/* init screen */
	xw.scr  = DefaultScreen(xw.dpy);
	xw.scrw = DisplayWidth(xw.dpy, xw.scr);
	xw.scrh = DisplayHeight(xw.dpy, xw.scr);

	visual = DefaultVisual(xw.dpy, xw.scr);
	cmap   = DefaultColormap(xw.dpy, xw.scr);
	depth  = DefaultDepth(xw.dpy, xw.scr);

	if (!xw.w)
		xw.w = xw.scrw;
	if (!xw.h)
		xw.h = xw.scrh;

 	xw.attrs.colormap = cmap;
	//xw.attrs.background_pixel = 0;
	//xw.attrs.border_pixel = 0;
	xw.attrs.save_under = False;
	xw.attrs.bit_gravity = CenterGravity;
	xw.attrs.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask |
	                      ButtonMotionMask | ButtonPressMask;

	xw.win = XCreateWindow(xw.dpy, XRootWindow(xw.dpy, xw.scr), 0, 0,
				xw.w, xw.h, 0, depth, InputOutput, visual,
				0, &xw.attrs);
				//CWBackPixel | CWColormap | CWBorderPixel, &xw.attrs);
				//CWBitGravity | CWEventMask, &xw.attrs);
			       //CWBackingStore | CWBitGravity | CWEventMask, &xw.attrs);

	//XSelectInput(xw.dpy, xw.win, ButtonReleaseMask | ButtonPressMask | KeyPressMask |
	  //           PointerMotionMask | StructureNotifyMask);

	XSelectInput(xw.dpy, xw.win, StructureNotifyMask | ExposureMask | KeyPressMask |
	                       ButtonPressMask);

	/* init atoms */
	atom[WMDelete] = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
	atom[WMName] = XInternAtom(xw.dpy, "_NET_WM_NAME", False);
	atom[WMFullscreen] = XInternAtom(xw.dpy, "_NET_WM_STATE_FULLSCREEN", False);
	atom[WMState] = XInternAtom(xw.dpy, "_NET_WM_STATE", False);

	XSetWMProtocols(xw.dpy, xw.win, &atom[WMDelete], 1);

	if (!(drw = drw_create(xw.dpy, xw.scr, xw.win, xw.w, xw.h)))
		die("mage: Unable to create drawing context");

	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	XSetWindowBackground(xw.dpy, xw.win, scheme[SchemeNorm][ColBg].pixel);

	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;
	//by = topbar ? 0 : xw.h - bh;

	/* init bars */
	drawbar();

	XStringListToTextProperty(&argv0, 1, &prop);
	XSetWMName(xw.dpy, xw.win, &prop);
	XSetTextProperty(xw.dpy, xw.win, &prop, atom[WMName]);
	XFree(prop.value);
	XMapWindow(xw.dpy, xw.win);
	xhints();
	XSync(xw.dpy, False);

	/* init imlib */
	imlib_init();
	img_load(&image, filenames[fileidx]);
	img_render(&image, &xw);

	/* init title */
 	update_title();
}

int
main(int argc, char *argv[])
{
	int i;

	ARGBEGIN {
	case 'v':
		fprintf(stderr, "mage-"VERSION"\n");
		return 0;
	case 'h':
		usage();
	case 'j':
		printf("This is optind %d\n", optind);
		printf("This is argc %d\n", argc);
	default:
		usage();
	} ARGEND

	if (!argv[0])
		usage();

	//temporal variables so we can later evaluate if they truly are files
	const char **files = (const char**) argv + optind - 1;
	int cnt = argc - optind + 1;

	//separate all the space of cnt, even if there is some that we won't use
	if (!(filenames =  malloc(cnt * sizeof(char*))))
		die("could not allocate memory");


	// tmp
	//zoomlvl = 1.0;
	scalemode = SCALE_DOWN;

	for (i = 0; i < cnt; i++)
		//return code so you can evaluate this. This is so it can load
		//as much images as posible
		if (img_load(&image, files[i]) == 0)
			// we finally pass only files that imlib2 can load (return 0)
			//imo this is better than using fopen (since it may be a file but not an image)
			filenames[filecnt++] = files[i];

	if (!filecnt)
		die("no valid image filename given, aborting");

	setup();

	run();

	cleanup();
	return 0;
}
