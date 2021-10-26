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
	img_render(&img, &xw);
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
printfile(const Arg *arg)
{
	printf("This is the file '%s'\n", filenames[fileidx]);
}
