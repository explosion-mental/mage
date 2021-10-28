void
imlib_init(XWindow *win)
{
	if (!win)
		return;
	imlib_context_set_display(win->dpy);
	imlib_context_set_visual(visual);
	imlib_context_set_colormap(cmap);
	imlib_context_set_drawable(win->win);
}

void
imlib_destroy()
{
	if (imlib_context_get_image())
		imlib_free_image();
}

int
img_load(Image *img, const char *filename)
{
	Imlib_Image *im;
	if (!img || !filename)
		return -1;

	if (imlib_context_get_image())
		imlib_free_image();

	if (!(im = imlib_load_image(filename))) {
		printf("could not open image: %s\n", filename);
		return -1;
	}

	imlib_context_set_image(im);

 	img->re = 0;
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	return 0;
}

void
img_render(Image *img, XWindow *win)
{
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;
 	float zw, zh;

	if (!img || !win || !imlib_context_get_image())
		return;

 	//img_check_pan(image, win);
	if (!img->re) {
		/* rendered for the first time */
		img->re = 1;

		/* set zoom level to fit image into window */
		if (scalemode != SCALE_ZOOM) {
			zw = (float) win->w / (float) img->w;
			zh = (float) win->h / (float) img->h;
			zoomlvl = MIN(zw, zh);

			if (zoomlvl < zoom_min)
				zoomlvl = zoom_min;
			else if (zoomlvl > zoom_max)
				zoomlvl = zoom_max;

			if (scalemode == SCALE_DOWN && zoomlvl > 1.0)
				zoomlvl = 1.0;
		}

		/* center image in window */
		img->x = (win->w - img->w * zoomlvl) / 2;
		img->y = (win->h - img->h * zoomlvl) / 2;
	} else
		//if (img->cp)
		{
		/* only useful after zooming */
		img_check_pan(img, win);
 		//img->cp = 0;
	}

 	/* calculate source and destination offsets */
	if (img->x < 0) {
		sx = -img->x / zoomlvl;
		sw = win->w / zoomlvl;
		dx = 0;
		dw = win->w;
	} else {
		sx = 0;
		sw = img->w;
		dx = img->x;
		dw = img->w * zoomlvl;
	}
	if (img->y < 0) {
		sy = - img->y / zoomlvl;
		sh = win->h / zoomlvl;
		dy = 0;
		dh = win->h;
	} else {
		sy = 0;
		sh = img->h;
		dy = img->y;
		dh = img->h * zoomlvl;
	}

	//XClearWindow(win->dpy, win->win);

	//imlib_context_set_drawable(win->pm);
	im_clear();
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);
	//XClearWindow(win->dpy, win->win);

}


void
im_clear(void)
{
	XClearWindow(xw.dpy, xw.win);
}

void
img_check_pan(Image *img, XWindow *win)
{
	if (!img)
		return;

	if (img->w * zoomlvl > win->w) {
		if (img->x > 0 && img->x + img->w * zoomlvl > win->w)
			img->x = 0;
		if (img->x < 0 && img->x + img->w * zoomlvl < win->w)
			img->x = win->w - img->w * zoomlvl;
	} else {
		img->x = (win->w - img->w * zoomlvl) / 2;
	}
	if (img->h * zoomlvl > win->h) {
		if (img->y > 0 && img->y + img->h * zoomlvl > win->h)
			img->y = 0;
		if (img->y < 0 && img->y + img->h * zoomlvl < win->h)
			img->y = win->h - img->h * zoomlvl;
	} else {
		img->y = (win->h - img->h * zoomlvl) / 2;
	}
}

int
img_zoom(Image *img, float z)
{
	if (z < zoom_min)
		z = zoom_min;
	else if (z > zoom_max)
		z = zoom_max;

	if (z != zoomlvl) {
		img->x -= (img->w * z - img->w * zoomlvl) / 2;
		img->y -= (img->h * z - img->h * zoomlvl) / 2;
		zoomlvl = z;
		return 1;
	} else {
		return 0;
	}
}

//int
//img_pan(Image *img, XWindow *win)
//{
//	int ox, oy;
//
//	if (!img || !win)
//		return 0;
//
//	ox = img->x;
//	oy = img->y;
//
//	switch (dir) {
//		case PAN_LEFT:
//			img->x += win->w / 5;
//			break;
//		case PAN_RIGHT:
//			img->x -= win->w / 5;
//			break;
//		case PAN_UP:
//			img->y += win->h / 5;
//			break;
//		case PAN_DOWN:
//			img->y -= win->h / 5;
//			break;
//	}
//
//	img_check_pan(img, win);
//
//	return ox != img->x || oy != img->y;
//}
