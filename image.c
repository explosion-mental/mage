void
singleview(void)
{
	int sx, sy, sw, sh; //source
	int dx, dy, dw, dh; //destination
	//float z = img->z;

	ci->im = imlib_load_image(ci->fname);
	imlib_context_set_image(ci->im);

	if (!ci->w)
		ci->w = imlib_image_get_width();
	if (!ci->h)
		ci->h = imlib_image_get_height();

	if (!ci->zoomed) { /* if the image isn't zoomed */
		scale->arrange(ci);
		//if (!(ABS(img->z - z) > 1.0 / MAX(img->w, img->h))) {
		//	img->z = z;
		//	img->x = xw.w / 2 - (xw.w / 2 - img->x) * img->z;
		//	img->y = xw.h / 2 - (xw.h / 2 - img->y) * img->z;
		//}
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


/* TODO finish thumbnail mode
 * - correctly place the first image
 * - cancel rendering thumbs if there is user input
 */

#define THUMB_SIZE	(128)
#define THUMB_PAD	(8)

int
calc_block(int dymension, int padding, int thumb_size ) {
	/* how many thumbs fit in the dymension */
	int fit_num_raw = dymension / ( thumb_size + padding );
	/* always ignore the decimals and floor the value */
	//int fit_num = math_floor( fit_num_raw )
	return fit_num_raw;
}

void
thumbnailview(void)
{
	int i, margin;
	//int width = 0, store;
	unsigned int rows, cols;
	//unsigned int top, bottom;
	unsigned int n;
	//int hpad, wpad, thumbpad;

	margin = 10;
	Image *t = images;

	/* clear and set pixmap */
	if (pm)
		XFreePixmap(dpy, pm);
	pm = XCreatePixmap(dpy, win, scrw, scrh, depth);
	XFillRectangle(dpy, pm, gc, 0, 0, scrw, scrh);
	imlib_context_set_drawable(pm);

	//int tmpw, tmph;

	cols = calc_block( winw, THUMB_PAD, THUMB_SIZE );
	rows = calc_block( winh, THUMB_PAD, THUMB_SIZE );


	printf("COLS: %d\n", cols);
	printf("ROWS: %d\n", rows);
	printf("WIDTH: %d\n", winw);
	printf("HEIGHT: %d\n", winh);

	n = cols * rows;
	if (n > filecnt)
		n = filecnt;
	int x = 0, y = 0;

	for (i = 0; i < n; i++) {
		/* cancel if there is user input */
		if (XPending(dpy) != 0)
			break;

		if (!t[i].im)
			t[i].im = imlib_load_image(t[i].fname);
		imlib_context_set_image(t[i].im);

		if (!t[i].w)
			t[i].w = imlib_image_get_width();
		if (!t[i].h)
			t[i].h = imlib_image_get_height();

		imlib_context_set_anti_alias(1); //faster but less quality


		t[i].y = i * THUMB_SIZE;

		/* width and height no bigger than size */
		t[i].w = MAX(THUMB_SIZE, t[i].w / THUMB_SIZE); //thumbsize or half the image, needs more operations
		t[i].h = MAX(THUMB_SIZE, t[i].h / THUMB_SIZE);

		int j;

		if (i == cols) {
			/* first row filled */
			x = margin;
			y += THUMB_SIZE + margin;
		} else {
			x += THUMB_SIZE + margin;
		}


		t[i].x = x;
		t[i].y = y;
		//t[i].x = i * t[i].w + cols * i + margin;	//take the count


		//t[i].y = y;
		//t[i].x = i * t[i].w + i/ xw.w + margin;

		imlib_render_image_on_drawable_at_size(t[i].x, t[i].y, t[i].w, t[i].h);
	}

	/* window background */
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

