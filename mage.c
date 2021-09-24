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
#define LENGTH(a)         (sizeof(a) / sizeof(a)[0])
#define TEXTW(X)          (drw_fontset_getwidth(drw, (X)) + lrpad)

enum { SchemeNorm, SchemeSel, SchemeBar }; /* color schemes */

/* Purely graphic info */
typedef struct {
	Display *dpy;
	Colormap cmap;
	Window win;
	Atom wmdeletewin, netwmname;
	//Atom xembed, wmdeletewin, netwmname, netwmpid;
	Visual *vis;
	XSetWindowAttributes attrs;
	int depth; /* bit depth */
	int scr;
	int w, h;
	Pixmap pm;
	unsigned int pmw, pmh;
	//Pixmap pmap; ?
	//int isfixed; /* is fixed geometry? */
	//int l, t; /* left and top offset */
	//int gm; /* geometry mask */
} XWindow;

typedef struct {
 	Imlib_Image *im;
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
	//unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Shortcut;


static void cleanup(void);
static void quit(const Arg *arg);
static int resize(int width, int height);
static void run();
static void usage();
static void xhints();
static void xinit();

/* X events */
static void bpress(XEvent *);
static void cmessage(XEvent *);
static void expose(XEvent *);
static void kpress(XEvent *);
static void configurenotify(XEvent *);

/* commands */
static void togglebar();

/* config.h for applying patches and the configuration. */
#include "config.h"

//enum {	ATOM_WM_DELETE_WINDOW,
//	ATOM__NET_WM_NAME,
//	ATOM__NET_WM_ICON_NAME,
//	ATOM__NET_WM_ICON,
//	ATOM__NET_WM_STATE,
//	ATOM__NET_WM_PID,
//	ATOM__NET_WM_STATE_FULLSCREEN,
//	ATOM_COUNT };
//Atom atoms[ATOM_COUNT];

typedef enum {
	SCALE_DOWN = 0,
	SCALE_FIT,
	SCALE_ZOOM
} scales_t;


/* Globals */
//static char **fname;
static char stext1[256], stext2[256];
static const char **fnames;
static unsigned int fileidx, filecnt;
static scales_t scalemode;

static XWindow xw;
static Image img;
static Drw *drw;
static Clr **scheme;
static int running = 1;
static int bh = 0;      /* bar geometry */
static int lrpad;       /* sum of left and right padding for text */

static float zoom;

static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress] = bpress,
	[ClientMessage] = cmessage,
	[ConfigureNotify] = configurenotify,
	[Expose] = expose,
	[KeyPress] = kpress,
};

#include "image.c"

int
filter(int fd, const char *cmd)
{
	int fds[2];

	if (pipe(fds) < 0)
		die("mage: Unable to create pipe:");

	switch (fork()) {
	case -1:
		die("mage: Unable to fork:");
	case 0:
		dup2(fd, 0);
		dup2(fds[1], 1);
		close(fds[0]);
		close(fds[1]);
		execlp("sh", "sh", "-c", cmd, (char *)0);
		fprintf(stderr, "mage: execlp sh -c '%s': %s\n", cmd, strerror(errno));
		_exit(1);
	}
	close(fds[1]);
	return fds[0];
}

void
cleanup()
{
	unsigned int i;
	static int in = 0;

	if (!in++) {
		imlib_destroy();
	}

	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
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

	resize(xw.w , xw.h);
//	if (showbar) {
//		printf("BAR ENABLED\n");
//		xw.h = xw.h + bh;
//		//img.h -= bh;
//		//img.y -= bh;
//		bh = 0;
//	} else {
//		printf("bar disabled\n");
//		bh = drw->fonts->h + 2;
//		xw.h = xw.h - bh;
//		resize(xw.h, xw.w);
//	}
}

