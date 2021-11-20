void
im_destroy()
{
	if (imlib_context_get_image())
		imlib_free_image();
	//	imlib_free_image_and_decache();
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
	//imlib_image_set_changes_on_disk();

	return 0;
}

int
img_load(Image *img, const char *filename)
{
	if (!img || !filename)
		return -1;

	if (im_load(filename) != 0)
		return -1;

	/* set defaults when opening image */
 	//img->redraw = 0;
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();
	img->checkpan = 0;
	img->zoomed = 0;

	imlib_context_set_anti_alias(antialiasing);

	return 0;
}

void
img_render(Image *img)
{
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;
	float zw, zh, z;

	if (!img || !imlib_context_get_image())
		return;

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
			img->checkpan = 1;
		}
	}

	if (img->checkpan) {
		img_check_pan(img);
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
