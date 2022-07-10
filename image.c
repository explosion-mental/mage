void
im_destroy()
{
//after the 'cache'ing images this function should accept `Image *` as an
//argument in order to reuse this
	if (ci->im) {
		imlib_free_image();
	//	imlib_free_image_and_decache();
		ci->im = NULL;
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
	im->z = MIN((float) winh / (float) im->h, maxzoom / 100.0);
}

void
scaledown(Image *im)
{
	im->z = MIN((float) winw / (float) im->w, (float) winh / (float) im->h);
	im->z = MIN(im->z, 1.0);
}

void
scalefit(Image *im)
{
	im->z = MIN((float) winw / (float) im->w, (float) winh / (float) im->h);
}

void
img_render(Image *img)
{
	int sx, sy, sw, sh; //source
	int dx, dy, dw, dh; //destination
	//float z = img->z;

	img->im = imlib_load_image(img->fname);
	imlib_context_set_image(img->im);
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	if (!img->zoomed) { /* if the image isn't zoomed */
		scale->arrange(img);
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
		sw = winw / img->z;
		dx = 0;
		dw = winw;
	} else {
		sx = 0;
		sw = img->w;
		dx = img->x;
		dw = img->w * img->z;
	}
	if (img->y <= 0) {
		sy = -img->y / img->z;
		sh = winh / img->z;
		dy = 0;
		dh = winh;
	} else {
		sy = 0;
		sh = img->h;
		dy = img->y;
		dh = img->h * img->z;
	}

	/* clear and set pixmap */
	if (pm)
		XFreePixmap(dpy, pm);
	pm = XCreatePixmap(dpy, win, winw, winh, depth);
	XFillRectangle(dpy, pm, gc, 0, 0, winw, winh);

	/* render image */
 	imlib_context_set_drawable(pm);

	/* config context */
	imlib_context_set_anti_alias(antialiasing);

	/* context */
	imlib_context_set_image(img->im);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

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
	if (h < winh)
		img->y = (winh - h) / 2;
	else if (img->y > 0)
		img->y = 0;
	else if (img->y + h < winh)
		img->y = winh - h;
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
