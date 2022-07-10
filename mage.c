/* See LICENSE file for copyright and license details. */
#include <sys/types.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <Imlib2.h>

char *argv0;
#include "arg.h"
#include "util.h"
#include "drw.h"

/* macros */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(a)               (sizeof(a) / sizeof(a)[0])
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define ABS(a)			((a) > 0 ? (a) : -(a))

enum { SchemeNorm, SchemeBar }; /* color schemes */
enum { WMDelete, WMName, WMFullscreen, WMState, WMLast }; /* atoms */

enum { SCALE_DOWN, SCALE_FIT, SCALE_WIDTH, SCALE_HEIGHT, }; /* custom-views */

typedef struct ScaleMode ScaleMode;

typedef struct {
	Imlib_Image *im;
 	//int re; /* rendered */
	//int redraw;
	const char *fname;
	int checkpan;
	int zoomed;
	int w, h;   /* dimension */
	float x, y; /* position */
	float z;    /* zoom */
} Image;

struct ScaleMode {
	const char *name;
	void (*arrange)(Image *im);
};


typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int b;
	int (*func)(const Arg *);
	const Arg arg;
} Mousekey;

typedef struct {
	unsigned int mod;
	KeySym keysym;
	int (*func)(const Arg *);
	const Arg arg;
} Shortcut;

/* function declarations */
static void cleanup(void);
static void run(void);
static void usage(void);
static void xhints(void);
static void setup(void);
static void drawbar(void);

/* X events */
static void bpress(XEvent *e);
static void cmessage(XEvent *e);
static void expose(XEvent *e);
static void kpress(XEvent *e);
static void configurenotify(XEvent *e);

/* image */
static void im_destroy(void);
static int img_load(Image *img);
static void img_render(Image *img);
static void img_zoom(Image *img, float z);
static void check_pan(Image *img);

/* commands */
static int togglebar(const Arg *arg);
static int quit(const Arg *arg);
static int flipvert(const Arg *arg);
static int fliphorz(const Arg *arg);
static int advance(const Arg *arg);
static int printfile(const Arg *arg);
static int zoom(const Arg *arg);
static int set_zoom(const Arg *arg);
static int togglefullscreen(const Arg *arg);
static int panhorz(const Arg *arg);
static int panvert(const Arg *arg);
static int first(const Arg *arg);
static int last(const Arg *arg);
static int rotate(const Arg *arg);
static int toggleantialias(const Arg *arg);
static int reload(const Arg *arg);
static int cyclescale(const Arg *arg);
static int savestate(const Arg *arg);

static void scaledown(Image *im);
static void scaleheight(Image *im);
static void scalewidth(Image *im);
static void scalefit(Image *im);

/* handling files */
static void addfile(const char *file);
static void getsize(const char *file);
static void readstdin(void);

/* variables */
static Atom atom[WMLast];
static char right[128], left[128];
static unsigned int filecnt = 0;
static size_t fileidx;
static Image *ci, *images; /* current image and images */
static const ScaleMode *scale;
static Drw *drw;
static Clr **scheme;
static int running = 1;
static int bh = 0;      /* bar geometry */
//static int by;		/* bar y */
static int lrpad;       /* sum of left and right padding for text */
static char *wmname = "mage";
static unsigned int numlockmask = 0; //should this be handled at all? (updatenumlockmask)
static Display *dpy;
static Colormap cmap;
static Window win;
static Visual *visual = NULL;
static int depth, screen;
static int scrw, scrh; /* X display screen geometry width, height */
static int winw, winh;
//static int winx, winy;
static Pixmap pm;
static GC gc;

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
#include "image.c"	//image (and imlib2) operations
#include "cmd.c"	//config.h commands

void
cleanup(void)
{
	unsigned int i;

	im_destroy();
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	drw_free(drw);
 	XFreeGC(dpy, gc);
	XDestroyWindow(dpy, win);
	XSync(dpy, False);
	XCloseDisplay(dpy);
}

void
drawbar(void)
{
	int y, tw = 0;

	if (!showbar) //not drawbar
		return;

	/* currently topbar is not supported */
	//y = topbar ? 0 : xw.h - bh;
	y = winh - bh;

	drw_setscheme(drw, scheme[SchemeBar]);

	/* left text */
	snprintf(left, LENGTH(left), "%s %dx%d", ci->fname, ci->w, ci->h);
	drw_text(drw, 0, y, winw / 2, bh, lrpad / 2, left, 0);

	/* right text */
	snprintf(right, LENGTH(right), "%s <%d%%> [%d/%d]", ci->zoomed ? "" : scale->name, (int)(ci->z * 100.0), (int) fileidx + 1, filecnt);
	tw = TEXTW(right) - lrpad + 2; /* 2px right padding */
	drw_text(drw, winw / 2, y, winw / 2, bh, winw / 2 - (tw + lrpad / 2), right, 0);

	drw_map(drw, win, 0, y, winw, bh);
}

