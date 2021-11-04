/* See LICENSE file for copyright and license details. */

static int showbar         = 1;        /* 0 means no bar */
static const int topbar    = 0;        /* 0 means bottom bar */
static int antialiasing    = 1;        /* 0 means pixelize images */
static scaling scalemode   = SCALE_DOWN; /* SCALE_DOWN - SCALE_FIT - SCALE_ZOOM */
static const char *fonts[] = { "monospace" };

/* how much screen estate is to be used at max for the content */
static const float zoom_levels[] = {
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};

static const char *colors[][3] = {
      			/*  fg       bg     */
	[SchemeNorm]  = { "#eeeeee", "#005577" },
	[SchemeSel]   = { "#bbbbbb", "#882100" },
	[SchemeBar]   = { "#bbb012", "#411828" },
};

static Shortcut shortcuts[] = {
	/* mod          keysym         function        argument */
	{ 0,            XK_Escape,     quit,           {0} },
	{ 0,            XK_q,          quit,           {0} },
	{ 0,            XK_b,          togglebar,      {0} },
	{ 0,            XK_BackSpace,  advance,        {.i = -1} },
	{ 0,            XK_space,      advance,        {.i = +1} },
	{ 0,            XK_p,          printfile,      {0} },
	{ 0,            XK_minus,      zoom,           {.i = -1} },
	{ 0,            XK_equal,      zoom,           {.i = +1} },
	{ ShiftMask,    XK_f,       togglefullscreen,     {0} },
	{ 0,            XK_f,          img_fit,        {0} },
	{ 0,            XK_a,       toggleantialias,     {0} },
	{ 0,            XK_h,          pan,            {.i = LEFT} },
	{ 0,            XK_j,          pan,            {.i = DOWN} },
	{ 0,            XK_k,          pan,            {.i = UP} },
	{ 0,            XK_l,          pan,            {.i = RIGHT} },
	{ 0,            XK_g,          first,          {0} },
	{ ShiftMask,    XK_g,          last,           {0} },
	//{ 0,            XK_greater,    rotate,         {.i = +1 } },
	//{ 0,            XK_less,       rotate,         {.i = -1 } },
	{ ShiftMask,    XK_period,     rotate,         {.i = +1 } },
	{ ShiftMask,    XK_comma,      rotate,         {.i = -1 } },
};

static Mousekey mshortcuts[] = {
	/* button         function        argument */
	{ Button1,        advance,     {.i = +1} },
	{ Button2,        quit,        {0} },
	{ Button3,        advance,     {.i = -1} },
	{ Button4,        zoom,        {.i = +1} },
	{ Button5,        zoom,        {.i = -1} },
};
