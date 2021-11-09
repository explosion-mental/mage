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

//to handle directories
#include <dirent.h>
//TODO verify what headers are really needed

char *argv0;

/* macros */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(a)               (sizeof(a) / sizeof(a)[0])
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

enum { SchemeNorm, SchemeSel, SchemeBar }; /* color schemes */
enum { LEFT, RIGHT, UP, DOWN }; /* 2d directions */
enum { WMDelete, WMName, WMFullscreen, WMState,
	WMLast }; /* atoms */

typedef enum {
	SCALE_DOWN,
	SCALE_FIT,
	SCALE_ZOOM
} scaling;

typedef struct {
	Display *dpy;
	//Colormap cmap;
	Window win;
	//Visual *vis;
	XSetWindowAttributes attrs;
	//int depth; /* bit depth */
	int scr;
	int scrw, scrh;
	//int x, y;
	int w, h;
	Pixmap pm;
	GC gc;
	//int gm; /* geometry mask */
} XWindow;

typedef struct {
 	unsigned char re; /* rendered */
	unsigned char checkpan;
	//zoomed;
	scaling scalemode;
	unsigned char aa; /* antialias */
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

/* function declarations */
static void cleanup(void);
static void run(void);
static void usage(void);
static void xhints(void);
static void setup(void);
static void drawbar(void);
static void update_title(void);

/* X events */
static void bpress(XEvent *);
static void cmessage(XEvent *);
static void expose(XEvent *);
static void kpress(XEvent *);
static void configurenotify(XEvent *);

/* image */
static void im_init(void);
//static void im_clear(void);
static void im_destroy(void);
static int im_load(const char *filename);
static int img_load(Image *img, const char *filename);
static void img_render(Image *img);
static int img_zoom(Image *img, float z);
static void img_check_pan(Image *img);

/* commands */
static void togglebar(const Arg *arg);
static void quit(const Arg *arg);
static void advance(const Arg *arg);
static void printfile(const Arg *arg);
static void zoom(const Arg *arg);
static void togglefullscreen(const Arg *arg);
static void pan(const Arg *arg);
static void first(const Arg *arg);
static void last(const Arg *arg);
static void rotate(const Arg *arg);
static void toggleantialias(const Arg *arg);
static void img_center(const Arg *arg);
static void img_fit(const Arg *arg);
static void reload(const Arg *arg);
static void cyclescale(const Arg *arg);

/* handling files */
static int check_img(const char *filename);
static void check_file(const char *file);
static char *readstdin(FILE *stream);

/* variables */
static Atom atom[WMLast];
static char right[128], left[128];
static const char **filenames;
static unsigned int fileidx = 0, filecnt = 0;

static XWindow xw;
static Image image;
static Drw *drw;
static Clr **scheme;
static int running = 1;
static int bh = 0;      /* bar geometry */
//static int by;		/* bar y */
static int lrpad;       /* sum of left and right padding for text */
static char *wmname = "mage";
//static char *scales[] = { "ScaleDown}

static unsigned int numlockmask = 0; //should this be handled at all? (updatenumlockmask)

/* zoom */
static float zoomlvl;	//this variable is global since functionally it's a global feature (applies to all images)h
static int zl_cnt;	//idx of the zoom[]
static float zoom_min, zoom_max;

/* x init*/
static Colormap cmap;
static int depth;
static Visual *visual;

/* config.h for applying patches and the configuration. */
#include "config.h"

static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress] = bpress,
	[ClientMessage] = cmessage,
	[ConfigureNotify] = configurenotify,
	[Expose] = expose,
	[KeyPress] = kpress,
};

//at the end everything should be merged
#include "image.c"
#include "cmd.c"

void
cleanup(void)
{
	unsigned int i;

	im_destroy();
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(filenames); //this should be free'ed?
	free(scheme);
	drw_free(drw);
 	XFreeGC(xw.dpy, xw.gc);
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

	if (showbar) //hack to not drawbar
		return;

	/* currently topbar is not supported */
	//y = topbar ? 0 : xw.h - bh;
	y = xw.h - bh;

	drw_setscheme(drw, scheme[SchemeBar]);

	/* left text */
	snprintf(left, LENGTH(left), "%s %dx%d", filenames[fileidx], image.w, image.h);
	drw_text(drw, 0, y, xw.w/2, bh, lrpad / 2, left, 0);

	/* right text */
	snprintf(right, LENGTH(right), "~%d <%d%%> [%d/%d]", scalemode, (int)(zoomlvl * 100.0), fileidx + 1, filecnt);
	tw = TEXTW(right) - lrpad + 2; /* 2px right padding */
	drw_text(drw, xw.w/2, y, xw.w/2, bh, xw.w/2 - (tw + lrpad / 2), right, 0);

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
	XClassHint class;
	class.res_name = wmname;
	class.res_class = "mage";

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
	if (e->xclient.data.l[0] == atom[WMDelete])
		running = 0;
}

