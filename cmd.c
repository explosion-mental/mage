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
	img_display(&img, &xw);
	//img_render(&img, &xw);
	drawbar();
}

void
advance(const Arg *arg)
{
	if (arg->i > 0) {
		if (fileidx + 1 < filecnt) {
			img_load(&img, filenames[++fileidx]);
			img_display(&img, &xw);
			update_title();
			drawbar();
		}
	} else {
		if (fileidx > 0) {
			img_load(&img, filenames[--fileidx]);
			img_display(&img, &xw);
			update_title();
			drawbar();
		}
	}
}

void
next_img(const Arg *arg)
{
	printf("NEXT\n");
	if (fileidx + 1 < filecnt) {
		img_load(&img, filenames[++fileidx]);
		img_display(&img, &xw);
	}
	//if (fileidx + 1 < filecnt) {
	//	XClearWindow(xw.dpy, xw.win);
	//	img_load(&img, filenames[++fileidx]);
	//	img_display(&img, &xw);
	//}
}

void
prev_img(const Arg *arg)
{
	printf("Previous\n");
	//fprintf(stderr, "Previous");
	if (fileidx > 0) {
		//XClearWindow(xw.dpy, xw.win);
		img_load(&img, filenames[--fileidx]);
		img_display(&img, &xw);
	}
}
