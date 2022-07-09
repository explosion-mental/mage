/* See LICENSE file for copyright and license details. */
/* Default settings; can be overriden by command line. */

static int showbar         = 1;          /* 0 means no bar */
static int antialiasing    = 1;          /* 0 means pixelize images */
static int quiet           = 0;          /* 0 means print warnings */
static int recursive       = 0;          /* 1 means load subdirectories */
static const int winwidth  = 800;        /* default window width */
static const int winheight = 600;        /* default window height */
static const float maxzoom = 800.0;      /* max value that zoom can reach (ignored by scaling) */
static const float minzoom = 12.5;       /* min value that zoom can reach (ignored by scaling) */
static const char *fonts[] = { "monospace" };

static const ScaleMode scalemodes[] = {
	/* symbol   arrange function */
	{ "down",   scaledown },  /* first entry is default */
	{ "width",  scalewidth },
	{ "fit",    scalefit },
	{ "height", scaleheight },
};

static const char *colors[][2] = {
      			/*  fg       bg     */
	[SchemeNorm]  = { "#eeeeee", "#005577" },
	[SchemeBar]   = { "#bbb012", "#411828" },
};

static const Shortcut shortcuts[] = {
	/* modifier     key              function            argument */
	{ 0,            XK_Escape,       quit,               {0} },
	{ 0,            XK_q,            quit,               {0} },
	{ 0,            XK_b,            togglebar,          {0} },
	{ 0,            XK_space,        advance,            {.i = +1} },
	{ 0,            XK_BackSpace,    advance,            {.i = -1} },
	{ 0,            XK_bracketright, advance,            {.i = +5} },
	{ 0,            XK_bracketleft,  advance,            {.i = -5} },
	{ 0,            XK_p,            printfile,          {0} },
	{ 0,            XK_r,            reload,             {0} },
	{ 0,            XK_minus,        zoom,               {.f = -12.5} },
	{ 0,            XK_equal,        zoom,               {.f = +12.5} },
	{ ShiftMask,    XK_equal,        set_zoom,           {.f = 100} },
	{ 0,            XK_f,            togglefullscreen,   {0} },
	{ 0,            XK_a,            toggleantialias,    {0} },
	{ 0,            XK_s,            savestate,          {0} },
	{ 0,            XK_u,            flipvert,           {0} },
	{ ShiftMask,    XK_u,            fliphorz,           {0} },
	{ 0,            XK_k,            panvert,            {.f = +5} },
	{ 0,            XK_j,            panvert,            {.f = -5} },
	{ 0,            XK_l,            panhorz,            {.f = +5} },
	{ 0,            XK_h,            panhorz,            {.f = -5} },
	{ 0,            XK_g,            first,              {0} },
	{ ShiftMask,    XK_g,            last,               {0} },
	{ ShiftMask,    XK_period,       rotate,             {.i = +1 } },
	{ ShiftMask,    XK_comma,        rotate,             {.i = -1 } },
	{ 0,            XK_backslash,    cyclescale,         {.i = +1 } },
	{ ShiftMask,    XK_backslash,    cyclescale,         {.i = -1 } },
};

static const Mousekey mshortcuts[] = {
	/* button         function        argument */
	{ Button1,        advance,     {.i = +1} },
	{ Button2,        quit,        {0} },
	{ Button3,        advance,     {.i = -1} },
	{ Button4,        zoom,        {.f = +3} },
	{ Button5,        zoom,        {.f = -3} },
};
