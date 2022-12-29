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
#define LENGTH(X)               (sizeof(X) / sizeof(X)[0])
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define ABS(X)			((X) > 0 ? (X) : -(X))

enum { SchemeNorm, SchemeBar }; /* color schemes */
enum { WMDelete, WMName, WMFullscreen, WMState, WMLast }; /* atoms */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct ScaleMode ScaleMode;

typedef struct {
 	//int re; /* rendered */
	const char *fname;

	int checkpan;
	int zoomed;

	int w, h;   /* original dimension */
	float x, y; /* position of the pan() */
	float z;    /* zoom */

	Imlib_Image crop; /* small handy version of the image, intented to be use in layouts. */
	int cw, ch;   /* crop dimension */
} Image;

struct ScaleMode {
	const char *name;
	void (*arrange)(Image *im);
};

typedef struct {
	const char *name;
	void (*arrange)(void);
} Layout;

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
static void addfile(const char *file);
static int advance(const Arg *arg);
static void bpress(XEvent *e);
static void check_pan(Image *img);
static void cleanup(void);
static void cmessage(XEvent *e);
static void configurenotify(XEvent *e);
static int cyclescale(const Arg *arg);
static void drawbar(void);
static void expose(XEvent *e);
static int first(const Arg *arg);
static int fliphorz(const Arg *arg);
static int flipvert(const Arg *arg);
static unsigned int getsize(const char *file);
static void img_zoom(Image *img, float z);
static void kpress(XEvent *e);
static int last(const Arg *arg);
static int panhorz(const Arg *arg);
static int panvert(const Arg *arg);
static int printfile(const Arg *arg);
static int quit(const Arg *arg);
static void readstdin(void);
static int reload(const Arg *arg);
static int rotate(const Arg *arg);
static void run(void);
static int savestate(const Arg *arg);
static void scaledown(Image *img);
static void scalefit(Image *img);
static void scaleheight(Image *img);
static void scalewidth(Image *img);
static int set_zoom(const Arg *arg);
static int setlayout(const Arg *arg);
static void setup(void);
static void singleview(void);
static void thumbnailview(void);
static int toggleantialias(const Arg *arg);
static int togglebar(const Arg *arg);
static int togglefullscreen(const Arg *arg);
static void usage(void);
static void xhints(void);
static int zoom(const Arg *arg);
static int loadimgs(Image *i);

/* variables */
static Atom atom[WMLast];
static char right[128], left[128]; /* status bar */
static unsigned int filecnt = 0;
static int dirty = 0;
static size_t fileidx;
static Image *ci, *images; /* current image and images */
static const ScaleMode *scale; /* scalemode */
static const Layout *lt; /* scalemode */
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
static int winw, winh; /* window width and height */
static int winy; /* window height - bar height */
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
addfile(const char *file)
{
	Imlib_Image *im;

	if ((im = imlib_load_image(file))) { /* can be opened */
		imlib_context_set_image(im);
		imlib_free_image();
		images[fileidx].fname = file;
		images[fileidx].checkpan = 0;
		images[fileidx].zoomed = 0;
		images[fileidx].z = 1.0;
		fileidx++;
	} else if (!quiet)
			fprintf(stderr, "mage: File '%s' cannot be opened.\n", file);
}

void
bpress(XEvent *e)
{
	unsigned int i;

	for (i = 0; i < LENGTH(mshortcuts); i++)
		if (e->xbutton.button == mshortcuts[i].b && mshortcuts[i].func)
			if (mshortcuts[i].func(&(mshortcuts[i].arg))) {
				lt->arrange();
				drawbar();
			}
}

void
cleanup(void)
{
	unsigned int i;

	for (i = 0; i < filecnt; i++) {
		if (images[i].crop) {
			imlib_context_set_image(images[i].crop);
			imlib_free_image();
			images[i].crop = NULL;
		}
	}

	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	free(images);
	drw_free(drw);
 	XFreeGC(dpy, gc);
	XFreePixmap(dpy, pm);
	XDestroyWindow(dpy, win);
	XSync(dpy, False);
	XCloseDisplay(dpy);
}

void
cmessage(XEvent *e)
{
	if (e->xclient.data.l[0] == atom[WMDelete])
		running = 0;
}

void
configurenotify(XEvent *e)
{
	XConfigureEvent *ev = &e->xconfigure;

	if (winw != ev->width || winh != ev->height) {
		winw = ev->width;
		winh = ev->height;
		drw_resize(drw, winw, winh);
	}
}

void
drawbar(void)
{
	int tw = 0;

	winy = winh - bh;

	if (!showbar)
		return;

	/* currently topbar is not supported */
	//y = topbar ? 0 : xw.h - bh;

	drw_setscheme(drw, scheme[SchemeBar]);

	/* left text */
	snprintf(left, LENGTH(left), "%s %dx%d", ci->fname, ci->w, ci->h);
	drw_text(drw, 0, winy, winw / 2, bh, lrpad / 2, left, 0);

	/* right text */
	snprintf(right, LENGTH(right), "%s <%d%%> [%zu/%u]", ci->zoomed ? "" : scale->name, (int)(ci->z * 100.0), fileidx + 1, filecnt);
	tw = TEXTW(right) - lrpad + 2; /* 2px right padding */
	drw_text(drw, winw / 2, winy, winw / 2, bh, winw / 2 - (tw + lrpad / 2), right, 0);

	drw_map(drw, win, 0, winy, winw, bh);
}

