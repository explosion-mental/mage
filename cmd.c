void
togglebar(const Arg *arg)
{
	showbar = !showbar;
	//if (showbar) {
	//	bh = 0;
	//	//printf("BAR ON\n");
	//} else {
	//	bh = drw->fonts->h + 2;
	//	printf("bar off\n");
	//}
	//updatebarpos();
	//XClearWindow(xw.dpy, bh);
	//XClearArea(xw.dpy, xw.win, 0, xw.h, xw.w, bh, True);
	XSync(xw.dpy, False);
	//img_load(&img, filenames[fileidx]);
	//img_display(&img, &xw);
	img_render(&image);
	drawbar();

	/* TODO */
	/* while changing images; shall the image have a 'dirty' stage where it
	 * updates everything, or just call that function directly? */
}

void
advance(const Arg *arg)
{
	if ((arg->i > 0 && fileidx + arg->i < filecnt)
	|| (arg->i < 0 && fileidx >= -arg->i)) {
		fileidx = fileidx + arg->i;
		img_load(&image, filenames[fileidx]);
		//im_clear();
		img_render(&image);
		update_title();
		drawbar();
	}
}

void
printfile(const Arg *arg)
{
	printf("This is the file '%s'\n", filenames[fileidx]);
}

void
zoom_steps(const Arg *arg)
{
	int i;
	if (arg->i > 0) {
		//if (img_zoom(&img, 125)) {
		for (i = 1; i < zoomcnt; ++i)
			if (zoom_levels[i] > zoomstate * 100.0) {
				img_zoom(&image, zoom_levels[i] / 100.0);
				break;
			}
		img_render(&image);
		update_title();
		drawbar();
		//}
	} else {
		//if (img_zoom(&img, -125)) {
		for (i = zoomcnt - 2; i >= 0; --i)
			if (zoom_levels[i] < zoomstate * 100.0) {
				//im_clear();
				img_zoom(&image, zoom_levels[i] / 100.0);
				break;
			}
		img_render(&image);
		update_title();
		drawbar();
		//}
	}
}

void
zoom(const Arg *arg)
{
	if (arg->i > 0)
		img_zoom(&image, zoomstate + arg->f / 100.0);
	else
		img_zoom(&image, zoomstate - -arg->f / 100.0);
	img_render(&image);
	update_title();
	drawbar();
}

void
togglefullscreen(const Arg *arg)
{
	XEvent e;

	e.type = ClientMessage;
	e.xclient.window = xw.win;
	e.xclient.message_type = atom[WMState];
	e.xclient.format = 32;
	e.xclient.data.l[0] = 2;
	e.xclient.data.l[1] = atom[WMFullscreen];
	e.xclient.data.l[2] = 0;
	XSendEvent(xw.dpy, DefaultRootWindow(xw.dpy), False,
	           SubstructureNotifyMask | SubstructureRedirectMask, &e);
}

void
panhorz(const Arg *arg)
{
	int x = image.x, y = image.y;

	if (arg->i > 0)
		image.x -= xw.w / arg->i;
	else
		image.x += xw.w / -arg->i;

	img_check_pan(&image);

	if (x != image.x || y != image.y) {
		img_render(&image);
		drawbar();
	}
}

void
panvert(const Arg *arg)
{
	int x = image.x, y = image.y;

	if (arg->i > 0)
		image.y += xw.h / arg->i;
	else
		image.y -= xw.h / -arg->i;

	img_check_pan(&image);

	if (x != image.x || y != image.y) {
		img_render(&image);
		drawbar(); //panning without bar?
	}
}

void
first(const Arg *arg)
{
	if (fileidx != 0) {
		fileidx = 0;
		img_load(&image, filenames[fileidx]);
		img_render(&image);
		drawbar();
	}
}

void
last(const Arg *arg)
{
	if (fileidx != filecnt - 1) {
		fileidx = filecnt - 1;
		img_load(&image, filenames[fileidx]);
		img_render(&image);
		drawbar();
	}
}

void
rotate(const Arg *arg)
{
	Image *img = &image;
	int ox, oy, tmp, d;

	if (arg->i > 0)
		d = 1;
	else
		d = 3;

	ox = d == 1 ? img->x : xw.w - img->x - img->w * zoomstate;
	oy = d == 3 ? img->y : xw.h - img->y - img->h * zoomstate;

	imlib_image_orientate(d);
	//the below commands overwrites the current state of the buffer image
	//into the real image (file)
	//imlib_save_image(filenames[fileidx]);

	img->x = oy + (xw.w - xw.h) / 2;
	img->y = ox + (xw.h - xw.w) / 2;

	tmp = img->w;
	img->w = img->h;
	img->h = tmp;

	img->checkpan = 1;
	img_render(img);
	drawbar();
}

void
toggleantialias(const Arg *arg)
{
	image.aa = !image.aa;
	imlib_context_set_anti_alias(image.aa);
	img_render(&image);
	drawbar();
}

void
reload(const Arg *arg)
{
	img_load(&image, filenames[fileidx]);
	img_render(&image);
	update_title();
	drawbar();
}

void
cyclescale(const Arg *arg)
{
	if (arg->i > 0) {
		if (scalemode < LENGTH(scales) - 1)
			scalemode++;
		else
			scalemode = 0;
		//here I invoke reload in order to invoke img_load which sets
		//`img->scalemode = scalemode`, in the future I would probably
		//remove scalemode from the Image struct since right now
		//doesn't seem very useful
		reload(0);
	} else {
		if (scalemode > 0)
			scalemode--;
		else
			scalemode = LENGTH(scales) - 1;
		reload(0);
	}
}

void
savestate(const Arg *arg)
{
	imlib_save_image(filenames[fileidx]);
}
