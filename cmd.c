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
	XEvent ev;
	XClientMessageEvent *cm;

	memset(&ev, 0, sizeof(ev));
	ev.type = ClientMessage;

	cm = &ev.xclient;
	cm->window = xw.win;
	cm->message_type = atom[WMState];
	cm->format = 32;
	cm->data.l[0] = 2;
	cm->data.l[1] = atom[WMFullscreen];

	XSendEvent(xw.dpy, DefaultRootWindow(xw.dpy), False,
	           SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}
