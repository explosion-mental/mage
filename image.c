void
im_destroy()
{
//TODO after the 'cache'ing images this function should accept `Image *` as an
//argument in order to reuse this
	int i;

	if (image.im) {
		imlib_free_image();
	//	imlib_free_image_and_decache();
		image.im = NULL;
	}

	if (thumbs) {
		for (i = 0; i < filecnt; i++) {
			if (thumbs[i].im) {
				imlib_context_set_image(thumbs[i].im);
				imlib_free_image();
			}
		}
		free(thumbs);
		thumbs = NULL;
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

	/* config context */
	imlib_context_set_anti_alias(antialiasing);
	imlib_context_set_blend(blend);

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








/* thumbnail mode todoes:
 * - calculate spcae between images
 * - should we use `imlib_render_image_on_drawable_at_size` or `imlib_create_cropped_scaled_image`?(which one is 'faster')
 */

#define THUMB_SIZE  50

void
tns_render(Image *t)
{
	int i, margin, width = 0, store;
	unsigned int rows, cols, top, bottom;

	margin = 10;

	/* clear and set pixmap */
	if (xw.pm)
		XFreePixmap(xw.dpy, xw.pm);
	xw.pm = XCreatePixmap(xw.dpy, xw.win, xw.scrw, xw.scrh, xw.depth);
	XFillRectangle(xw.dpy, xw.pm, xw.gc, 0, 0, xw.scrw, xw.scrh);
	imlib_context_set_drawable(xw.pm);


	/* calculate cols and rows */
//	for (cols = 0; cols <= filecnt/2; cols++)
//		if (cols*cols >= filecnt)
//			break;
//
//	if (filecnt == 5) /* set layout against the general calculation: not 1:2:2, but 2:3 */
//		cols = 2;
	int tmp;
	//for (i = 0; i < filecnt; i++) {
	//	tmp += THUMB_SIZE;
	//	if (tmp >= xw.w)
	//		cols = i - 1;
	//}

	cols = xw.w / THUMB_SIZE;
	rows = xw.h / THUMB_SIZE;
	printf("COLS: %d\n", cols);

	/* load images */
	//i is the index
	for (i = 0; i < filecnt; i++) {
		img_load(&t[i], filenames[i]);
		imlib_context_set_anti_alias(1); //faster but less quality

		//operations ideas..
		//t[i].x = (THUMB_SIZE - t[i].w) / 2;
		//t[i].y = (THUMB_SIZE - t[i].h) / 2;
		//space += space;
		//t[i].x = t[i].w / 2;
		//t[i].y = t[i].h / 2;

		//int x = (t[i].w < t[i].h) ? 0 : (t[i].w - t[i].h) / 2;
		//int y = (t[i].w > t[i].h) ? 0 : (t[i].h - t[i].w) / 2;
		//int s = (t[i].w < t[i].h) ? t[i].w : t[i].h;

		/* render the image */
		//TODO should we use `imlib_render_image_on_drawable_at_size` or `imlib_create_cropped_scaled_image`?(which one is 'faster')
		//imlib_context_set_image(t[i].im);
		//imlib_render_image_part_on_drawable_at_size(0, 0, t[i].w, t[i].h, t[i].x, t[i].y, THUMB_SIZE, THUMB_SIZE);
		//image.im = imlib_create_cropped_scaled_image(0, 0, THUMB_SIZE, THUMB_SIZE, THUMB_SIZE, THUMB_SIZE);
		//t[i].im = imlib_create_cropped_scaled_image(t[i].x, t[i].y, t[i].w, t[i].h, THUMB_SIZE, THUMB_SIZE);
		//imlib_render_image_on_drawable_at_size(t[i].x + margin + THUMB_SIZE, t[i].y + margin, t[i].w, t[i].h);

		float x, y;

		//t[i].y = i * THUMB_SIZE;

		/* width and height no bigger than size */
		t[i].w = MIN(THUMB_SIZE, t[i].w / 2); //thumbsize or half the image, needs more operations
		t[i].h = MIN(THUMB_SIZE, t[i].h / 2);

		int j;
		/* calculate position */
		//for (j = 1; j < cols + 1; j++) {//this has to increment every time the i loops runs..
		//	x = (xw.w - THUMB_SIZE - margin) / j;
		//	break;
		//}

		if (i % cols == 0) {
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
		//imlib_context_set_image(t[i].im);
		//if (width > xw.w) {
		//	t[i].y = t[i].h + margin;
		//	store = i;
		//}
	}

	/* window background */
	XSetWindowBackgroundPixmap(xw.dpy, xw.win, xw.pm);
	XClearWindow(xw.dpy, xw.win);
}