static void
drawbar(void)
{
	//int x = 0, w = 0;
	int tw1 = 0, tw2 = 0;
	//int boxs = drw->fonts->h / 9;
	//int boxw = drw->fonts->h / 9;
	char ex1[] = "[START]left[END]";
	char ex2[] = "[start]RIGHT[end]";
	int y;

	XClearWindow(xw.dpy, xw.win);
	drw_setscheme(drw, scheme[SchemeBar]);
	tw1 = TEXTW(stext1) - lrpad + 2; /* 2px right padding */
	tw2 = TEXTW(stext2) - lrpad + 2; /* 2px right padding */

	y = topbar ? 0 : xw.h - bh;

	/* left text */
	//Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert)
	drw_text(drw, 0, y, xw.w/2, bh, lrpad / 2, stext1, 0);
	//drw_rect(drw, 0, 0, xw.w, bh, 0, 1);

	/* right text */
	drw_text(drw, xw.w/2, y, xw.w/2, bh, (xw.w/2 - (tw2 + (lrpad / 2)) ), stext2, 0);
	strcpy(stext1, fnames[0]);
	strcpy(stext2, "Right");
	//drw_rect(drw, 0, 0, xw.w, bh, 0, 1);
	//drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
	drw_map(drw, xw.win, 0, y, xw.w, bh);
}

void
togglebar()
{
	showbar = !showbar;
	updatebarpos();
	//drawbar();
}

void
quit(const Arg *arg)
{
	running = 0;
}

int
resize(int width, int height)
{
	//int changed;

	int changed = xw.w != width || xw.h != height;
	//if (xw.w != width || xw.h != height) {
	//win->x = cev->x;
	//win->y = cev->y;
	xw.w = width;
	xw.h = height;
	//win->bw = cev->border_width;
	drw_resize(drw, width, height);
	return changed;
}


void
run()
{
	XEvent ev;

	/* Waiting for window mapping */
	while (1) {
		XNextEvent(xw.dpy, &ev);
		if (ev.type == ConfigureNotify) {
			resize(ev.xconfigure.width, ev.xconfigure.height);
		} else if (ev.type == MapNotify) {
			break;
		}
	}

	while (running) {
		XNextEvent(xw.dpy, &ev);
		if (handler[ev.type])
			(handler[ev.type])(&ev);
	}
}

void
xhints()
{
	XClassHint class = {.res_name = "mage", .res_class = "Mage"};
	XWMHints wm = {.flags = InputHint, .input = True};
	XSizeHints *sizeh = NULL;

	if (!(sizeh = XAllocSizeHints()))
		die("mage: Unable to allocate size hints");

	sizeh->flags = PSize;
	//sizeh->flags = PWinGravity;
	//sizehints.win_gravity = NorthWestGravity;
	sizeh->height = xw.h;
	sizeh->width = xw.w;

	XSetWMProperties(xw.dpy, xw.win, NULL, NULL, NULL, 0, sizeh, &wm, &class);
	XFree(sizeh);
}

