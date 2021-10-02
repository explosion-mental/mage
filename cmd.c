void
togglebar()
{
	showbar = !showbar;
	//updatebarpos();
	//drawbar();
}

void
next()
{
	if (fileidx + 1 < filecnt) {
		img_load(&img, fnames[++fileidx]);
		img_display(&img, &xw);
	}
}

void
prev()
{
	if (fileidx > 0) {
		img_load(&img, fnames[--fileidx]);
		img_display(&img, &xw);
	}
}
