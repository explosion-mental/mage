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
	Window win;
	Atom wmdeletewin, netwmname;
	Visual *vis;
	XSetWindowAttributes attrs;
	int scr;
	int w, h;
	int uw, uh; /* usable dimensions for drawing text and images */
	int by;     /* bar geometry */
} XWindow;

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
static void resize(int width, int height);
static void run();
static void usage();
static void xhints();
static void xinit();

static void bpress(XEvent *);
static void cmessage(XEvent *);
static void expose(XEvent *);
static void kpress(XEvent *);
static void configure(XEvent *);

/* commands */
static void togglebar();

/* config.h for applying patches and the configuration. */
#include "config.h"

/* Globals */
static const char *fname = NULL;
static char stext[256];
static XWindow xw;
static Drw *drw;
static Clr **scheme;
static int running = 1;
static int bh, blw = 0;      /* bar geometry */
static int lrpad;            /* sum of left and right padding for text */

static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress] = bpress,
	[ClientMessage] = cmessage,
	[ConfigureNotify] = configure,
	[Expose] = expose,
	[KeyPress] = kpress,
};

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

	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	drw_free(drw);
	XDestroyWindow(xw.dpy, xw.win);
	XSync(xw.dpy, False);
	XCloseDisplay(xw.dpy);
}

static void
drawbar(void)
{
	int x = 0, w = 0, tw1 = 0, tw2 = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 9;
	char ex1[] = "[START]left[END]";
	char ex2[] = "[start]RIGHT[end]";

	drw_setscheme(drw, scheme[SchemeBar]);
	tw1 = TEXTW(ex1) - lrpad + 2; /* 2px right padding */
	tw2 = TEXTW(ex2) - lrpad + 2; /* 2px right padding */

	/* left text */
	//Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert)
	drw_text(drw, 0, 0, xw.w/2, bh, 0, ex1, 0);
	//drw_rect(drw, 0, 0, xw.w, bh, 0, 1);

	/* right text */
	drw_text(drw, xw.w/2, 0, xw.w/2, bh, xw.w/2 - tw2, ex2, 0);
	//drw_rect(drw, 0, 0, xw.w, bh, 0, 1);
	//drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
	drw_map(drw, xw.win, 0, 0, xw.w, bh);
}

void
togglebar()
{
	//xw.h = xw.uh;
	//m->wh = m->mh;
	if (bh > 0) {
		printf("bar off");
		bh = 0;
		//xw.uh -= bh;
		//m->by = topbar ? xw.uh : m->wy + m->wh;
		//m->wy = topbar ? m->wy + bh : m->wy;
	} else {
		printf("bar on");
		bh = drw->fonts->h + 2;
	}
		//xw.uh = -bh;
}

void
quit(const Arg *arg)
{
	running = 0;
}

void
resize(int width, int height)
{
	xw.w = width;
	xw.h = height - bh;
	xw.uw = usablewidth * width;
	xw.uh = usableheight * height;
	drw_resize(drw, width, height);
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
	XClassHint class = {.res_name = "mage", .res_class = "image-viewer"};
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
xinit()
{
	XTextProperty prop;
	unsigned int i;

	if (!(xw.dpy = XOpenDisplay(NULL)))
		die("mage: Unable to open display");


	xw.scr = XDefaultScreen(xw.dpy);
	xw.vis = XDefaultVisual(xw.dpy, xw.scr);
	resize(DisplayWidth(xw.dpy, xw.scr), DisplayHeight(xw.dpy, xw.scr));

	xw.attrs.bit_gravity = CenterGravity;
	xw.attrs.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask |
	                      ButtonMotionMask | ButtonPressMask;

	xw.win = XCreateWindow(xw.dpy, XRootWindow(xw.dpy, xw.scr), 0, 0,
	                       xw.w, xw.h, 0, XDefaultDepth(xw.dpy, xw.scr),
	                       InputOutput, xw.vis, CWBitGravity | CWEventMask,
	                       &xw.attrs);

	xw.wmdeletewin = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
	xw.netwmname = XInternAtom(xw.dpy, "_NET_WM_NAME", False);
	XSetWMProtocols(xw.dpy, xw.win, &xw.wmdeletewin, 1);

	if (!(drw = drw_create(xw.dpy, xw.scr, xw.win, xw.w, xw.h)))
		die("mage: Unable to create drawing context");

	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	//scheme = drw_scm_create(drw, colors, 2);

	//drw_setscheme(drw, scheme[SchemeNorm]);
	XSetWindowBackground(xw.dpy, xw.win, scheme[SchemeNorm][ColBg].pixel);
	//XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);

	//xloadfonts();

	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;

	XStringListToTextProperty(&argv0, 1, &prop);
	XSetWMName(xw.dpy, xw.win, &prop);
	XSetTextProperty(xw.dpy, xw.win, &prop, xw.netwmname);
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
	if (0 == e->xexpose.count) {
		drawbar();
		printf("expose\n");
	}
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
configure(XEvent *e)
{
	printf("configure\n");
	resize(e->xconfigure.width, e->xconfigure.height);
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

	//if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
	//	die("no fonts could be loaded.");
	//lrpad = drw->fonts->h;
	//bh = drw->fonts->h + barh;
	xinit();
	drawbar();
}

int
main(int argc, char *argv[])
{
	//FILE *fp = NULL;

	ARGBEGIN {
	case 'v':
		fprintf(stderr, "mage-"VERSION"\n");
		return 0;
	default:
		usage();
	} ARGEND

	//if (!argv[0] || !strcmp(argv[0], "-"))
	//	fp = stdin;
//	if (!argv[0]) {
//		XSelectInput(xw.dpy, xw.win, ExposureMask | KeyPressMask);
//		XMapWindow(xw.dpy, xw.win);
//	}


	//if (!(fp = fopen(fname = argv[0], "r")))
	//	die("mage: Unable to open '%s' for reading:", fname);
	//load(fp);
	//fclose(fp);

	setup();
	run();

	cleanup();
	return 0;
}
