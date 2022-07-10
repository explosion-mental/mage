/* return 1 = re-draw bar and re-render image
 * return 0 = do nothing */

int
togglebar(const Arg *arg)
{
	showbar = !showbar;
	if (showbar)
		xw.h -= bh;
	else
		xw.h += bh;
	XSync(xw.dpy, False);
	return 1;
}

int
advance(const Arg *arg)
{
	if ((arg->i > 0 && fileidx + arg->i < filecnt)
	|| (arg->i < 0 && fileidx >= -arg->i)) {
		images[fileidx].im = NULL;
		ci->im = NULL;
		imlib_free_image();
		fileidx = fileidx + arg->i;
		ci = ci + arg->i;
		ci = &images[fileidx];
		return 1;
	}

	return 0;
}

int
printfile(const Arg *arg)
{
	printf("%s\n", ci->fname);
	quit(NULL);
	return 0;
}

int
set_zoom(const Arg *arg)
{
	img_zoom(ci, arg->f / 100.0);
	return 1;
}

int
zoom(const Arg *arg)
{
	img_zoom(ci, ci->z + arg->f / 100.0);
	return 1;
}

int
togglefullscreen(const Arg *arg)
{
	XEvent e;

	e.type = ClientMessage;
	e.xclient.window = xw.win;
	e.xclient.message_type = atom[WMState];
	e.xclient.format = 32;
	e.xclient.data.l[0] = 2;
	e.xclient.data.l[1] = atom[WMFullscreen];
	e.xclient.data.l[2] = 0;
	XSendEvent(xw.dpy, DefaultRootWindow(xw.dpy), False,
	           SubstructureNotifyMask | SubstructureRedirectMask, &e);
	return 0;
}

int
panhorz(const Arg *arg)
{
	float x = ci->x, y = ci->y;

	if (arg->i > 0)
		ci->x -= xw.w / arg->f;
	else
		ci->x += xw.w / -arg->f;

	check_pan(ci);

	if (x != ci->x || y != ci->y) {
		return 1;
	}
	return 0;
}

int
panvert(const Arg *arg)
{
	float x = ci->x, y = ci->y;

	if (arg->i > 0)
		ci->y += xw.h / arg->f;
	else
		ci->y -= xw.h / -arg->f;

	check_pan(ci);

	if (x != ci->x || y != ci->y) {
		return 1;
	}
	return 0;
}

int
first(const Arg *arg)
{
	if (fileidx != 0) {
		images[fileidx].im = NULL;
		ci->im = NULL;
		imlib_free_image();
		fileidx = 0;
		ci = ci + arg->i;
		ci = &images[fileidx];
		return 1;
	}
	return 0;
}

int
last(const Arg *arg)
{
	if (fileidx != filecnt - 1) {
		images[fileidx].im = NULL;
		ci->im = NULL;
		imlib_free_image();
		fileidx = filecnt - 1;
		ci = ci + arg->i;
		ci = &images[fileidx];
		return 1;
	}
	return 0;
}

int
rotate(const Arg *arg)
{
	int ox, oy, tmp, d;

	if (arg->i > 0)
		d = 1;
	else
		d = 3;

	ox = d == 1 ? ci->x : xw.w - ci->x - ci->w * ci->z;
	oy = d == 3 ? ci->y : xw.h - ci->y - ci->h * ci->z;

	imlib_image_orientate(d);

	ci->x = oy + (xw.w - xw.h) / 2;
	ci->y = ox + (xw.h - xw.w) / 2;

	tmp = ci->w;
	ci->w = ci->h;
	ci->h = tmp;

	ci->checkpan = 1;
	return 1;
}

int
toggleantialias(const Arg *arg)
{
	antialiasing = !antialiasing;
	imlib_context_set_anti_alias(antialiasing);
	return 1;
}

int
reload(const Arg *arg)
{
	images[fileidx].im = NULL;
	ci->im = NULL;
	imlib_free_image();

	return 1;
}

int
cyclescale(const Arg *arg)
{
	ScaleMode *l;
	unsigned int idx = 0;
	for (l = (ScaleMode *)scalemodes; l != scale; l++, idx++);

	if (arg->i > 0) {
		if (!ci->zoomed) { /* use the same scalemode as before if the image is zoomed */
			if (idx < LENGTH(scalemodes) - 1)
				idx = idx + arg->i;
			else
				idx = 0;
		}
		ci->zoomed = 0;
	} else {
		if (!ci->zoomed) {
			if (idx > 0)
				idx = idx + arg->i;
			else
				idx = LENGTH(scalemodes) - 1;
		}
		ci->zoomed = 0;
	}

	scale = &scalemodes[idx];

	return 1;
}

int
savestate(const Arg *arg)
{
	imlib_save_image(ci->fname);
	return 0;
}

int
fliphorz(const Arg *arg)
{
	imlib_image_flip_horizontal();
	return 1;
}

int
flipvert(const Arg *arg)
{
	imlib_image_flip_vertical();
	return 1;
}

int
quit(const Arg *arg)
{
	running = 0;
	return 0;
}