void
xinit()
{
	int i;
	XTextProperty prop;

	if (!(xw.dpy = XOpenDisplay(NULL)))
		die("mage: Unable to open display");
	xw.scr = DefaultScreen(xw.dpy);
	xw.vis = DefaultVisual(xw.dpy, xw.scr);
	xw.cmap = DefaultColormap(xw.dpy, xw.scr);
	xw.depth = DefaultDepth(xw.dpy, xw.scr);
	//xw.pm = XCreatePixmap(xw.dpy, xw.win, xw.pmw, xw.pmh, xw.depth);

	//xw.h = DisplayWidth(xw.dpy, xw.scr);
	//xw.w = DisplayHeight(xw.dpy, xw.scr);
	//to resize or not?
	resize(DisplayWidth(xw.dpy, xw.scr), DisplayHeight(xw.dpy, xw.scr));

	xw.attrs.bit_gravity = CenterGravity;
	xw.attrs.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask |
	                      ButtonMotionMask | ButtonPressMask;
	//xw.attrs.backing_store = NotUseful;
	//xw.attrs.save_under = False;

	xw.win = XCreateWindow(xw.dpy, XRootWindow(xw.dpy, xw.scr), 0, 0,
	                       xw.w, xw.h, 0, xw.depth, InputOutput, xw.vis,
			       CWBitGravity | CWEventMask, &xw.attrs);
			       //CWBackingStore | CWBitGravity | CWEventMask, &xw.attrs);

	xw.wmdeletewin = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
	xw.netwmname = XInternAtom(xw.dpy, "_NET_WM_NAME", False);

	XSetWMProtocols(xw.dpy, xw.win, &xw.wmdeletewin, 1);

	if (!(drw = drw_create(xw.dpy, xw.scr, xw.win, xw.w, xw.h)))
		die("mage: Unable to create drawing context");

	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	drw_setscheme(drw, scheme[SchemeNorm]);
	XSetWindowBackground(xw.dpy, xw.win, scheme[SchemeNorm][ColBg].pixel);

	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	//bh = drw->fonts->h + 2;
	//xw.by = drw->fonts->h + 2;

	XStringListToTextProperty(&argv0, 1, &prop); //program name
	XSetWMName(xw.dpy, xw.win, &prop);
	XSetTextProperty(xw.dpy, xw.win, &prop, xw.netwmname); //&atoms[ATOM__NET_WM_NAME]);
	XFree(prop.value);
	XMapWindow(xw.dpy, xw.win);
	xhints();
	XSync(xw.dpy, False);
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

void
expose(XEvent *e)
{
	//if (0 == e->xexpose.count) {
	//	//printf("expose\n");
	img_render(&img, &xw, e->xexpose.x, e->xexpose.y, e->xexpose.width, e->xexpose.height);
	//img_render(&img, &xw, e->xexpose.x, e->xexpose.y, xw.w, xw.h);
	//	//drw_map(drw, xw.win, e->xexpose.x, e->xexpose.y, e->xexpose.width, e->xexpose.height);
	//	//drawbar();
	//}
}

void
kpress(XEvent *e)
{
	unsigned int i;
	KeySym sym;

	sym = XkbKeycodeToKeysym(xw.dpy, (KeyCode)e->xkey.keycode, 0, 0);
	for (i = 0; i < LENGTH(shortcuts); i++)
		if (sym == shortcuts[i].keysym && shortcuts[i].func)
			shortcuts[i].func(&(shortcuts[i].arg));
}

void
configurenotify(XEvent *e)
{
	//printf("configure\n");
	resize(e->xconfigure.width, e->xconfigure.height);
	//drw_map(drw, xw.win, 0, 0, xw.w, xw.h);
	//drawbar();
	//img_render(&img, &xw, 0, 0, xw.w, xw.h);
	//if (slides[idx].img)
	//	slides[idx].img->state &= ~SCALED;
}

void
usage()
{
	die("usage: %s [file]", argv0);
}

static void
setup(void)
{
	xinit();
	//updatebarpos();
	//drawbar();

	/* imlib */
	imlib_init(&xw);
	img_load(&img, fnames[fileidx]);
	img_display(&img, &xw);
}

int
main(int argc, char *argv[])
{
	//FILE *fp = NULL;//Image *fp = NULL;
	// do really 'weird naming files segment fault'?

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

	//if (!argv[0] || !strcmp(argv[0], "-"))
	//	fp = stdin;

	//if (!filecnt)
	//	usage();

	//opts?
	//while ((int opt = getopt(argc, argv, "hv")) != -1) {
	//	switch (opt) {
	//		case '?':
	//			usage();
	//		case 'h':
	//			usage();
	//		case 'v':
	//			usage();
	//	}
	//}


	fnames = (const char**) argv + (argc - 1);
	filecnt = argc - (argc - 1);

	// tmp
	fileidx = 0;
	zoom = 1.0;
	scalemode = SCALE_DOWN;

	setup();
	printf("This is the file '%s'\n", fnames[0]);

	run();

	cleanup();
	return 0;
}
