void
im_destroy()
{
	if (imlib_context_get_image())
		imlib_free_image();
	//	imlib_free_image_and_decache();
}

int
img_load(Image *img, const char *filename)
{
	if (!img || !filename)
		return -1;

	img->im = imlib_load_image(filename);

	if (img->im != NULL) { /* handle the actual image */
		imlib_context_set_image(img->im);
		if (imlib_image_get_data_for_reading_only() == NULL) {
			imlib_free_image();
			img->im = NULL;
		}
	}

	/* set defaults when opening image */
 	//img->redraw = 0;
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();
	img->checkpan = 0;
	img->zoomed = 0;

	/* context */
	imlib_context_set_anti_alias(antialiasing);
	imlib_context_set_image(img->im);

	return 0;
}

void
img_render(Image *img)
{
	if (mode) {
		tns_render(thumbs);
		return;
	}

	int sx, sy, sw, sh; //source
	int dx, dy, dw, dh; //destination
	float zw, zh, z;

	if (!img->zoomed) { /* if the image isn't zoomed */
		zw = (float) xw.w / (float) img->w;
		zh = (float) xw.h / (float) img->h;

		switch (scalemode) {
		case SCALE_WIDTH:
			z = zw;
			break;
		case SCALE_HEIGHT:
			z = zh;
			break;
		default:
			z = MIN(zw, zh);
			break;
		}
		z = MIN(z, scalemode == SCALE_DOWN ? 1.0 : maxzoom / 100.0);

		if (ABS(zoomstate - z) > 1.0 / MAX(img->w, img->h)) {
			zoomstate = z;
			img->x = xw.w / 2 - (xw.w / 2 - img->x) * zoomstate;
			img->y = xw.h / 2 - (xw.h / 2 - img->y) * zoomstate;
		}
		img->checkpan = 1;
	}

	if (img->checkpan) {
		check_pan(img);
		img->checkpan = 0;
	}

 	/* calculate source and destination offsets */
	if (img->x <= 0) {
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
	if (img->y <= 0) {
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
	/* context */
	//imlib_context_set_anti_alias(antialiasing);
	//imlib_context_set_image(img->im);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	/* window background */
	XSetWindowBackgroundPixmap(xw.dpy, xw.win, xw.pm);
	XClearWindow(xw.dpy, xw.win);
}

void
check_pan(Image *img)
{
	if (!img)
		return;

	float w = img->w * zoomstate;
	float h = img->h * zoomstate;

	if (w < xw.w)
		img->x = (xw.w - w) / 2;
	else if (img->x > 0)
		img->x = 0;
	else if (img->x + w < xw.w)
		img->x = xw.w - w;
	if (h < xw.h)
		img->y = (xw.h - h) / 2;
	else if (img->y > 0)
		img->y = 0;
	else if (img->y + h < xw.h)
		img->y = xw.h - h;
}

void
img_zoom(Image *img, float z)
{
	if (!img)
		return;

	z = MAX(z, minzoom / 100.0);
	z = MIN(z, maxzoom / 100.0);

	if (z != zoomstate) {
		img->x -= (img->w * z - img->w * zoomstate) / 2;
		img->y -= (img->h * z - img->h * zoomstate) / 2;
		zoomstate = z;
		img->checkpan = 1;
		img->zoomed = 1;
	}
}











static int cnt, cols, rows, tnsfirst, sel;
#define THUMB_SIZE  50


const int thumb_dim = THUMB_SIZE + 10;

void
tns_load(Image *tns, char **filenames)
{
	int i, w, h;
	float z, zw, zh;

	tns = (Image *) malloc(filecnt * sizeof(Image));
	cnt = filecnt;

	for (i = 0; i < filecnt; i++) {
		if (!(tns->im = imlib_load_image(filenames[i])))
			continue;

		imlib_context_set_image(thumbs->im);

		w = imlib_image_get_width();
		h = imlib_image_get_height();
		zw = (float) THUMB_SIZE / (float) w;
		zh = (float) THUMB_SIZE / (float) h;
		z = MIN(zw, zh);

		thumbs->w = z * w;
		thumbs->h = z * h;

		/* clear and set pixmap */
		if (xw.pm)
			XFreePixmap(xw.dpy, xw.pm);
		xw.pm = XCreatePixmap(xw.dpy, xw.win, xw.scrw, xw.scrh, xw.depth);
		XFillRectangle(xw.dpy, xw.pm, xw.gc, 0, 0, xw.scrw, xw.scrh);

		/* render image */
 		imlib_context_set_drawable(xw.pm);

		/* context */
		//imlib_context_set_anti_alias(antialiasing);
		//imlib_context_set_image(img->im);
		//imlib_context_set_drawable(xw.pm);
		imlib_render_image_part_on_drawable_at_size(0, 0, w, h,
		                                            0, 0, tns->w, tns->h);
		imlib_free_image();
		/* window background */
		XSetWindowBackgroundPixmap(xw.dpy, xw.win, xw.pm);
		XClearWindow(xw.dpy, xw.win);
	}
}

void
tns_free(Image *tns)
{
	int i;

	if (!tns)
		return;

	//for (i = 0; i < tnscnt; ++i)
		//win_free_pixmap(win, tns->thumbs[i].pm);

	free(thumbs);
	thumbs = NULL;
}

void
tns_render(Image *tns)
{
	int i, count, x, y;

	cols = xw.w / thumb_dim;
	rows = xw.h / thumb_dim;

	count = cols * rows;
	if (tnsfirst && tnsfirst + count > cnt)
		tnsfirst = MAX(0, cnt - count);
	count = MIN(tnsfirst + count, cnt);

	/* clear and set pixmap */
	if (xw.pm)
		XFreePixmap(xw.dpy, xw.pm);
	xw.pm = XCreatePixmap(xw.dpy, xw.win, xw.scrw, xw.scrh, xw.depth);
	XFillRectangle(xw.dpy, xw.pm, xw.gc, 0, 0, xw.scrw, xw.scrh);

	x = y = 5;
	i = tnsfirst;

	while (i < count) {
		tns[i].x = x + (THUMB_SIZE - tns[i].w) / 2;
		tns[i].y = y + (THUMB_SIZE - tns[i].h) / 2;
		imlib_context_set_image(tns->im);
		imlib_render_image_part_on_drawable_at_size(0, 0, tns->w, tns->h,
		                                            tns->x, tns->y, tns->w, tns->h);
		if (++i % cols == 0) {
			x = 5;
			y += thumb_dim;
		} else {
			x += thumb_dim;
		}
	}

	/* window background */
	XSetWindowBackgroundPixmap(xw.dpy, xw.win, xw.pm);
	XClearWindow(xw.dpy, xw.win);
}

