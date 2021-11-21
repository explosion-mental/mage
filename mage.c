/* See LICENSE file for copyright and license details. */
#include <sys/types.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <Imlib2.h>

#include "arg.h"
#include "util.h"
#include "drw.h"

char *argv0;

/* macros */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(a)               (sizeof(a) / sizeof(a)[0])
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define ABS(a)			((a) > 0 ? (a) : -(a))

enum { SchemeNorm, SchemeSel, SchemeBar }; /* color schemes */
enum { WMDelete, WMName, WMFullscreen, WMState, WMLast }; /* atoms */

typedef enum {
	SCALE_DOWN,
	SCALE_FIT,
	SCALE_WIDTH,
	SCALE_HEIGHT,
} scaling;

typedef struct {
	Display *dpy;
	Colormap cmap;
	Window win;
	Visual *vis;
	XSetWindowAttributes attrs;
	int depth; /* bit depth */
	int scr;
	int scrw, scrh;
	//int x, y;
	int w, h;
	Pixmap pm;
	GC gc;
	//int gm; /* geometry mask */
} XWindow;

typedef struct {
	//Imlib_Image *im;
 	//int re; /* rendered */
	//int redraw;
	int checkpan;
	int zoomed;
	int w, h;   /* dimension */
	float x, y; /* position */
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

/* X events */
static void bpress(XEvent *);
static void cmessage(XEvent *);
static void expose(XEvent *);
static void kpress(XEvent *);
static void configurenotify(XEvent *);

/* image */
static void im_destroy(void);
static int im_load(const char *filename);
static int img_load(Image *img, const char *filename);
static void img_render(Image *img);
static void img_zoom(Image *img, float z);
static void check_pan(Image *img);

/* commands */
static void togglebar(const Arg *arg);
static void quit(const Arg *arg);
static void flipvert(const Arg *arg);
static void fliphorz(const Arg *arg);
static void advance(const Arg *arg);
static void printfile(const Arg *arg);
static void zoom(const Arg *arg);
static void set_zoom(const Arg *arg);
static void togglefullscreen(const Arg *arg);
static void panhorz(const Arg *arg);
static void panvert(const Arg *arg);
static void first(const Arg *arg);
static void last(const Arg *arg);
static void rotate(const Arg *arg);
static void toggleantialias(const Arg *arg);
static void reload(const Arg *arg);
static void cyclescale(const Arg *arg);
static void savestate(const Arg *arg);

/* handling files */
static int check_img(char *filename);
static void check_file(char *file);
static void readstdin(void);

/* variables */
static Atom atom[WMLast];
static char right[128], left[128];
static char **filenames;
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
static const char *scales[] = { "down", "fit", "width", "height" };
static float zoomstate = 1.0;
static unsigned int numlockmask = 0; //should this be handled at all? (updatenumlockmask)

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
	free(filenames);
	free(scheme);
	drw_free(drw);
 	XFreeGC(xw.dpy, xw.gc);
	XDestroyWindow(xw.dpy, xw.win);
	XSync(xw.dpy, False);
	XCloseDisplay(xw.dpy);
}

void
drawbar(void)
{
	int y, tw = 0;

	if (!showbar) //not drawbar
		return;

	/* currently topbar is not supported */
	//y = topbar ? 0 : xw.h - bh;
	y = xw.h - bh;

	drw_setscheme(drw, scheme[SchemeBar]);

	/* left text */
	snprintf(left, LENGTH(left), "%s %dx%d", filenames[fileidx], image.w, image.h);
	drw_text(drw, 0, y, xw.w/2, bh, lrpad / 2, left, 0);

	/* right text */
	snprintf(right, LENGTH(right), "%s <%d%%> [%d/%d]", image.zoomed ? "" : scales[scalemode], (int)(zoomstate * 100.0), fileidx + 1, filecnt);
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
		//TODO currently resize is a bit sluggish (noticeable on big images)
		//image.redraw = 0;
		img_render(&image);
		//XSync(xw.dpy, True);
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

	if (xw.w != ev->width || xw.h + bh!= ev->height) {
		xw.w = ev->width;
		xw.h = ev->height;
		drw_resize(drw, xw.w, xw.h + bh);
		//if (showbar)
		//	xw.h -= bh;
		//else
		//	xw.h += bh;
	}
}

//ATL: Alternative take, return filename or NULL handle whether it's a
//directory or not
int
check_img(char *file)
{
	if (access(file, F_OK) != -1) {
		//the file exist
		if (im_load(file) == 0) {
			//the file is an image
			imlib_free_image();
			if (!(filenames = realloc(filenames, (filecnt + 1) * sizeof (const char *))))
				die("cannot realloc %u bytes:", (filecnt + 1) * sizeof (const char *));
			filenames[filecnt] = file;
			filecnt++;
			return 0;
		} else //return 1 if the image cant be loaded (it may be a directory)
			return 1;
	} else { //returns 2 if the file doesn't exist
		if (!quiet)
			fprintf(stderr, "mage: %s: No such file or directory\n", file);
		return 2;
	}
	//return 0;
}

