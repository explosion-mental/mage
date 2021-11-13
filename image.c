void
im_init(void)
{
	zoomstate = 1.0;
	image.aa = antialiasing;

	imlib_context_set_display(xw.dpy);
	imlib_context_set_visual(xw.vis);
	imlib_context_set_colormap(xw.cmap);
	//imlib_context_set_drawable(xw.pm);
	//imlib_context_set_drawable(xw.win);
}

void
im_destroy()
{
	if (imlib_context_get_image())
		imlib_free_image();
}

int
im_load(const char *filename)
{
	Imlib_Image *im;

	if (!filename)
		return -1;

	if (!(im = imlib_load_image(filename)))
		return -1;

	imlib_context_set_image(im);
	imlib_image_set_changes_on_disk();

	return 0;
}


int
img_load(Image *img, const char *filename)
{
	if (!img || !filename)
		return -1;

	im_destroy();

	if (im_load(filename) != 0)
		return -1;


	/* sets defaults when opening image */
	img->aa = 1;
	img->checkpan = 0;
 	img->re = 0;
	img->zoomed = 0;
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	imlib_context_set_anti_alias(img->aa);

	return 0;
}

void
img_render(Image *img)
{
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;
	float zw, zh;

	if (!img || !imlib_context_get_image())
		return;

	if (img->zoomed == 0) {
		zw = (float) xw.w / (float) image.w;
		zh = (float) xw.h / (float) image.h;

		zoomstate = MIN(zw, zh);
		zoomstate = MAX(zoomstate, minzoom / 100.0);
		zoomstate = MIN(zoomstate, maxzoom / 100.0);

		if (scalemode == SCALE_DOWN && zoomstate > 1.0)
			zoomstate = 1.0;
	}

	if (!img->re) {
		/* rendered for the first time */
		img->re = 1;
		Image *img = &image;
		img->x = (xw.w - img->w * zoomstate) / 2;
		img->y = (xw.h - img->h * zoomstate) / 2;
	} else if (img->checkpan) {
		/* only useful after zooming */
		img_check_pan(img);
 		img->checkpan = 0;
	}

 	/* calculate source and destination offsets */
	if (img->x < 0) {
		sx = -img->x / zoomstate;
		sw = xw.w / zoomstate;
		dx = 0;
		dw = xw.w;
	} else {
		sx = 0;
		sw = img->w;
		dx = img->x;
		dw = img->w * zoomstate;
	}
	if (img->y < 0) {
		sy = -img->y / zoomstate;
		sh = xw.h / zoomstate;
		dy = 0;
		dh = xw.h;
	} else {
		sy = 0;
		sh = img->h;
		dy = img->y;
		dh = img->h * zoomstate;
	}

	/* clear and set pixmap */
	if (xw.pm)
		XFreePixmap(xw.dpy, xw.pm);
	xw.pm = XCreatePixmap(xw.dpy, xw.win, xw.scrw, xw.scrh, xw.depth);
	XFillRectangle(xw.dpy, xw.pm, xw.gc, 0, 0, xw.scrw, xw.scrh);

	/* render image */
 	imlib_context_set_drawable(xw.pm);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	/* window background */
	XSetWindowBackgroundPixmap(xw.dpy, xw.win, xw.pm);
	XClearWindow(xw.dpy, xw.win);
}

void
img_check_pan(Image *img)
{
	if (!img)
		return;

	if (img->w * zoomstate > xw.w) {
		if (img->x > 0 && img->x + img->w * zoomstate > xw.w)
			img->x = 0;
		if (img->x < 0 && img->x + img->w * zoomstate < xw.w)
			img->x = xw.w - img->w * zoomstate;
	} else {
		img->x = (xw.w - img->w * zoomstate) / 2;
	}

	if (img->h * zoomstate > xw.h) {
		if (img->y > 0 && img->y + img->h * zoomstate > xw.h)
			img->y = 0;
		if (img->y < 0 && img->y + img->h * zoomstate < xw.h)
			img->y = xw.h - img->h * zoomstate;
	} else {
		img->y = (xw.h - img->h * zoomstate) / 2;
	}
}

int
img_zoom(Image *img, float z)
{
	if (!img)
		return 0;

	z = MAX(z, minzoom / 100.0);
	z = MIN(z, maxzoom / 100.0);
	img->zoomed = 1;

	if (z != zoomstate) {
		img->x -= (img->w * z - img->w * zoomstate) / 2;
		img->y -= (img->h * z - img->h * zoomstate) / 2;
		zoomstate = z;
		img->checkpan = 1;
		return 1;
	} else {
		return 0;
	}
}
