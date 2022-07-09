void
im_destroy()
{
//after the 'cache'ing images this function should accept `Image *` as an
//argument in order to reuse this
	if (image.im) {
		imlib_free_image();
	//	imlib_free_image_and_decache();
		image.im = NULL;
	}
}

int
img_load(Image *img, const char *filename)
{
	if (!img || !filename)
		return -1;

	/* handle image */
	img->im = imlib_load_image(filename);
	imlib_context_set_image(img->im);
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	/* set defaults */
 	//img->redraw = 0;
	img->checkpan = 0;
	img->zoomed = 0;

	return 0;
}

void
scalewidth(Image *im)
{
	im->z = MIN((float) xw.w / (float) im->w, maxzoom / 100.0);
}

void
scaleheight(Image *im)
{
	im->z = MIN((float) xw.h / (float) im->h, maxzoom / 100.0);
}

void
scaledown(Image *im)
{
	im->z = MIN((float) xw.w / (float) im->w, (float) xw.h / (float) im->h);
	im->z = MIN(im->z, 1.0);
}

void
scalefit(Image *im)
{
	im->z = MIN((float) xw.w / (float) im->w, (float) xw.h / (float) im->h);
}

void
img_render(Image *img)
{
	int sx, sy, sw, sh; //source
	int dx, dy, dw, dh; //destination
	//float z = img->z;

	if (!img->zoomed) { /* if the image isn't zoomed */
		img->scale->arrange(img);
		//if (!(ABS(img->z - z) > 1.0 / MAX(img->w, img->h))) {
		//	img->z = z;
		//	img->x = xw.w / 2 - (xw.w / 2 - img->x) * img->z;
		//	img->y = xw.h / 2 - (xw.h / 2 - img->y) * img->z;
		//}
		img->checkpan = 1;
	}

	if (img->checkpan) {
		check_pan(img);
		img->checkpan = 0;
	}

 	/* calculate source and destination offsets */
	if (img->x <= 0) {
		sx = -img->x / img->z;
		sw = xw.w / img->z;
		dx = 0;
		dw = xw.w;
	} else {
		sx = 0;
		sw = img->w;
		dx = img->x;
		dw = img->w * img->z;
	}
	if (img->y <= 0) {
		sy = -img->y / img->z;
		sh = xw.h / img->z;
		dy = 0;
		dh = xw.h;
	} else {
		sy = 0;
		sh = img->h;
		dy = img->y;
		dh = img->h * img->z;
	}

	/* clear and set pixmap */
	if (xw.pm)
		XFreePixmap(xw.dpy, xw.pm);
	xw.pm = XCreatePixmap(xw.dpy, xw.win, xw.w, xw.h, xw.depth);
	XFillRectangle(xw.dpy, xw.pm, xw.gc, 0, 0, xw.w, xw.h);

	/* render image */
 	imlib_context_set_drawable(xw.pm);

	/* config context */
	imlib_context_set_anti_alias(antialiasing);

	/* context */
	imlib_context_set_image(img->im);
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

	float w = img->w * img->z;
	float h = img->h * img->z;

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

	if (z != img->z) {
		img->x -= (img->w * z - img->w * img->z) / 2;
		img->y -= (img->h * z - img->h * img->z) / 2;
		img->z = z;
		img->checkpan = 1;
		img->zoomed = 1;
	}
}
