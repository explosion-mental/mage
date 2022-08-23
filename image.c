void
singleview(void)
{
	int sx, sy, sw, sh; //source
	int dx, dy, dw, dh; //destination

	ci->im = imlib_load_image(ci->fname);
	imlib_context_set_image(ci->im);

	ci->w = imlib_image_get_width();
	ci->h = imlib_image_get_height();

	if (!ci->zoomed) { /* if the image isn't zoomed */
		scale->arrange(ci);
		ci->checkpan = 1;
	}

	if (ci->checkpan) {
		check_pan(ci);
		ci->checkpan = 0;
	}

 	/* calculate source and destination offsets */
	if (ci->x <= 0) {
		sx = -ci->x / ci->z;
		sw = winw / ci->z;
		dx = 0;
		dw = winw;
	} else {
		sx = 0;
		sw = ci->w;
		dx = ci->x;
		dw = ci->w * ci->z;
	}
	if (ci->y <= 0) {
		sy = -ci->y / ci->z;
		sh = winy / ci->z;
		dy = 0;
		dh = winy;
	} else {
		sy = 0;
		sh = ci->h;
		dy = ci->y;
		dh = ci->h * ci->z;
	}

	/* clear and set pixmap */
	if (pm)
		XFreePixmap(dpy, pm);

	pm = XCreatePixmap(dpy, win, scrw, scrh, depth);
	XFillRectangle(dpy, pm, gc, 0, 0, scrw, scrh);

	/* config context */
	imlib_context_set_anti_alias(antialiasing);

	/* render image */
 	imlib_context_set_drawable(pm);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	/* window background */
	XSetWindowBackgroundPixmap(dpy, win, pm);
	XClearWindow(dpy, win);
}

/* TODO:
 * - move to the next rows */

int
calc_block(int dimension, int padding, int size)
{	/* how many thumbs fit in the dimension */
	return dimension / (size + padding);
}

void
thumbnailview(void)
{
	unsigned int i;
	int x = 0, y = 0;
	unsigned int rows, cols, n;
	Image *t;

	cols = calc_block(winw, thumbpad, thumbsize );
	rows = calc_block(winh, thumbpad, thumbsize );

	/* debug */
//	printf("COLS: %d\n", cols);
//	printf("ROWS: %d\n", rows);
//	printf("WIDTH: %d\n", winw);
//	printf("HEIGHT: %d\n", winh);

	n = cols * rows; /* max number of images to show */

	if (n > filecnt) /* make sure it isn't more than all the images */
		n = filecnt;

	/* clean pixmap (our canvas) */
	if (pm)
		XFreePixmap(dpy, pm);
	pm = XCreatePixmap(dpy, win, scrw, scrh, depth);
	XFillRectangle(dpy, pm, gc, 0, 0, scrw, scrh);
	imlib_context_set_drawable(pm);

	/* load and render images */
	for (i = 0; i < n; i++) {
		t = &images[i];

		/* cancel if there is user input.
		 * TODO: when switching too fast with images none it's
		 * displayed */
		if (XPending(dpy) != 0)
			break;

		if (!t->im)
			t->im = imlib_load_image(t->fname);

		imlib_context_set_image(t->im);

		if (!t->w)
			t->w = imlib_image_get_width();
		if (!t->h)
			t->h = imlib_image_get_height();

		imlib_context_set_anti_alias(1); //faster but less quality

		/* width and height no bigger than size */
		t->w = MAX(thumbsize, t->w / thumbsize); //thumbsize or half the image, needs more operations
		t->h = MAX(thumbsize, t->h / thumbsize);

		if ((i % cols) == 0) { /* first row filled */
			x = thumbpad;
			y += thumbsize + thumbpad; /* move to the next row */
		} else /* there is space */
			x += thumbsize + thumbpad; /* move to the next col */

		t->x = x;
		t->y = y;

		/* render image */
		imlib_render_image_on_drawable_at_size(t->x, t->y, t->w, t->h);
	}

	/* draw rectangle around the current image */
	XGCValues gcval;
	GC c;
	gcval.foreground = scheme[SchemeNorm][ColFg].pixel;
	c = XCreateGC(dpy, win, GCForeground, &gcval); //context for Pixmap
	XDrawRectangle(dpy, pm, c, ci->x - 3, ci->y - 3, ci->w + 6, ci->h + 6);
	XFreeGC(dpy, c);

	/* update window */
	XSetWindowBackgroundPixmap(dpy, win, pm);
	XClearWindow(dpy, win);
}

void
check_pan(Image *img)
{
	if (!img)
		return;

	float w = img->w * img->z;
	float h = img->h * img->z;

	if (w < winw)
		img->x = (winw - w) / 2;
	else if (img->x > 0)
		img->x = 0;
	else if (img->x + w < winw)
		img->x = winw - w;
	if (h < winy)
		img->y = (winy - h) / 2;
	else if (img->y > 0)
		img->y = 0;
	else if (img->y + h < winy)
		img->y = winy - h;
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

void
scalewidth(Image *im)
{
	im->z = MIN((float) winw / (float) im->w, maxzoom / 100.0);
}

void
scaleheight(Image *im)
{
	im->z = MIN((float) winy / (float) im->h, maxzoom / 100.0);
}

void
scaledown(Image *im)
{
	im->z = MIN((float) winw / (float) im->w, (float) winy / (float) im->h);
	im->z = MIN(im->z, 1.0);
}

void
scalefit(Image *im)
{
	im->z = MIN((float) winw / (float) im->w, (float) winy / (float) im->h);
}