void
expose(XEvent *e)
{
	if (0 == e->xexpose.count) {
		//drw_map(drw, xw.win, e->xexpose.x, e->xexpose.y, e->xexpose.width, e->xexpose.height);
		//drw_resize(drw, e->xexpose.width, e->xexpose.height);
		image.checkpan = 1; //hack to redraw image
		img_render(&image);
		drawbar();
	}
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

int
check_img(const char *filename)
{
	int ret = im_load(filename);

	if (ret == 0) {
		imlib_free_image();
		if (fileidx == filecnt) {
			//filecnt *= 2;
			filecnt++; //+ 1 for every new arg we add
			if (!(filenames = realloc(filenames, filecnt * sizeof(const char *))))
				die("mage: could not allocate memory img_check");
		}
		filenames[fileidx++] = filename;
	}

	return ret;
}

void
check_file(const char *file)
{
	char *filename;
	const char **dirnames;
	int dircnt = 256, diridx;
	unsigned char first;
	size_t len;
	struct dirent *dentry;
	DIR *dir;

	//todo handle first if the file exist

	if (access(file, F_OK) != -1 ) {
		/* check if it's an image */
		if (check_img(file) == 0)
			//if it isn't an image we don't return, but rather pass
			//to check if it's a directory (hack)
			return;
	} else {
		if (!quiet)
			fprintf(stderr, "mage: file %s does not exist\n", file);
		return;
	}

	diridx = first = 1;
	if (!(dirnames = (const char**) malloc(dircnt * sizeof(const char*))))
		die("could not allocate memory");
	dirnames[0] = file;

	/* check if it's a directory */
	//do we need to check this? if it isn't a file then what is this?
	if ((dir = opendir(file))) {
		/* handle directory */
		while (diridx > 0) {
			file = dirnames[--diridx];
			while ((dentry = readdir(dir))) {
				if (!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, ".."))
					continue;
				len = strlen(file) + strlen(dentry->d_name) + 2;
				if (!(filename = malloc(len * sizeof(char))))
					die("could not allocate memory");
				snprintf(filename, len, "%s/%s", file, dentry->d_name);
				if (recursive)
					check_file(filename);
				else
					if (check_img(filename) != 0)
						free(filename);
			}
			closedir(dir);
			if (!first)
				free((void*) file);
			else
				first = 0;
		}
		free(dirnames);
		return;
	} else if (ENOENT == errno) { /* directory does not exist */
		if (!quiet)
			fprintf(stderr, "mage: directory doesn't exist %s\n", file);
		return;
	} else /* opendir() failed for some other reason */
		return;
}

char *
readstdin(FILE *stream)
{
	size_t len;
	char *buf, *s, *end;

	if (!stream || feof(stream) || ferror(stream))
		return NULL;

	len = 4096;
	if (!(s = buf = (char*) malloc(len * sizeof(char))))
		die("mage: could not allocate memory");

	do {
		*s = '\0';
		fgets(s, len - (s - buf), stream);
		if ((end = strchr(s, '\n'))) {
			*end = '\0';
		} else if (strlen(s) + 1 == len - (s - buf)) {
			buf = (char*) realloc(buf, 2 * len * sizeof(char));
			s = buf + len - 1;
			len *= 2;
		} else
			s += strlen(s);
	} while (!end && !feof(stream) && !ferror(stream));

	if (!ferror(stream) && *buf) {
		s = (char*) malloc((strlen(buf) + 1) * sizeof(char));
		strcpy(s, buf);
	} else
		s = NULL;

	free(buf);

	return s;
}

void
setup(void)
{
	int i;
	XTextProperty prop;
	XGCValues gcval;

	if (!(xw.dpy = XOpenDisplay(NULL)))
		die("mage: Unable to open display");

	/* init screen */
	xw.scr  = DefaultScreen(xw.dpy);
	xw.scrw = DisplayWidth(xw.dpy, xw.scr);
	xw.scrh = DisplayHeight(xw.dpy, xw.scr);
	visual = DefaultVisual(xw.dpy, xw.scr);
	cmap   = DefaultColormap(xw.dpy, xw.scr);
	depth  = DefaultDepth(xw.dpy, xw.scr);
	xw.pm = 0;

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

	//XSetWindowBackground(xw.dpy, xw.win, scheme[SchemeNorm][ColBg].pixel);
	gcval.foreground = scheme[SchemeNorm][ColBg].pixel;
	xw.gc = XCreateGC(xw.dpy, xw.win, GCForeground, &gcval);
	xw.pm = 0;

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
	im_init();
	img_load(&image, filenames[fileidx]);
	img_render(&image);

	/* init title */
 	update_title();
}

void
usage()
{
	die("usage: %s [-fhprv] [-n class] file...", argv0);
}

int
main(int argc, char *argv[])
{
	int i;
	int fs = 0;

	ARGBEGIN {
	case 'v':
		die("mage-"VERSION);
	case 'f':
		fs = 1;
		break;
	case 'h':
		usage();
	case 'p':
		antialiasing = 0;
		break;
	case 'r':
		recursive = 1;
		break;
	case 'n':
		wmname = EARGF(usage());
		break;
	case 'q':
		quiet = 1;
		break;
	default:
		usage();
	} ARGEND

	if (!argv[0])
		usage();

	//temporal variables so we can later evaluate if they truly are files
	const char **files = (const char**) argv + optind - 1;
	int cnt = argc - optind + 1;
	const char *input;

	/* handle only images or directories */
	if (!strcmp(argv[0], "-")) {
		while ((input = readstdin(stdin)))
			check_file(input);
	} else {
		for (i = 0; i < cnt; i++)
			check_file(files[i]);
	}

	filecnt = fileidx;
	fileidx = 0;

	if (!filecnt)
		die("mage: no valid image filename given, aborting");

	setup();

	if (fs) togglefullscreen(0);

	run();

	cleanup();
	return 0;
}
