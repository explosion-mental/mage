void
singleview(void)
{
	int sx, sy, sw, sh; //source
	int dx, dy, dw, dh; //destination

	imlib_context_set_image(ci->im);

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

	/* render image */
 	imlib_context_set_drawable(pm);
	imlib_context_set_anti_alias(antialiasing);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	/* window background */
	XSetWindowBackgroundPixmap(dpy, win, pm);
	XClearWindow(dpy, win);
}

/* TODO:
 * - move to the next rows */

void
loadimgs(Image *i)
{
	float z, zw, zh;

	if (i->crop) /* crop alrd exist */
		return;

	if (!i->im) {
		i->im = imlib_load_image(i->fname);
	}

	imlib_context_set_image(i->im);
	/* real width and height */
	i->w = imlib_image_get_width();
	i->h = imlib_image_get_height();
	/* croped w and h */
	zw = (float) thumbsize / (float) i->w;
	zh = (float) thumbsize / (float) i->h;
	z = MIN(zw, zh);
	if (!i->im && z > 1.0)
		z = 1.0;
	/* croped width and height */
	i->cw = z * i->w;
	i->ch = z * i->h;

	imlib_context_set_anti_alias(1);
	if (!(i->crop = imlib_create_cropped_scaled_image(0, 0, i->w, i->h, i->cw, i->ch)))
		die("could not allocate memory.");
}

void
thumbnailview(void)
{
	Image *t;
	int x = 0, y = 0;
	unsigned int i, rows, cols, n;

	/* how many thumbs fit in the dimension */
	cols = winw / (thumbpad + thumbsize);
	rows = winh / (thumbpad + thumbsize);

	n = cols * rows; /* max number of images to show */

	if (n > filecnt) /* make sure it isn't more than all the images */
		n = filecnt;

	/* clean pixmap (our canvas) */
	if (pm)
		XFreePixmap(dpy, pm);
	pm = XCreatePixmap(dpy, win, scrw, scrh, depth);
	XFillRectangle(dpy, pm, gc, 0, 0, scrw, scrh);
	imlib_context_set_drawable(pm);
	y = (winh - (rows * (thumbpad + thumbsize))) / 2;
	x = (winw - (cols * (thumbpad + thumbsize))) / 2;

	/* load and render images */
	for (i = 0; i < n; i++) {
		t = &images[i];

		if (!t->crop) /* if no image, quit here in order to access loadimgs() in run() */
			return;

		t->x = x;
		t->y = y;
		/* render image */
		imlib_context_set_image(t->crop);
		imlib_context_set_anti_alias(1); //faster but less quality
		imlib_render_image_on_drawable_at_size(t->x, t->y, t->cw, t->ch);

		if (((i + 1) % cols) == 0) { /* row filled */
			x = (winw - (cols * (thumbpad + thumbsize))) / 2;
			y += thumbsize + thumbpad; /* move to the next row */
		} else	/* there is space */
			x += thumbsize + thumbpad; /* move to the next col */
	}

	/* draw rectangle around the current image */
	XGCValues gcval;
	GC c;
	gcval.foreground = scheme[SchemeNorm][ColFg].pixel;
	c = XCreateGC(dpy, win, GCForeground, &gcval); //context for Pixmap
	XDrawRectangle(dpy, pm, c, ci->x - 3, ci->y - 3, ci->cw + 6, ci->ch + 6);
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