void
expose(XEvent *e)
{
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0) {
		ci->zoomed = 1;
		lt->arrange();
		drawbar();
	}
}

unsigned int
getsize(const char *file)
{
	DIR *dir;
	struct dirent *e;
	struct stat s;
	unsigned int cnt = 0;

	if (!(stat(file, &s))) {
		if (S_ISDIR(s.st_mode)) { /* it's a dir */
		if ((dir = opendir(file))) {
			while ((e = readdir(dir))) {
				if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
					continue;
				if (e->d_type == DT_REG)
					cnt++;
			}
			closedir(dir);
		} else if (!quiet)
			fprintf(stderr, "mage: Directory '%s', cannot be opened.\n", file);
	} else if (S_ISREG(s.st_mode)) {
		if (imlib_load_image(file)) {
			cnt++;
		} else {
			if (!quiet)
				fprintf(stderr, "mage: File '%s' cannot be opened.\n", file);
			return 0;
		}
	} else
		fprintf(stderr, "mage: '%s' No such file or directory.\n", file);
	}

	return cnt;
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
				lt->arrange();
				drawbar();
			}
}

void
readstdin(void)
{
	size_t n;
	ssize_t len;
	char *line = NULL;
	size_t oldi = filecnt; /* big arbitrary size */
	int sizecnt = 2;

	/* read stdin */
	while ((len = getline(&line, &n, stdin)) > 0) {
		if (line[len-1] == '\n')
			line[len-1] = '\0';
		addfile(line);
		line = NULL;
		if (fileidx >= oldi) {
			/* need more space, realloc */
			if (!(images = realloc(images, sizeof(Image) * sizecnt * oldi)))
				die("cannot realloc %u bytes:", sizeof(Image) * sizecnt * oldi);
			sizecnt++;
		}
	}

	/* realloc exactly the number of files that can be opened */
	filecnt = fileidx;
	if (!(images = realloc(images, sizeof(Image) * fileidx)))
		die("cannot realloc %u bytes:", sizeof(Image) * fileidx);
}

void
run(void)
{
	XEvent ev;

	while (running) {
		if (!XNextEvent(dpy, &ev) && handler[ev.type])
			handler[ev.type](&ev); /* call handler */
		if (dirty == 1) {
			lt->arrange();
			drawbar();
			dirty = 0;
		}
	}
}

void
setup(void)
{
	unsigned int i;
	XTextProperty prop;
	XSetWindowAttributes wa;
	XGCValues gcval;

	if (!(dpy = XOpenDisplay(NULL)))
		die("mage: Unable to open display");

	/* init image */
	fileidx = 0;
	ci = images;
	if (!scale)
		scale = &scalemodes[0];
	if (!lt)
		lt = &layouts[0];

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

	//wa.background_pixel = 0;
	//wa.border_pixel = 0;
 	wa.colormap = cmap;
	wa.save_under = False;
	wa.bit_gravity = CenterGravity;
	wa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask |
			ButtonPressMask; //ButtonMotionMask

	win = XCreateWindow(dpy, XRootWindow(dpy, screen), 0, 0, winw, winh, 0,
			depth, InputOutput, visual, 0, &wa);

	XSelectInput(dpy, win, wa.event_mask);

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
	XMapRaised(dpy, win);
	drw_resize(drw, winw, winh);
	xhints();
	XSync(dpy, False);
	/* init imlib */
	imlib_context_set_display(dpy);
	imlib_context_set_visual(visual);
	imlib_context_set_colormap(cmap);
	//imlib_context_set_drawable(xw.pm);
	//imlib_context_set_drawable(xw.win);
}

void
usage(void)
{
	die("usage: %s [-fhpqrv] [-s scalemode] [-n class] file...", argv0);
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

int
main(int argc, char *argv[])
{
	DIR *dir;
	struct dirent *e;
	int i, fs = 0;
	char *mode;
	void (*scalefunc)(Image *im);

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
		loaddirs = 1;
		break;
	case 'n':
		wmname = EARGF(usage());
		break;
	case 's':
		mode = EARGF(usage());
		if (!strcmp(mode, "down"))
			scalefunc = scaledown;
		else if (!strcmp(mode, "width"))
		      scalefunc = scalewidth;
		else if (!strcmp(mode, "height"))
		      scalefunc = scaleheight;
		else if (!strcmp(mode, "fit"))
		      scalefunc = scalefit;
		else
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
		break;
	} ARGEND

	if (!argv[0])
		usage();

	if (loaddirs) {
		for (i = 0; i < argc; i++)
			filecnt += getsize(argv[i]); /* count images in dir */
	} else if (!strcmp(argv[0], "-"))
		filecnt = 1024; /* big arbitrary size */
	else
		filecnt = argc;

	images = ecalloc(filecnt, sizeof(Image));

	if (loaddirs) {
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
				closedir(dir);
			} else
				addfile(argv[i]);
		}
	} else if (!strcmp(argv[0], "-"))
		readstdin();
	else	/* handle images */
		for (i = 0; i < argc; i++)
			addfile(argv[i]);
	filecnt = fileidx;
	if (!filecnt)
		die("mage: No more images to display");
	setup();
	if (fs)
		togglefullscreen(NULL);
	run();
	cleanup();
	return EXIT_SUCCESS;
}
