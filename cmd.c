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
		fileidx = fileidx + arg->i;
		img_load(&image, filenames[fileidx]);
		return 1;
	}
	return 0;
}

int
printfile(const Arg *arg)
{
	printf("%s\n", filenames[fileidx]);
	quit(NULL);
	return 0;
}

int
set_zoom(const Arg *arg)
{
	img_zoom(&image, arg->f / 100.0);
	return 1;
}

int
zoom(const Arg *arg)
{
	img_zoom(&image, zoomstate + arg->f / 100.0);
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
	float x = image.x, y = image.y;

	if (arg->i > 0)
		image.x -= xw.w / arg->f;
	else
		image.x += xw.w / -arg->f;

	check_pan(&image);

	if (x != image.x || y != image.y) {
		return 1;
	}
	return 0;
}

int
panvert(const Arg *arg)
{
	float x = image.x, y = image.y;

	if (arg->i > 0)
		image.y += xw.h / arg->f;
	else
		image.y -= xw.h / -arg->f;

	check_pan(&image);

	if (x != image.x || y != image.y) {
		return 1;
	}
	return 0;
}

int
first(const Arg *arg)
{
	if (fileidx != 0) {
		fileidx = 0;
		img_load(&image, filenames[fileidx]);
		return 1;
	}
	return 0;
}

int
last(const Arg *arg)
{
	if (fileidx != filecnt - 1) {
		fileidx = filecnt - 1;
		img_load(&image, filenames[fileidx]);
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

	ox = d == 1 ? image.x : xw.w - image.x - image.w * zoomstate;
	oy = d == 3 ? image.y : xw.h - image.y - image.h * zoomstate;

	imlib_image_orientate(d);

	image.x = oy + (xw.w - xw.h) / 2;
	image.y = ox + (xw.h - xw.w) / 2;

	tmp = image.w;
	image.w = image.h;
	image.h = tmp;

	image.checkpan = 1;
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
	img_load(&image, filenames[fileidx]);
	return 1;
}

int
cyclescale(const Arg *arg)
{
	if (arg->i > 0) {
		if (!image.zoomed) { /* use the same scalemode as before if the image is zoomed */
			if (scalemode < LENGTH(scales) - 1)
				scalemode = scalemode + arg->i;
			else
				scalemode = 0;
		}
		image.zoomed = 0;
	} else {
		if (!image.zoomed) {
			if (scalemode > 0)
				scalemode = scalemode + arg->i;
			else
				scalemode = LENGTH(scales) - 1;
		}
		image.zoomed = 0;
	}
	return 1;
}

int
savestate(const Arg *arg)
{
	imlib_save_image(filenames[fileidx]);
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