//needs to be simplified further
void
check_file(char *file)
{
	char *filename;
	char **dirnames;
	DIR *dir;
	int dircnt, diridx;
	unsigned int first;
	size_t len;
	struct dirent *dentry;
	int ret;

	/* check if it's an image */
	if (!check_img(file))
		return;

	dircnt = 512;
	diridx = first = 1;
	if (!(dirnames = malloc(dircnt * sizeof (const char *))))
		die("cannot malloc %u bytes:", dircnt * sizeof (const char *));
	dirnames[0] = file;

	/* check if it's a directory */
	//do we need to check this? if it isn't a file then what else could it be?
	if ((dir = opendir(file))) {
		/* handle directory */
		while (diridx > 0) {
			file = dirnames[--diridx];
			while ((dentry = readdir(dir))) {
				/* ignore directories '.' and '..' */
				if (!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, ".."))
					continue;
				len = strlen(file) + strlen(dentry->d_name) + 2;
				if (!(filename = malloc(len * sizeof(char))))
					die("cannot malloc %u bytes:", len * sizeof (char));
				snprintf(filename, len, "%s/%s", file, dentry->d_name);
				if (recursive)
					check_file(filename);
				else {
					ret = check_img(filename);
					if (ret == 1) {
						if (!quiet)
							fprintf(stderr, "mage: %s: Ignoring directory\n", filename);
						free(filename);
					} else if (ret == 2)
						free(filename);
				}
			}
			closedir(dir);
			if (!first)
				free(file);
			else
				first = 0;
		}
		free(dirnames);
		return;
	} else	/* opendir() failed */
		return;
}

static void
readstdin(void)
{
	size_t n;
	ssize_t len;
	char *tmp = NULL, *line;

	while ((len = getline(&tmp, &n, stdin)) > 0) {
		if (tmp[len-1] == '\n')
			tmp[len-1] = '\0';
		line = estrdup(tmp);
		check_file(line);
	}
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
	xw.scr   = DefaultScreen(xw.dpy);
	xw.scrw  = DisplayWidth(xw.dpy, xw.scr);
	xw.scrh  = DisplayHeight(xw.dpy, xw.scr);
	xw.vis   = DefaultVisual(xw.dpy, xw.scr);
	xw.cmap  = DefaultColormap(xw.dpy, xw.scr);
	xw.depth = DefaultDepth(xw.dpy, xw.scr);
	xw.pm = 0;

	if (!xw.w)
		xw.w = xw.scrw;
	if (!xw.h)
		xw.h = xw.scrh;

 	xw.attrs.colormap = xw.cmap;
	//xw.attrs.background_pixel = 0;
	//xw.attrs.border_pixel = 0;
	xw.attrs.save_under = False;
	xw.attrs.bit_gravity = CenterGravity;
	xw.attrs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask |
	                       ButtonPressMask; //ButtonMotionMask

	xw.win = XCreateWindow(xw.dpy, XRootWindow(xw.dpy, xw.scr), 0, 0,
				xw.w, xw.h, 0, xw.depth, InputOutput, xw.vis,
				0, &xw.attrs);

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
		scheme[i] = drw_scm_create(drw, colors[i], 2);
	XSetWindowBackground(xw.dpy, xw.win, scheme[SchemeNorm][ColBg].pixel);
	gcval.foreground = scheme[SchemeNorm][ColBg].pixel;
	xw.gc = XCreateGC(xw.dpy, xw.win, GCForeground, &gcval); //context for Pixmap
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;

	/* init bar */
	drawbar();

	XStringListToTextProperty(&argv0, 1, &prop);
	XSetWMName(xw.dpy, xw.win, &prop);
	XSetTextProperty(xw.dpy, xw.win, &prop, atom[WMName]);
	XFree(prop.value);
	XMapWindow(xw.dpy, xw.win);
	drw_resize(drw, xw.w, xw.h);
	xhints();
	XSync(xw.dpy, False);

	/* init image */
	imlib_context_set_display(xw.dpy);
	imlib_context_set_visual(xw.vis);
	imlib_context_set_colormap(xw.cmap);
	//imlib_context_set_drawable(xw.pm);
	//imlib_context_set_drawable(xw.win);
	img_load(&image, filenames[fileidx]);
	img_render(&image);
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
		for (i = 0; i < LENGTH(scales); i++)
			if (!strcmp(mode, scales[i])) {
				scalemode = i;
				break;
			}
		break;
	case 'v':
		die("mage-"VERSION);
		break;
	default:
		usage();
	} ARGEND

	if (!argv[0])
		usage();

	if (!strcmp(argv[0], "-"))
		readstdin();
	else /* handle only images or directories */
		for (i = 0; i < argc; i++)
			check_file(argv[i]);

	if (!filecnt)
		//exit(1);
		die("mage: No more images to display");

	setup();

	if (fs) togglefullscreen(0);

	run();

	cleanup();

	return 0;
}
