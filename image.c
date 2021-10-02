void
imlib_init(XWindow *win)
{
	if (!win)
		return;
	imlib_context_set_display(win->dpy);
	imlib_context_set_visual(win->vis);
	imlib_context_set_colormap(win->cmap);
	imlib_context_set_drawable(win->win);
}

void
imlib_destroy()
{
	if (imlib_context_get_image())
		imlib_free_image();
}

void
img_load(Image *image, const char *filename)
{
	Imlib_Image *im;
	if (!image || !filename)
		return;

	if (imlib_context_get_image())
		imlib_free_image();

	if (!(im = imlib_load_image(filename)))
		die("could not open image: %s", filename);

	imlib_context_set_image(im);

	image->w = imlib_image_get_width();
	image->h = imlib_image_get_height();
}

void
//img_render(Image *image, XWindow *win)
img_render(Image *image, XWindow *win, int x, int y, int w, int h)
{
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;

	if (!image || !win || !imlib_context_get_image())
		return;

	if (image->x < x) {
		sx = (x - image->x) / zoom;
		sw = MIN(w / zoom, image->w - sx);
		dx = x;
		dw = sw * zoom;
	} else {
		sx = 0;
		sw = MIN((w - image->x + x) / zoom, image->w);
		dx = image->x;
		dw = sw * zoom;
	}
	if (image->y < y) {
		sy = (y - image->y) / zoom;
		sh = MIN(h / zoom, image->h - sy);
		dy = y;
		dh = sh * zoom;
	} else {
		sy = 0;
		sh = MIN((h - image->y + y) / zoom, image->h);
		dy = image->y;
		dh = sh * zoom;
	}

	if (sw < 0 || sh < 0)
		return;

	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);
}


void
im_clear(XWindow *win)
{
	XClearWindow(win->dpy, win->win);
}

void
img_display(Image *image, XWindow *win)
{

	if (!image || !win || !imlib_context_get_image())
		return;

	float zw, zh;
	/* set zoom level to fit image into window */
	if (scalemode != SCALE_ZOOM) {
		zw = (float) win->w / (float) image->w;
		zh = (float) win->h / (float) image->h;
		zoom = MIN(zw, zh);

		if (zoom * 100.0 < ZOOM_MIN)
			zoom = ZOOM_MIN / 100.0;
		else if (zoom * 100.0 > ZOOM_MAX)
			zoom = ZOOM_MAX / 100.0;

		if (scalemode == SCALE_DOWN && zoom > 1.0)
			zoom = 1.0;
	}

	/* center image in window */
	image->x = (win->w - image->w * zoom) / 2;
	image->y = (win->h - image->h * zoom) / 2;

	im_clear(win);

	img_render(image, win, 0, 0, win->w, win->h);
}
