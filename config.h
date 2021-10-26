/* See LICENSE file for copyright and license details. */

/* how much screen estate is to be used at max for the content */
static int showbar              = 1;        /* 0 means no bar */
static const int topbar         = 0;        /* 0 means bottom bar */
static char *fonts[] = {
	"SauceCodePro Nerd Font:pixelsize=14:antialias=true:autohint=true",
//	"Noto Color Emoji:pixelsize=16:antialias=true:autohint=true: style=Regular", /* Emojis */
	"JoyPixels:pixelsize=14:antialias=true:autohint=true"
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
	{ 0,            XK_k,          advance,        {.i = +1} },
	{ 0,            XK_j,          advance,        {.i = -1} },
	{ 0,            XK_p,          printfile,      {0} },
	//{ XK_f,           fullscreen,     {0} },
};

static Mousekey mshortcuts[] = {
	/* button         function        argument */
	{ Button1,        advance,     {.i = +1} },
	{ Button3,        quit,        {0} },
//	{ Button4,        quit,        {0} },
//	{ Button5,        quit,        {0} },
};
