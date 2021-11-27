void
togglebar(const Arg *arg)
{
	showbar = !showbar;
	if (showbar)
		xw.h -= bh;
	else
		xw.h += bh;
	XSync(xw.dpy, False);
	img_render(&image);
	drawbar();
}

void
advance(const Arg *arg)
{
	if ((arg->i > 0 && fileidx + arg->i < filecnt)
	|| (arg->i < 0 && fileidx >= -arg->i)) {
		fileidx = fileidx + arg->i;
		img_load(&image, filenames[fileidx]);
		img_render(&image);
		drawbar();
	}
}

void
printfile(const Arg *arg)
{
	printf("%s\n", filenames[fileidx]);
	quit(0);
}

void
set_zoom(const Arg *arg)
{
	img_zoom(&image, arg->f / 100.0);
	img_render(&image);
	drawbar();
}

void
zoom(const Arg *arg)
{
	img_zoom(&image, zoomstate + arg->f / 100.0);
	img_render(&image);
	drawbar();
}

void
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
}

void
panhorz(const Arg *arg)
{
	float x = image.x, y = image.y;

	if (arg->i > 0)
		image.x -= xw.w / arg->f;
	else
		image.x += xw.w / -arg->f;

	check_pan(&image);

	if (x != image.x || y != image.y) {
		img_render(&image);
		drawbar();
	}
}

void
panvert(const Arg *arg)
{
	float x = image.x, y = image.y;

	if (arg->i > 0)
		image.y += xw.h / arg->f;
	else
		image.y -= xw.h / -arg->f;

	check_pan(&image);

	if (x != image.x || y != image.y) {
		img_render(&image);
		drawbar(); //panning without bar?
	}
}

void
first(const Arg *arg)
{
	if (fileidx != 0) {
		fileidx = 0;
		img_load(&image, filenames[fileidx]);
		img_render(&image);
		drawbar();
	}
}

void
last(const Arg *arg)
{
	if (fileidx != filecnt - 1) {
		fileidx = filecnt - 1;
		img_load(&image, filenames[fileidx]);
		img_render(&image);
		drawbar();
	}
}

void
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
	img_render(&image);
	drawbar();
}

void
toggleantialias(const Arg *arg)
{
	antialiasing = !antialiasing;
	imlib_context_set_anti_alias(antialiasing);
	img_render(&image);
	drawbar();
}

void
reload(const Arg *arg)
{
	img_load(&image, filenames[fileidx]);
	img_render(&image);
	drawbar();
}

void
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
		img_render(&image);
		drawbar();
	} else {
		if (!image.zoomed) {
			if (scalemode > 0)
				scalemode = scalemode + arg->i;
			else
				scalemode = LENGTH(scales) - 1;
		}
		image.zoomed = 0;
		img_render(&image);
		drawbar();
	}
}

void
savestate(const Arg *arg)
{
	imlib_save_image(filenames[fileidx]);
}

void
fliphorz(const Arg *arg)
{
	imlib_image_flip_horizontal();
	img_render(&image);
	drawbar();
}

void
flipvert(const Arg *arg)
{
	imlib_image_flip_vertical();
	img_render(&image);
	drawbar();
}
void
toggleblend(const Arg *arg)
{
	blend = !blend;
	imlib_context_set_blend(blend);
	img_render(&image);
	drawbar();
}

void
changemode(const Arg *arg)
{
	mode = !mode;
	img_render(&image);
	drawbar();
}