void
run(void)
{
	XEvent ev;

	while (running) {
		XNextEvent(dpy, &ev);
		if (handler[ev.type])
			(handler[ev.type])(&ev);
		//if (image.redraw)
		//	img_render();
	}
}

void
xhints(void)
{
	XClassHint class = {.res_name = wmname, .res_class = "mage"};
	XWMHints wm = {.flags = InputHint, .input = True};
	XSizeHints *sizeh = NULL;

	if (!(sizeh = XAllocSizeHints()))
		die("mage: Unable to allocate size hints");

	sizeh->flags = PSize;
	sizeh->height = winh;
	sizeh->width = winw;

	XSetWMProperties(dpy, win, NULL, NULL, NULL, 0, sizeh, &wm, &class);
	XFree(sizeh);
}

void
bpress(XEvent *e)
{
	unsigned int i;

	for (i = 0; i < LENGTH(mshortcuts); i++)
		if (e->xbutton.button == mshortcuts[i].b && mshortcuts[i].func)
			if (mshortcuts[i].func(&(mshortcuts[i].arg))) {
				img_render(ci);
				drawbar();
			}
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
	if (e->xexpose.count == 0) {
		img_render(ci);
		drawbar();
	}
}
void
kpress(XEvent *e)
{
	const XKeyEvent *ev = &e->xkey;
	unsigned int i;
	KeySym keysym;

	keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, 0);
	for (i = 0; i < LENGTH(shortcuts); i++)
		if (keysym == shortcuts[i].keysym
		&& CLEANMASK(shortcuts[i].mod) == CLEANMASK(ev->state)
		&& shortcuts[i].func)
			if (shortcuts[i].func(&(shortcuts[i].arg))) { /* if the func returns something, reload */
				img_render(ci);
				drawbar();
			}
}

void
configurenotify(XEvent *e)
{
	XConfigureEvent *ev = &e->xconfigure;

	if (winw != ev->width || winh + bh!= ev->height) {
		winw = ev->width;
		winh = ev->height;
		drw_resize(drw, winw, winh + bh);
		//if (showbar)
		//	xw.h -= bh;
		//else
		//	xw.h += bh;
	}
}

void
getsize(const char *file)
{
	DIR *dir;
	struct dirent *e;
	struct stat s;

	if (!(stat(file, &s))) {
		if (S_ISDIR(s.st_mode)) { /* it's a dir */
		if ((dir = opendir(file))) {
			while ((e = readdir(dir))) {
				if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
					continue;
				if (e->d_type == DT_REG)
					filecnt++;
			}
			closedir(dir);
		} else if (!quiet)
			fprintf(stderr, "mage: Directory '%s', cannot be opened.\n", file);
	} else if (S_ISREG(s.st_mode)) {
		if (imlib_load_image(file)) {
			filecnt++;
		} else {
			if (!quiet)
				fprintf(stderr, "mage: File '%s' cannot be opened.\n", file);
			return;
		}
	} else
		fprintf(stderr, "mage: '%s' No such file or directory.\n", file);
	}
}

void
addfile(const char *file)
{
	if ((imlib_load_image(file))) { /* can be opened */
		images[fileidx].fname = file;
		//images[fileidx].im = m;
		//imlib_context_set_image(m);
		//images[fileidx].w = imlib_image_get_width();
		//images[fileidx].h = imlib_image_get_height();
		images[fileidx].checkpan = 0;
		images[fileidx].zoomed = 0;
		fileidx++;
	} else {
		if (!quiet)
			fprintf(stderr, "mage: File '%s' cannot be opened.\n", file);
		return;
	}
}

void
readstdin(void)
{
	size_t n;
	ssize_t len;
	char *line = NULL;
	size_t i = filecnt;
	int sizecnt = 2;

	while ((len = getline(&line, &n, stdin)) > 0) {
		if (line[len-1] == '\n')
			line[len-1] = '\0';
		addfile(line);
		line = NULL;
		if (filecnt >= i) {
			images = realloc(images, sizeof(Image *) * sizecnt * i);
			sizecnt++;
		}
	}
}

