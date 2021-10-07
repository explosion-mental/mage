void
togglebar(const Arg *arg)
{
	printf("bar off\n");
	//fprintf(stderr, "Previous");
	showbar = !showbar;
	//updatebarpos();
	drawbar();
}

void
advance(const Arg *arg)
{
	if (arg->i > 0) {
		if (fileidx + 1 < filecnt) {
			img_load(&img, filenames[++fileidx]);
			img_display(&img, &xw);
		}
	} else {
		if (fileidx > 0) {
			img_load(&img, filenames[--fileidx]);
			img_display(&img, &xw);
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
