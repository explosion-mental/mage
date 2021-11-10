void
im_init(void)
{
	zoomcnt = LENGTH(zoom_levels);
	zoom_min = zoom_levels[0] / 100.0;
	zoom_max = zoom_levels[zoomcnt - 1] / 100.0;
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

	imlib_context_set_anti_alias(img->aa);

	//Sets these variables for every image, so the value differs between
	//images (if this doesn't existed all images will be treated the same,
	//e.g. if I comment out `img->zommed = 0` then the zoom level would
	//stay constant)
	//Update: instead of zoomed we look if the scalemode is SCALE_ZOOM,
	//probably in the future I would revert this behaviour

 	img->scalemode = scalemode;
 	img->re = 0;
	img->checkpan = 0;
	//img->zoomed = 0;
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	return 0;
}

void
img_render(Image *img)
{
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;

	if (!img || !imlib_context_get_image())
		return;

	//if (!img->zoomed && scalemode != SCALE_ZOOM) {
	//	img_fit(0);
	//	if (scalemode == SCALE_DOWN && zoomstate > 1.0)

	if (img->scalemode != SCALE_ZOOM) {
		img_fit(0);
		if (img->scalemode == SCALE_DOWN && zoomstate > 1.0)
				zoomstate = 1.0;
	}

	if (!img->re) {
		/* rendered for the first time */
		img->re = 1;
		img_center(0);
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

	//XClearWindow(win->dpy, win->win);
	//imlib_context_set_drawable(win->pm);
	//if (img->cp == 0)
	//	im_clear();
	//drw_resize(drw, xw.scrw, xw.scrh);
	//drw_rect(drw, 0, 0, xw.scrw, xw.scrh, 0, 0);
	//XFillRectangle(xw.dpy, drw->drawable, bgc, 0, 0, e->scrw, e->scrh);

	if (xw.pm)
		XFreePixmap(xw.dpy, xw.pm);
	xw.pm = XCreatePixmap(xw.dpy, xw.win, xw.scrw, xw.scrh, xw.depth);
	XFillRectangle(xw.dpy, xw.pm, xw.gc, 0, 0, xw.scrw, xw.scrh);

	//drw_rect(drw, 0, 0, xw.scrw, xw.scrh, 0, 0);
	//XFillRectangle(xw.dpy, xw.pm, , 0, 0, xw.scrw, xw.scrh);

 	imlib_context_set_drawable(xw.pm);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	XSetWindowBackgroundPixmap(xw.dpy, xw.win, xw.pm);
	XClearWindow(xw.dpy, xw.win);

	//XSetWindowBackgroundPixmap(xw.dpy, xw.win, drw->drawable);
	//XClearWindow(xw.dpy, xw.win);
	//XClearWindow(xw.dpy, bh);
	//XClearArea(xw.dpy, xw.win, 0, xw.h, xw.w, bh, True);
	//XClearWindow(win->dpy, win->win);
}


void
im_clear(void)
{
	//drw_resize(drw, xw.scrw, xw.scrh);
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
	z = MAX(z, zoom_min);
	z = MIN(z, zoom_max);
 	img->scalemode = SCALE_ZOOM;

	if (z != zoomstate) {
		img->x -= (img->w * z - img->w * zoomstate) / 2;
		img->y -= (img->h * z - img->h * zoomstate) / 2;
		zoomstate = z;
		img->checkpan = 1;
		//img->zoomed = 1;
		return 1;
	} else {
		return 0;
	}
}

void
img_fit(const Arg *arg)
{
	float zw, zh;

	zw = (float) xw.w / (float) image.w;
	zh = (float) xw.h / (float) image.h;

	zoomstate = MIN(zw, zh);
	zoomstate = MAX(zoomstate, zoom_min);
	zoomstate = MIN(zoomstate, zoom_max);
}

void
img_center(const Arg *arg)
{
	Image *img = &image;
	img->x = (xw.w - img->w * zoomstate) / 2;
	img->y = (xw.h - img->h * zoomstate) / 2;
}