void
setup(void)
{
	int i;
	XTextProperty prop;
	XSetWindowAttributes attrs;
	XGCValues gcval;

	if (!(dpy = XOpenDisplay(NULL)))
		die("mage: Unable to open display");

	if (!scale)
		scale = &scalemodes[0];
	ci->z = 1.0;

	/* init screen */
	screen = DefaultScreen(dpy);
	scrw = DisplayWidth(dpy, screen);
	scrh = DisplayHeight(dpy, screen);
	visual = DefaultVisual(dpy, screen);
	cmap = DefaultColormap(dpy, screen);
	depth = DefaultDepth(dpy, screen);
	pm = 0;

	winw = winwidth;
	winh = winheight;

	//xw.attrs.background_pixel = 0;
	//xw.attrs.border_pixel = 0;
 	attrs.colormap = cmap;
	attrs.save_under = False;
	attrs.bit_gravity = CenterGravity;
	attrs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask |
	                       ButtonPressMask; //ButtonMotionMask

	win = XCreateWindow(dpy, XRootWindow(dpy, screen), 0, 0, winw, winh, 0,
			depth, InputOutput, visual, 0, &attrs);

	XSelectInput(dpy, win, StructureNotifyMask | ExposureMask | KeyPressMask |
	                       ButtonPressMask);

	/* init atoms */
	atom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	atom[WMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	atom[WMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	atom[WMState] = XInternAtom(dpy, "_NET_WM_STATE", False);

	XSetWMProtocols(dpy, win, &atom[WMDelete], 1);

	if (!(drw = drw_create(dpy, screen, win, winw, winh)))
		die("mage: Unable to create drawing context");

	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 2);
	XSetWindowBackground(dpy, win, scheme[SchemeNorm][ColBg].pixel);
	gcval.foreground = scheme[SchemeNorm][ColBg].pixel;
	gc = XCreateGC(dpy, win, GCForeground, &gcval); //context for Pixmap
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2; /* two pixel padding */

	/* init bar */
	drawbar();

	XStringListToTextProperty(&argv0, 1, &prop);
	XSetWMName(dpy, win, &prop);
	XSetTextProperty(dpy, win, &prop, atom[WMName]);
	XFree(prop.value);
	XMapWindow(dpy, win);
	drw_resize(drw, winw, winh);
	xhints();
	XSync(dpy, False);

	/* init image */
	imlib_context_set_display(dpy);
	imlib_context_set_visual(visual);
	imlib_context_set_colormap(cmap);
	//imlib_context_set_drawable(xw.pm);
	//imlib_context_set_drawable(xw.win);

	img_render(ci);
}

void
usage(void)
{
	die("usage: %s [-fhpqrv] [-s scalemode] [-n class] file...", argv0);
}

int
main(int argc, char *argv[])
{
	int i, fs = 0;
	char *mode;
	void (*scalefunc)(Image *im);

	if (!argv[0])
		usage();

	ARGBEGIN {
	case 'f':
		fs = 1;
		break;
	case 'h':
		usage();
		break;
	case 'p':
		antialiasing = 0;
		break;
	case 'q':
		quiet = 1;
		break;
	case 'r':
		recursive = 1;
		break;
	case 'n':
		wmname = EARGF(usage());
		break;
	case 's':
		mode = EARGF(usage());
		if (!strcmp(mode, "down")) {
			scalefunc = scaledown;
		} else if (!strcmp(mode, "width")) {
			scalefunc = scalewidth;
		} else if (!strcmp(mode, "height")) {
			scalefunc = scaleheight;
		} else if (!strcmp(mode, "fit")) {
			scalefunc = scalefit;
		} else
			break;
		for (ScaleMode *l = (ScaleMode *)scalemodes; l; l++)
			if (l->arrange == scalefunc) {
				scale = l;
				break;
			}
		break;
	case 'v':
		die("mage-"VERSION);
		break;
	default:
		usage();
	} ARGEND

	DIR *dir;

	struct dirent *e;

	if (recursive) {
		for (i = 0; i < argc; i++)
			getsize(argv[i]); /* count images in dir */
		images = ecalloc(filecnt, sizeof(Image));
		for (i = 0; i < argc; i++) {
			if ((dir = opendir(argv[i]))) {
				while ((e = readdir(dir))) {
					if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
						continue;
					if (e->d_type == DT_REG) {
						int len = strlen(argv[i]) + 1 + strlen(e->d_name) + 1;
						char *f = ecalloc(len, sizeof(char));
						snprintf(f, len, "%s/%s", argv[i], e->d_name);
						addfile(f);
					}
				}
			} else {
				addfile(argv[i]);
			}
			closedir(dir);
		}
	} else if (!strcmp(argv[0], "-"))
		filecnt = 1024; /* big arbitrary size */
	else
		filecnt = argc;

	if (!recursive)
		images = ecalloc(filecnt, sizeof(Image));

	if (!strcmp(argv[0], "-"))
		readstdin();
	else if (!recursive) /* handle images */
		for (i = 0; i < argc; i++)
			addfile(argv[i]);

	if (!filecnt || !fileidx)
		die("mage: No more images to display");
	fileidx = 0;

	ci = images;

	setup();

	if (fs)
		togglefullscreen(NULL);

	run();

	cleanup();

	return 0;
}
