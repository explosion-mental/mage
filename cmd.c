void
togglebar(const Arg *arg)
{
	showbar = !showbar;
	if (showbar) {
		bh = 0;
		printf("BAR ON\n");
	} else {
		bh = drw->fonts->h + 2;
		printf("bar off\n");
	}
	//updatebarpos();
	//XClearWindow(xw.dpy, bh);
	//XClearArea(xw.dpy, xw.win, 0, xw.h, xw.w, bh, True);
	XSync(xw.dpy, False);
	//img_load(&img, filenames[fileidx]);
	//img_display(&img, &xw);
	img_render(&image);
	drawbar();

	/* todo */
	/* while changing images; shall the image have a 'dirty' stage where it
	 * updates everything, or just call that function directly? */
}

void
advance(const Arg *arg)
{
	if (arg->i > 0) {
		if (fileidx + 1 < filecnt) {
			img_load(&image, filenames[++fileidx]);
			//im_clear();
			img_render(&image);
			update_title();
			drawbar();
		}
	} else {
		if (fileidx > 0) {
			img_load(&image, filenames[--fileidx]);
			//im_clear();
			img_render(&image);
			update_title();
			drawbar();
		}
	}
}

void
printfile(const Arg *arg)
{
	printf("This is the file '%s'\n", filenames[fileidx]);
}

void
zoom(const Arg *arg)
{
	int i;
	if (arg->i > 0) {
		//if (img_zoom(&img, 125)) {
		for (i = 1; i < zl_cnt; ++i)
			if (zoom_levels[i] > zoomlvl * 100.0) {
				img_zoom(&image, zoom_levels[i] / 100.0);
				break;
			}
		img_render(&image);
		update_title();
		drawbar();
		//}
	} else {
		//if (img_zoom(&img, -125)) {
		for (i = zl_cnt - 2; i >= 0; --i)
			if (zoom_levels[i] < zoomlvl * 100.0) {
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
pan(const Arg *arg)
{
	int ox, oy;

	ox = image.x;
	oy = image.y;

	switch (arg->i) {
		case LEFT:
			image.x += xw.w / 5;
			break;
		case RIGHT:
			image.x -= xw.w / 5;
			break;
		case UP:
			image.y += xw.h / 5;
			break;
		case DOWN:
			image.y -= xw.h / 5;
			break;
	}

	img_check_pan(&image);

	if (ox != image.x || oy != image.y) {
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

	ox = d == 1 ? img->x : xw.w - img->x - img->w * zoomlvl;
	oy = d == 3 ? img->y : xw.h - img->y - img->h * zoomlvl;

	imlib_image_orientate(d);

	img->x = oy + (xw.w - xw.h) / 2;
	img->y = ox + (xw.h - xw.w) / 2;

	tmp = img->w;
	img->w = img->h;
	img->h = tmp;

	img->checkpan = 1;
	img_render(img);
}

void
toggleantialias(const Arg *arg)
{
	Image *img = &image;

	img->aa ^= 1;
	imlib_context_set_anti_alias(img->aa);
	img_render(img);
}