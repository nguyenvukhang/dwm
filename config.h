#pragma once

#include "dtypes.h"

/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "sans:size=10.5" };
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_accent_400[]  = "#d8b4fe";
static const char col_accent_900[]  = "#581c87";
static const char *COLORS[][3]      = {
	/*               fg          bg               border   */
	[SchemeNorm] = { col_gray3,  col_gray1,       col_gray2      },
	[SchemeSel]  = { col_gray1,  col_accent_400,  col_accent_400 },
	[SchemeBar]  = { col_gray3,  col_gray2,       col_gray2      },
};

/* tagging */
static const char *TAGS[] = { "1", "2", "3", "4", "T" };
#define TAG_1 1
#define TAG_2 (1 << 1)
#define TAG_3 (1 << 2)
#define TAG_4 (1 << 3)
#define TAG_T (1 << 4)

static const Rule RULES[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class       instance    title       tags mask     isfloating   monitor */
	{ "firefox",   NULL,       NULL,       TAG_2,        0,           -1 },
	{ "discord",   NULL,       NULL,       TAG_3,        0,           -1 },
	{ "Telegram",  NULL,       NULL,       TAG_3,        0,           -1 },
};

/* layout(s) */
static const float mfact     = 0.5;  /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
static const int refreshrate = 60;  /* refresh rate (per second) for client move/resize */

static const Layout LAYOUTS[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define HYPER Mod4Mask|ControlMask|ShiftMask|Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "rofi", "-show", "run", "-monitor", dmenumon, "-matching", "fuzzy", "-sort", "-sorting-method", "fzf", NULL };
static const char *termcmd[]  = { "alatty", NULL };

static const Key KEYS[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_space,  spawn,          {.v = dmenucmd } },
	{ MODKEY,                       XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY|ControlMask|ShiftMask, XK_equal,  setmfact,       {.f = -0.04} },
	{ MODKEY|ControlMask|ShiftMask, XK_minus,  setmfact,       {.f = +0.04} },
	{ MODKEY,                       XK_Tab,    focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_q,      killclient,     {0} },
	{ MODKEY|ControlMask,           XK_f,      togglefloating, {0} },
	TAGKEYS(                        XK_1,                      TAG_1)
	TAGKEYS(                        XK_2,                      TAG_2)
	TAGKEYS(                        XK_3,                      TAG_3)
	TAGKEYS(                        XK_4,                      TAG_4)
	TAGKEYS(                        XK_0,                      TAG_T)
	{ HYPER,                        XK_q,      quit,           {.i = 0} },
	{ HYPER,                        XK_r,      quit,           {.i = 1} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button BUTTONS[] = {
	/* click                event mask      button          function        argument */
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
};
