/* -*- mode: linux-c -*-
 *
 *      Copyright (C) 2005-2006 Cyrill Gorcunov <gclion@mail.ru>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *      May God bless you!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include <X11/xpm.h>

#include "xonclock.h"
#include "parse.h"
#include "cache.h"
#include "placement.h"
#include "image.h"
#include "pmdesktop.h"

#include "loaders/png.h"
#include "loaders/tiff.h"
#include "loaders/jpeg.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * global variables
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

const char* short_options = "dhsv";
static struct option long_options[] = {
	{"skin",		1,	NULL,	'a'},
	{"position",		1,	NULL,	'b'},
	{"offset-v",		1,	NULL,	'c'},
	{"show-date",		0,	NULL,	'd'},
	{"hour-length",		1,	NULL,	'e'},
	{"hour-width",		1,	NULL,	'f'},
	{"minute-length",	1,	NULL,	'g'},
	{"help",		0,	NULL,	'h'}, /* have short opt */
	{"minute-width",	1,	NULL,	'i'},
	{"second-length",	1,	NULL,	'j'},
	{"second-width",	1,	NULL,	'k'},
	{"color-hour",		1,	NULL,	'l'},
	{"color-minute",	1,	NULL,	'm'},
	{"color-second",	1,	NULL,	'n'},
	{"sleep",		1,	NULL,	'o'},
	{"date-font",		1,	NULL,	'p'},
	{"date-color",		1,	NULL,	'q'},
	{"date-format",		1,	NULL,	'r'},
	{"show-seconds",	0,	NULL,	's'}, /* have short opt */
	{"offset-h",		1,	NULL,	't'},
	{"date-position",	1,	NULL,	'u'},
	{"version",		0,	NULL,	'v'}, /* have short opt */
	{"date-offset-v",	1,	NULL,	'w'},
	{"date-offset-h",	1,	NULL,	'x'},
	{"no-winredirect",	0,	NULL,	'y'},
	{"hands-rotation-axis",	1,	NULL,	'z'},
	{"top",			0,	NULL,	'A'},
	{"overlap",		1,	NULL,	'B'},
	{"date-clip-format",	1,	NULL,	'C'},
	{"movable",		0,	NULL,	'D'},
	{"use-background",	0,	NULL,	'E'},
	{"remerge-sleep",	1,	NULL,	'F'},
	{"tz-hour",		1,	NULL,	'G'},
	{"tz-min",		1,	NULL,	'H'},
	{"alpha-threshold", 	1,	NULL,	'I'},
	{"window-id", 	1,	NULL,	'W'},
	{NULL,			0,	NULL,	0}

};

/* set at configuraion time by configure script */
char *prog_name = PACKAGE_NAME;
char *prog_version = PACKAGE_VERSION;
char *own_skin = OWN_SKIN;

/* signal flags */
volatile sig_atomic_t sigtime = 0;
volatile sig_atomic_t sigtime_in_progress = 0;
volatile sig_atomic_t sigterm_in_progress = 0;
volatile sig_atomic_t sigterm_requested = 0;
volatile sig_atomic_t sigterm_reqcount = 0;
volatile sig_atomic_t sigint_in_progress = 0;
volatile sig_atomic_t expose_req = 0;

/* to setup timer */
struct itimerval itime = {};
struct itimerval otime = {};
sigset_t block_timer;

/* exposing thread */
void * expose_on_alarm(void *args);
pthread_t expose_thread_id;
volatile sig_atomic_t expose_thread_exit_req = 0;

Display *display = NULL;	/* main display */
Window window;			/* clock window (shaped) */
Visual *vis = NULL;		/* clock window visual */
int depth = 0;
Colormap cm;
XEvent x_event = {};
XClassHint *xo_main_hint = NULL;	/* windows managers
					 * may catch WM_CLASS
					 * property and do (or do not)
					 * something with the program */
unsigned char *skin_rgb = NULL;
unsigned char *skin_alpha = NULL;

unsigned char *rgb_merged = NULL;
unsigned char *alpha_merged = NULL;
unsigned char *bg_rgb = NULL;

/* clock skin and mask */
static Pixmap pix_skin, pix_mask;


/* to free allocated resource properly we set the appropriate
 * flag at a resource allocation */
static char sf_display = 0;
static char sf_window = 0;
static char sf_cache = 0;
static char sf_pix_skin = 0;
static char sf_pix_mask = 0;
static char sf_xo_main_hint = 0;
static char sf_skin_rgb = 0;
static char sf_skin_alpha = 0;
static char sf_rgb_merged = 0;
static char sf_alpha_merged = 0;
static char sf_bg_rgb = 0;
static char sf_thread = 0;

/* for clock window movement */
volatile sig_atomic_t left_button_pressed = 0;

/* to get an updated background */
unsigned long remerge_sleep = 5000;
volatile sig_atomic_t updating_bg = 0;

/* to customize time-zone */
static int tz_hour = 0;
static int tz_min = 0;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	main
 * $Desc:	program entry point
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 

int main(int argc, char * argv[])
{
	/* main work is doing in cache */
	xo_cache_t xo_cache = {};

	/* some X stuff */
	Screen *screen = NULL;
	int screen_num;
	XSetWindowAttributes wins_attribs = {};
	XWindowAttributes win_attribs = {};
	unsigned int width = 0, height = 0;	/* clock window sizes */
	unsigned long ul_flags = 0;
	GC gc;					/* working GC */
	Pixmap pix_composed;			/* composed clock (with
						 * clock hands) */

	/* clock hand color and geometry */
	clock_hand_t hand_sec = {};
	clock_hand_t hand_min = {};
	clock_hand_t hand_hour = {};

	/* for getopt_long */
	int next_option;
	int opt_index;

	/* skin file */
	char *skin_file_path = own_skin;

	/* show seconds hand flag */
	int show_seconds = 0;

	/* hand color components */
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;

	/* work with signals */
	struct sigaction sa = {};
	sigset_t sa_set;

	/* startup sleep time */
	unsigned int sleep_time = 0;

	/* clock position and offsets */
	int pos_x = 0, pos_y = 0;
	int offset_v = 0, offset_h = 0;
	int base_pos = BOTTOM_RIGHT;
	int dpy_width = 0, dpy_height = 0;
	int pos_right_max = 0, pos_top_max = 0;

	/* date string */
	int show_date = 0;
	date_str_t date_string = {};

	/* use CWOverrideRedirect */
	int no_winredirect = 0;

	/* hands rotation axis coords */
	XPoint hands_rot_axis = {};
	int use_hands_rot_axis = 0;
	int rotx, roty;
	char *token;

	int use_always_on_top = 0;
	int overlap = 0;

	char conf_path[PATH_LEN];
	int home_dir_len = 0;
	char *home_dir = NULL;
	Time prev_click_right = 0; /* for mouse clicks timing */
	Time prev_click_left = 0; /* for mouse clicks timing */
	int rc = 0;

	/* for X clipboard */
	XEvent respond;
	XSelectionRequestEvent *sel_req;
	Atom atom_clipboard, atom_compound_text, atom_targets;
	int atoms_list_cnt = 0;
	Atom atoms_list[1]; /* we work with one atom only */
	char *x_date_buf = NULL;
	int atoms_created = 0;
	char *date_clip_format = "%G-%m-%d %H:%M:%S"; /* see man strftime */

	int skin_fmt = IFMT_UNK;
	int skin_update_req = 0;

	/* to move the clock window by a mouse */
	int is_movable = 0;
	int pos_x1 = 0, pos_x2 = 0, pos_y1 = 0, pos_y2 = 0;
	int mv_delta_x = 0, mv_delta_y = 0;

	XImage *skin_image = NULL; /* to handle XPM files */

	/* external window parent, if required */
    Window win_id = NULL;

	/* use background root window */
	int use_background = 0;
	unsigned char *bg_rgb_prev = NULL;

	unsigned long ev_mask = 0;

	unsigned int alpha_threshold = 0;

	/* init clock hands by default (the length'll be set later) */
	hand_sec.color.red = 100;
	hand_sec.color.green = 150;
	hand_sec.color.blue = 200;
	hand_sec.color.alpha = 255;
	hand_sec.width = 1;

	hand_min.color.red = 40;
	hand_min.color.green = 40;
	hand_min.color.blue = 40;
	hand_min.color.alpha = 255;
	hand_min.width = 3;

	hand_hour.color.red = 40;
	hand_hour.color.green = 40;
	hand_hour.color.blue = 40;
	hand_hour.color.alpha = 255;
	hand_hour.width = 4;

	/* default date string values */
	date_string.color.red = 255;
	date_string.color.green = 10;
	date_string.color.blue = 10;
	date_string.color.alpha = 255;
	date_string.font_name = "fixed-8";
	date_string.pos = MIDDLE_CENTER;
	date_string.offset_v = 10;
	date_string.offset_h = 0;
	date_string.format = "%G-%m-%d"; /* see man strftime */

	/* read config file */
	xo_bzero(&conf_path[0], PATH_LEN);
	if ((home_dir = get_resolved_path("~/")) != NULL) {
		home_dir_len = strlen(home_dir);
		if (home_dir_len < PATH_LEN - RCFILE_LEN) {
			strcat(&conf_path[0], home_dir);
			if (home_dir[home_dir_len - 1] == '/') {
				strcat(&conf_path[0], RCFILE);
			} else {
				strcat(&conf_path[0], "/" RCFILE);
			}
			if (parse_config(conf_path) == 0) {
				/* walk thru all possib. options
				 * ignoring errors */
				get_opt_STRING(get_conf_opt("skin"),
					       &skin_file_path);
				if (skin_file_path) {
					skin_file_path =
						get_resolved_path(skin_file_path);
				}
				get_opt_POSITION(get_conf_opt("position"),
						 &base_pos);
				get_opt_NUMBER(get_conf_opt("offset-v"),
					       &offset_v);
				get_opt_NUMBER(get_conf_opt("offset-h"),
					       &offset_h);
				get_opt_ABSNUMBER(get_conf_opt("hour-length"),
						  &hand_hour.length);
				get_opt_ABSNUMBER(get_conf_opt("hour-width"),
						  &hand_hour.width);
				get_opt_ABSNUMBER(get_conf_opt("minute-length"),
						  &hand_min.length);
				get_opt_ABSNUMBER(get_conf_opt("minute-width"),
						  &hand_min.width);
				get_opt_ABSNUMBER(get_conf_opt("second-length"),
						  &hand_sec.length);
				get_opt_ABSNUMBER(get_conf_opt("second-width"),
						  &hand_sec.width);
				get_opt_COLORA(get_conf_opt("color-hour"),
					       &hand_hour.color.red,
					       &hand_hour.color.green,
					       &hand_hour.color.blue,
					       &hand_hour.color.alpha);
				get_opt_COLORA(get_conf_opt("color-minute"),
					       &hand_min.color.red,
					       &hand_min.color.green,
					       &hand_min.color.blue,
					       &hand_min.color.alpha);
				get_opt_COLORA(get_conf_opt("color-second"),
					       &hand_sec.color.red,
					       &hand_sec.color.green,
					       &hand_sec.color.blue,
					       &hand_sec.color.alpha);
				get_opt_ABSNUMBER(get_conf_opt("sleep"),
						  &sleep_time);
				get_opt_BOOL(get_conf_opt("show-seconds"),
					     &show_seconds);
				get_opt_STRING(get_conf_opt("date-font"),
					       &date_string.font_name);
				get_opt_COLORA(get_conf_opt("date-color"),
					       &date_string.color.red,
					       &date_string.color.green,
					       &date_string.color.blue,
					       &date_string.color.alpha);
				get_opt_STRING(get_conf_opt("date-format"),
					       &date_string.format);
				get_opt_BOOL(get_conf_opt("show-date"),
					     &show_date);
				get_opt_POSITION(get_conf_opt("date-position"),
						 &date_string.pos);
				get_opt_NUMBER(get_conf_opt("date-offset-v"),
					       &date_string.offset_v);
				get_opt_NUMBER(get_conf_opt("date-offset-h"),
					       &date_string.offset_h);
				get_opt_BOOL(get_conf_opt("no-winredirect"),
					     &no_winredirect);
				rc = get_opt_COORDS(get_conf_opt("hands-rotation-axis"),
						    &rotx, &roty);
				if (rc == 0) {
					use_hands_rot_axis = 1;
					hands_rot_axis.x = (short)rotx;
					hands_rot_axis.y = (short)roty;
				}
				get_opt_BOOL(get_conf_opt("top"),
					     &use_always_on_top);
				get_opt_ABSNUMBER(get_conf_opt("overlap"),
						  &overlap);
				get_opt_STRING(get_conf_opt("date-clip-format"),
					       &date_clip_format);
				get_opt_BOOL(get_conf_opt("movable"),
					     &is_movable);
				get_opt_BOOL(get_conf_opt("use-background"),
					     &use_background);
				get_opt_NUMBER_LONG(get_conf_opt("remerge-sleep"),
						    &remerge_sleep);
				get_opt_NUMBER(get_conf_opt("tz-hour"),
					       &tz_hour);
				get_opt_NUMBER(get_conf_opt("tz-min"),
					       &tz_min);
				get_opt_ABSNUMBER(get_conf_opt("alpha-threshold"),
						  &alpha_threshold);
			}
		}
	} /* config file */

	/*
	 * command line arguments parsing
	 */
	do {
		next_option = getopt_long(argc, argv,
					  short_options,
					  long_options,
					  &opt_index);
		if (next_option == -1)
			break;
		switch(next_option){
		case 'a': /* --skin */
			skin_file_path = get_resolved_path(optarg);
			break;
		case 'b': /* --position */
			/* see enum placement in placement.h */
			if (strcmp(optarg, "TOP-LEFT") == 0) {
				base_pos = TOP_LEFT;
			} else if (strcmp(optarg, "TOP-CENTER") == 0) {
				base_pos = TOP_CENTER;
			} else if (strcmp(optarg, "TOP-RIGHT") == 0) {
				base_pos = TOP_RIGHT;
			} else if (strcmp(optarg, "MIDDLE-LEFT") == 0) {
				base_pos = MIDDLE_LEFT;
			} else if (strcmp(optarg, "MIDDLE-CENTER") == 0) {
				base_pos = MIDDLE_CENTER;
			} else if (strcmp(optarg, "MIDDLE-RIGHT") == 0) {
				base_pos = MIDDLE_RIGHT;
			} else if (strcmp(optarg, "BOTTOM-LEFT") == 0) {
				base_pos = BOTTOM_LEFT;
			} else if (strcmp(optarg, "BOTTOM-CENTER") == 0) {
				base_pos = BOTTOM_CENTER;
			} else if (strcmp(optarg, "BOTTOM-RIGHT") == 0) {
				base_pos = BOTTOM_RIGHT;
			} else {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'c': /* --offset-v */
			offset_v = atoi(optarg);
			break;
		case 't': /* --offset-h */
			offset_h = atoi(optarg);
			break;
		case 'e': /* --hour-length */
			hand_hour.length = abs(atoi(optarg));
			if (hand_hour.length < 1) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'f': /* --hour-width */
			hand_hour.width = abs(atoi(optarg));
			if (hand_hour.width < 1) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'g': /* --minute-length */
			hand_min.length = abs(atoi(optarg));
			if (hand_min.length < 1) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'i': /* --minute-width */
			hand_min.width = abs(atoi(optarg));
			if (hand_min.width < 1) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'j': /* --second-length */
			hand_sec.length = abs(atoi(optarg));
			if (hand_sec.length < 1) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'k': /* --second-width */
			hand_sec.width = abs(atoi(optarg));
			if (hand_sec.width < 1) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'l': /* --color-hour */
		case 'm': /* --color-minute */
		case 'n': /* --color-second */
			/* trying to parse -> see below */
			if (parse_rgba_string(optarg,
					      &red,
					      &green,
					      &blue,
					      &alpha) != 0) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			if (next_option == 'l') { /* --color-hour */
				hand_hour.color.red = red;
				hand_hour.color.green = green;
				hand_hour.color.blue = blue;
				hand_hour.color.alpha = alpha;
			} else if (next_option == 'm') { /* --color-minute */
				hand_min.color.red = red;
				hand_min.color.green = green;
				hand_min.color.blue = blue;
				hand_min.color.alpha = alpha;
			} else { /* --color-second */
				hand_sec.color.red = red;
				hand_sec.color.green = green;
				hand_sec.color.blue = blue;
				hand_sec.color.alpha = alpha;
			}
			break;
		case 'o': /* --sleep */
			sleep_time = abs(atoi(optarg));
			break;
		case 'v': /* -v or --version */
			print_version();
			xonclock_clear();
			exit(0);
			break;
		case 'h': /* -h or --help */
			/* TODO: print a normal help */
			printf("USAGE: %s [OPTIONS]\n", prog_name);
			printf("The options described in %s man page.\n",
			       prog_name);
			xonclock_clear();
			exit(0);
			break;
		case 's': /* -s or --show-seconds */
			show_seconds = 1;
			break;
		case 'p': /* --date-font */
			date_string.font_name = optarg;
			break;
		case 'q': /* --date-color */
			/* trying to parse -> see below */
			if (parse_rgba_string(optarg,
					      &red,
					      &green,
					      &blue,
					      &alpha) != 0) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			date_string.color.red = red;
			date_string.color.green = green;
			date_string.color.blue = blue;
			date_string.color.alpha = alpha;
			break;
		case 'r': /* --date-format */
			date_string.format = optarg;
			break;
		case 'd': /* -d or --show-date */
			show_date = 1;
			break;
		case 'u': /* --date-position */
			/* see enum placement in placement.h */
			if (strcmp(optarg, "TOP-LEFT") == 0) {
				date_string.pos = TOP_LEFT;
			} else if (strcmp(optarg, "TOP-CENTER") == 0) {
				date_string.pos = TOP_CENTER;
			} else if (strcmp(optarg, "TOP-RIGHT") == 0) {
				date_string.pos = TOP_RIGHT;
			} else if (strcmp(optarg, "MIDDLE-LEFT") == 0) {
				date_string.pos = MIDDLE_LEFT;
			} else if (strcmp(optarg, "MIDDLE-CENTER") == 0) {
				date_string.pos = MIDDLE_CENTER;
			} else if (strcmp(optarg, "MIDDLE-RIGHT") == 0) {
				date_string.pos = MIDDLE_RIGHT;
			} else if (strcmp(optarg, "BOTTOM-LEFT") == 0) {
				date_string.pos = BOTTOM_LEFT;
			} else if (strcmp(optarg, "BOTTOM-CENTER") == 0) {
				date_string.pos = BOTTOM_CENTER;
			} else if (strcmp(optarg, "BOTTOM-RIGHT") == 0) {
				date_string.pos = BOTTOM_RIGHT;
			} else {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			break;
		case 'w': /* --date-offset-v */
			date_string.offset_v = atoi(optarg);
			break;
		case 'x': /* --date-offset-h */
			date_string.offset_h = atoi(optarg);
			break;
		case 'y': /* --no-winredirect */
			no_winredirect = 1;
			break;
		case 'z': /* --hands-rotation-axis */
			token = strtok(optarg, "/");
			if (!token) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			hands_rot_axis.x = atoi(token);
			token = strtok(NULL, "/");
			if (!token) {
				msg("Bad argument \"%s\" "
				    "at \"--%s\" option",
				    optarg,
				    long_options[opt_index].name);
				xonclock_clear();
				exit(1);
			}
			hands_rot_axis.y = atoi(token);
			use_hands_rot_axis = 1;
			break;
		case 'A': /* --top */
			use_always_on_top = 1;
			break;
		case 'B': /* --overlap */
			overlap = abs(atoi(optarg));
			break;
		case 'C': /* --date-clip-format */
			date_clip_format = optarg;
			break;
		case 'D': /* --movable */
			is_movable = 1;
			break;
		case 'E': /* --use-background */
			use_background = 1;
			break;
		case 'F': /* --remerge-sleep */
			remerge_sleep = atol(optarg);
			break;
		case 'G': /* --tz-hour */
			tz_hour = atoi(optarg);
			break;
		case 'H': /* --tz-min */
			tz_min = atoi(optarg);
			break;
		case 'I': /* alpha-threshold */
			alpha_threshold = abs(atoi(optarg));
			break;
		case 'W': /* --window-id */
			win_id = strtol(optarg, 0, 0);
		    //msg("external window id : %s",optarg);
			break;
		} /* switch(next_option) ignoring invalid options */
	} while(next_option != -1);

	/* startup sleep */
	if (sleep_time)
		sleep(sleep_time);

	/* open main display */
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		msg("Unable to open X display");
		xonclock_clear();
		exit(1);
	}
	XO_FLAG_ON(sf_display);

	/* screen and other info */
	screen_num = DefaultScreen(display);
	screen = ScreenOfDisplay(display, screen_num);
	if (screen == NULL) {
		msg("Unable to get screen");
		xonclock_clear();
		exit(1);
	}
	vis = DefaultVisual(display, screen_num);
	depth = DefaultDepth(display, screen_num);
	cm = DefaultColormap(display, screen_num);

	/* get skin file type */
	switch((skin_fmt = get_image_type_from_ext(skin_file_path))) {
	case IFMT_PNG:
		if (!read_png(skin_file_path,
			      &width, &height,
			      &skin_rgb, &skin_alpha)) {
			msg("Error occured while loading a skin file: %s",
			    skin_file_path);
			xonclock_clear();
			exit(1);
		}
		XO_FLAG_ON(sf_skin_rgb);
		XO_FLAG_ON(sf_skin_alpha);
		break;
	case IFMT_XPM:
		/* we load our skin to get its width and height */
		rc = XpmReadFileToImage(display, skin_file_path,
					&skin_image,
					NULL, NULL);
		if (rc != XpmSuccess) {
			msg("Error occured while loading a skin file: %s",
			    skin_file_path);
			xonclock_clear();
			exit(1);
		}
		width = skin_image->width;
		height = skin_image->height;
		XDestroyImage(skin_image);
		break;
	case IFMT_TIF:
		if (!read_tiff(skin_file_path,
			       &width, &height,
			       &skin_rgb, &skin_alpha)) {
			msg("Error occured while loading a skin file: %s",
			    skin_file_path);
			xonclock_clear();
			exit(1);
		}
		XO_FLAG_ON(sf_skin_rgb);
		XO_FLAG_ON(sf_skin_alpha);
		break;
	case IFMT_JPG:
		if (!read_jpeg(skin_file_path,
			       &width, &height,
			       &skin_rgb, &skin_alpha)) {
			msg("Error occured while loading a skin file: %s",
			    skin_file_path);
			xonclock_clear();
			exit(1);
		}
		XO_FLAG_ON(sf_skin_rgb);
		XO_FLAG_ON(sf_skin_alpha);
		break;
	default:
		msg("Unrecognized skin file: %s",
		    skin_file_path);
		xonclock_clear();
		exit(1);
		break;
	}

	/* clock skin geometry */
	if (width < MIN_CLOCK_WIDTH || height < MIN_CLOCK_HEIGHT) {
		msg("Bad skin image size");
		xonclock_clear();
		exit(1);
	}

	/* check for rotation axis */
	if (use_hands_rot_axis) {
		if (hands_rot_axis.x > width ||
		    hands_rot_axis.y > height) {
			msg("Clock hands rotation axis coordinates "
				"exceeds clock skin sizes");
			xonclock_clear(); /* -> below */
			exit(1);
		}
	}

	/* make default hands length (if needed) */
	if (width < height) {
		if (hand_sec.length <= 1)
			hand_sec.length = width / 2 - width / 6;
		if (hand_min.length <= 1)
			hand_min.length = width / 2 - width / 5;
		if (hand_hour.length <= 1)
			hand_hour.length = width / 2 - width / 4;
	} else {
		if (hand_sec.length <= 1)
			hand_sec.length = height / 2 - height / 6;
		if (hand_min.length <= 1)
			hand_min.length = height / 2 - height / 5;
		if (hand_hour.length <= 1)
			hand_hour.length = height / 2 - height / 4;
	}

	/* check for hands overlaps */
	if (overlap > hand_sec.length || overlap == 0) {
		hand_sec.overlap = hand_sec.length / 5;
	} else {
		hand_sec.overlap = overlap;
	}
	if (overlap > hand_min.length || overlap == 0) {
		hand_min.overlap = hand_min.length / 5;
	} else {
		hand_min.overlap = overlap;
	}
	if (overlap > hand_hour.length || overlap == 0) {
		hand_hour.overlap = hand_hour.length / 5;
	} else {
		hand_hour.overlap = overlap;
	}
	/* now normalize them */
	overlap = hand_sec.overlap;
	if (overlap < hand_min.overlap)
		overlap = hand_min.overlap;
	if (overlap < hand_hour.overlap)
		overlap = hand_hour.overlap;
	hand_sec.overlap = overlap;
	hand_min.overlap = overlap;
	hand_hour.overlap = overlap;

	dpy_width = DisplayWidth(display, screen_num);
	dpy_height = DisplayHeight(display, screen_num);
	pos_right_max = dpy_width - width;
	pos_top_max = dpy_height - height;

	/* compute clock placement -> placement.c */
	if (get_clock_placement(dpy_width,
				dpy_height,
				width, height, base_pos,
				offset_h, offset_v,
				&pos_x, &pos_y) == -1) {
		msg("Unable to get clock position");
		xonclock_clear();
		exit(1);
	}

	/* make the clock window */
	ul_flags = CWBackPixmap;
	if (!no_winredirect) /* to work with WindowManager */
		ul_flags |= CWOverrideRedirect;
	wins_attribs.override_redirect = True;
	wins_attribs.background_pixmap = ParentRelative;

	/** This is an experimental patch by Patriot, Mar 2009.
		win_id points to a user supplied window-ID. This allows
		xonclock to create a child window of any valid displayed
		window. This patch was intended to overcome ROX-desktop
		limitation. */
		
	/* Warning: win_id is NOT internally validated as this is considered
				to be an experimental hack. That's on KIV list. */

	if (!win_id) /* if not specified then use RootWindow */
	  //win_id = RootWindow(display, screen_num);
	  
	  /* see pmdesktop.c */
	  win_id = find_root(display, screen_num, dpy_width, dpy_height);

	window = XCreateWindow(display,
			       win_id, //RootWindow(display, screen_num),
			       pos_x, pos_y,
			       width, height,
			       0, DefaultDepth(display, screen_num),
			       InputOutput,
			       CopyFromParent,
			       ul_flags, &wins_attribs);
	XGetWindowAttributes(display, window, &win_attribs);

	/* make clock skin pixmap and its mask */
	if (skin_fmt == IFMT_XPM) {
		if (XpmReadFileToPixmap(display,
					window,
					skin_file_path,
					&pix_skin,
					&pix_mask, NULL) != XpmSuccess) {
			msg("Unable to create image and mask"
			    " Pixmaps from a skin");
			xonclock_clear();
			exit(1);
		}
		XO_FLAG_ON(sf_pix_skin);
		XO_FLAG_ON(sf_pix_mask);
		if (use_background) { /* --use-background is not allowed with XPM */
			use_background = 0;
		}
	} else {
		gc = DefaultGC(display, screen_num); /* temporary used */
		if (!use_background) {
			if (!pixmap_from_rgb(display, &window,
					     &gc, vis, depth,
					     &skin_rgb[0],
					     width, height,
					     &pix_skin)) {
				msg("Unable to create image "
				    "Pixmap from a skin");
				xonclock_clear();
				exit(1);
			}
		} else {	/* we use background */
			bg_rgb = get_dpy_bg_rgb(display,
						NULL,
						pos_x, pos_y,
						width, height);
			if (!bg_rgb) {
				msg("Unable to create background data");
				xonclock_clear();
				exit(1);
			}
			XO_FLAG_ON(sf_bg_rgb);
			rgb_merged = calloc(3, width * height);
			if (!rgb_merged) {
				msg("No free memory");
				xonclock_clear();
				exit(1);
			}
			XO_FLAG_ON(sf_rgb_merged);
			alpha_merged = calloc(1, width * height);
			if (!rgb_merged) {
				msg("No free memory");
				xonclock_clear();
				exit(1);
			}
			XO_FLAG_ON(sf_alpha_merged);
			if (!merge_rgba_aa(bg_rgb,
					   skin_rgb,
					   NULL,
					   skin_alpha,
					   rgb_merged,
					   alpha_merged,
					   width, height)) {
				msg("Unable to merge skin and background");
				xonclock_clear();
				exit(1);
			}
			if (!pixmap_from_rgb(display, &window,
					     &gc, vis, depth,
					     &rgb_merged[0],
					     width, height,
					     &pix_skin)) {
				msg("Unable to create image Pixmap"
				    " from a skin");
				xonclock_clear();
				exit(1);
			}
		}
		XO_FLAG_ON(sf_pix_skin);
		if (!mask_from_alpha(display, &window,
				     vis, depth,
				     &skin_alpha[0], width, height,
				     alpha_threshold,
				     &pix_mask)) {
			msg("Unable to create mask Pixmap from a skin");
			xonclock_clear();
			exit(1);
		}
		XO_FLAG_ON(sf_pix_mask);
	}

	/* just shaping for clock without background */
	if (!use_background) {
		XShapeCombineMask(display, window,
				  ShapeBounding, 0, 0,
				  pix_mask, ShapeSet);
		/* we do not need our RGB and ALPHA skin data
		 * for non background clock */
		free(skin_rgb);
		XO_FLAG_OFF(sf_skin_rgb);
		free(skin_alpha);
		XO_FLAG_OFF(sf_skin_alpha);
	}
	XSetWindowBackgroundPixmap(display, window, ParentRelative);
	XO_FLAG_ON(sf_window);


	/* setup the cache */
	xo_cache.display = display;
	xo_cache.window = &window;
	xo_cache.pix_skin = &pix_skin;
	if (use_background)
		xo_cache.pix_mask = NULL;
	else
		xo_cache.pix_mask = &pix_mask;
	xo_cache.hand_hour = &hand_hour;
	xo_cache.hand_min = &hand_min;
	xo_cache.hand_sec = &hand_sec;
	xo_cache.flags = XO_CACHE_MINUTE | XO_CACHE_HOUR;
	if (show_seconds)
		xo_cache.flags |= XO_CACHE_SECOND;
	if (show_date) {
		xo_cache.date_string = &date_string;
	} else {
		xo_cache.date_string = NULL;
	}
	if (use_hands_rot_axis) {
		xo_cache.flags |= XO_CACHE_ROT_AXIS;
		xo_cache.hand_rot_axis = &hands_rot_axis;
	}
	if (!use_background) {
		xo_cache.flags |= XO_CACHE_USE_MASK;
	}
	if (tz_hour < -24)
		tz_hour = -24;
	else if (tz_hour > 24)
		tz_hour = 24;
	xo_cache.tz_hour = tz_hour;
	if (tz_min < -60)
		tz_min = -60;
	else if (tz_min > 60)
		tz_min = 60;
	xo_cache.tz_min = tz_min;
	if (cache_init(&xo_cache) == -1) {
		msg("Cache initialization failed");
		xonclock_clear();
		exit(1);
	}
	XO_FLAG_ON(sf_cache);

	/* save GC */
	cache_get_gc(&gc);

	/* check for signal */
	if (sigterm_requested == 1) {
		xonclock_clear();
		exit(1);
	}

	/* allocate memory for hint */
	xo_main_hint = XAllocClassHint();
	if (xo_main_hint == NULL) {
		msg("No free memory to allocate XClassHint structure");
		xonclock_clear();
		exit(1);
	}
	XO_FLAG_ON(sf_xo_main_hint);

	/* set class properties */
	xo_main_hint->res_name = prog_name;
	xo_main_hint->res_class = "Xonclock";
	XSetClassHint(display, window, xo_main_hint);

	/* setup main clock window */
	ev_mask = ExposureMask | ButtonPressMask |
		PropertyChangeMask | MotionNotify |
		ButtonReleaseMask | ButtonMotionMask;
	XSelectInput(display, window, ev_mask);

	/* to use X clipboard */
	atom_clipboard = XInternAtom(display, "CLIPBOARD", False);
	atom_compound_text = XInternAtom(display, "COMPOUND_TEXT", False);
	atom_targets = XInternAtom(display, "TARGETS", False);
	if (atom_clipboard == None ||
	    atom_compound_text == None ||
	    atom_targets == None) {
		msg("Unable to create atoms to use X clipboard");
		XO_FLAG_OFF(atoms_created);
	} else {
		XO_FLAG_ON(atoms_created);
		/* we work with COMPOUND_TEXT only */
		atoms_list_cnt = 1;
		atoms_list[0] = atom_compound_text;
	}

	/* setup timer signal handler */
	xo_bzero(&sa, sizeof(struct sigaction));
	sigemptyset(&sa_set);
	sa.sa_handler = &sig_time;
	sa.sa_flags = SA_NOMASK;
	sa.sa_mask = sa_set;
	sigaction(SIGALRM, &sa, NULL);

	if (pthread_create(&expose_thread_id, NULL, &expose_on_alarm, NULL)) {
		msg("Unable to create a new thread");
		xonclock_clear();
		exit(1);
	}
	XO_FLAG_ON(sf_thread);

	/* setup termination signal handlers */
	xo_bzero(&sa, sizeof(struct sigaction));
	sigemptyset(&sa_set);
	sa.sa_handler = &sig_term;
	sa.sa_flags = SA_ONESHOT;
	sa.sa_mask = sa_set;
	sigaction(SIGTERM, &sa, NULL);
	xo_bzero(&sa, sizeof(struct sigaction));
	sigemptyset(&sa_set);
	sa.sa_handler = &sig_int;
	sa.sa_flags = SA_ONESHOT;
	sa.sa_mask = sa_set;
	sigaction(SIGINT, &sa, NULL);

	/* to block alarm signal race */
	sigemptyset(&block_timer);
	sigaddset(&block_timer, SIGALRM);

	/* setup update timer interval */
	itime.it_value.tv_sec = 1;
	itime.it_value.tv_usec = 0;
	itime.it_interval.tv_sec = 0;
	itime.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &itime, &otime) == -1) {
		msg("Unable to setup update timer");
		xonclock_clear();
		exit(1);
	}


	/* show the clock */
	XMapWindow(display, window);
	if (use_always_on_top != 1) {
		XLowerWindow(display, window); /* bring to back order */
	}

	XFlush(display);

	/*
	 * main clock window event loop
	 */
	while(1) {
		XNextEvent(display, &x_event);
		if (sigterm_requested == 1) {
			xonclock_clear();
			exit(0);
		}
		switch (x_event.type) {
		case Expose:
		if (x_event.xexpose.count == 0 ||
			    sigtime == 1) {
			if (sigterm_requested == 1) {
				xonclock_clear();
				exit(0);
			}
			if (cache_get_composed(&pix_composed,
					       sigtime) == -1) {
				msg("Error occured while "
				    "compositing the clock");
				xonclock_clear();
				exit(1);
			}
			/* copy composed pixmap in our clock window */
			XCopyArea(display, pix_composed,
				  window, gc,
				  0, 0, width, height,
				  0, 0);
			if (use_always_on_top == 1) {
				XRaiseWindow(display, window);
			}
			if (sigtime == 1)
				XO_FLAG_OFF(sigtime);
		}
		break;
		case SelectionRequest:
			sel_req = &(x_event.xselectionrequest);
			if (atoms_created == 1) {
				if (sel_req->target == atom_compound_text) {
					/* get date string -> cache.c */
					x_date_buf = cache_x_date_buf_op(0, date_clip_format);
					if (x_date_buf == NULL) {
						msg("x_date_buf == NULL");
						respond.xselection.property = None;
					} else {
						XChangeProperty(display,
								sel_req->requestor,
								sel_req->property,
								atom_compound_text,
								8,
								PropModeReplace,
								(unsigned char*) x_date_buf,
								strlen(x_date_buf));
						respond.xselection.property = sel_req->property;
					}
				} else if (sel_req->target == atom_targets) {
					XChangeProperty(display,
							sel_req->requestor,
							sel_req->property,
							sel_req->target,
							32,
							PropModeReplace,
							(unsigned char*)&atoms_list[0],
							atoms_list_cnt);
				} else {
					respond.xselection.property = None;
				}
			} else {
				respond.xselection.property = None;
			}
			respond.xselection.type = SelectionNotify;
			respond.xselection.display = sel_req->display;
			respond.xselection.requestor = sel_req->requestor;
			respond.xselection.selection = sel_req->selection;
			respond.xselection.target = sel_req->target;
			respond.xselection.time = sel_req->time;
			XSendEvent (display, sel_req->requestor,
				    0, 0, &respond);
			XFlush(display);
			break; /* SelectionRequest */
		case ButtonRelease:
			if (left_button_pressed == 1 &&
			    is_movable == 1) {
				pos_x2 = x_event.xbutton.x;
				pos_y2 = x_event.xbutton.y;
				mv_delta_x = pos_x2 - pos_x1;
				mv_delta_y = pos_y2 - pos_y1;
				if (mv_delta_x != 0 || mv_delta_y != 0) {
					pos_x += mv_delta_x;
					pos_y += mv_delta_y;
					if (pos_x > pos_right_max) {
						pos_x = pos_right_max;
					} else {
						if (pos_x <= 0)
							pos_x = 0;
					}
					if (pos_y > pos_top_max) {
						pos_y = pos_top_max;
					} else {
						if (pos_y <= 0)
							pos_y = 0;
					}
					XMoveWindow(display, window,
						    pos_x, pos_y);
				}
				left_button_pressed = 0;
			}
			if (!use_background || !skin_update_req)
				break;
			/*
			 * new background
			 */
			updating_bg = 1;
			XUnmapWindow(display, window);
			XFlush(display);
			usleep(remerge_sleep);
			bg_rgb_prev = bg_rgb;
			bg_rgb = get_dpy_bg_rgb(display, bg_rgb,
						pos_x, pos_y,
						width, height);
			if (!bg_rgb) {
				msg("Unable to get background data");
				bg_rgb = bg_rgb_prev;
				xonclock_clear();
				exit(1);
			}
			XFreePixmap(display, pix_skin);
			XMapWindow(display, window);
			XO_FLAG_OFF(sf_pix_skin);
			if (!merge_rgba_aa(bg_rgb, skin_rgb,
					   NULL, skin_alpha,
					   rgb_merged, alpha_merged,
					   width, height)) {
				msg("Unable to merge skin and "
				    "background");
				xonclock_clear();
				exit(1);
			}
			if (!pixmap_from_rgb(display, &window,
					     &gc, vis, depth,
					     &rgb_merged[0],
					     width, height,
					     &pix_skin)) {
				msg("Unable to create clock Pixmap");
				xonclock_clear();
				exit(1);
			}
			XO_FLAG_ON(sf_pix_skin);
			if (!cache_update_skin(&pix_skin,
					       width, height)) {
				msg("Unable to update the cache");
				xonclock_clear();
				exit(1);
			}
			updating_bg = 0;
			skin_update_req = 0;
			break;
		case MotionNotify:
			/* move clock window */
			if (left_button_pressed  == 1 &&
			    is_movable == 1) {
				pos_x2 = x_event.xmotion.x;
				pos_y2 = x_event.xmotion.y;
				mv_delta_x = pos_x2 - pos_x1;
				mv_delta_y = pos_y2 - pos_y1;
				if (mv_delta_x != 0 || mv_delta_y != 0) {
					pos_x += mv_delta_x;
					pos_y += mv_delta_y;
					/* cut of the root window overlapping */
					if (pos_x > pos_right_max) {
						pos_x = pos_right_max;
					} else {
						if (pos_x <= 0)
							pos_x = 0;
					}
					if (pos_y > pos_top_max) {
						pos_y = pos_top_max;
					} else {
						if (pos_y <= 0)
							pos_y = 0;
					}
					XMoveWindow(display, window,
						    pos_x, pos_y);
					skin_update_req = 1; /* we set it
							      * on every
							      * MotionNotify */
				}
			}
			break;
		case ButtonPress:
			switch(x_event.xbutton.button) {
			case Button1: /* left button */
				if (atoms_created == 1) {
					/* update a date string to be pasted -> cache.c */
					cache_x_date_buf_op(1, date_clip_format);
					XSetSelectionOwner(display,
							   atom_clipboard,
							   window,
							   CurrentTime);
				}
				if (is_movable == 1) {
					left_button_pressed = 1;
					pos_x1 = x_event.xbutton.x;
					pos_y1 = x_event.xbutton.y;
				}
				if (prev_click_left != 0) {
					if ((x_event.xbutton.time -
					     prev_click_left) <= DBLCLICKTIME) {
						if (use_background) {
							skin_update_req = 1; /* it'll be handled later */
						}
						prev_click_left = 0;
					} else {
						prev_click_left = x_event.xbutton.time;
					}
				} else {
					prev_click_left = x_event.xbutton.time;
				}
				break;
			case Button3: /* right button */
				if (prev_click_right != 0) {
					if ((x_event.xbutton.time -
					     prev_click_right) <= DBLCLICKTIME) {
						sigterm_requested = 1; /* it'll be handled later */
						prev_click_right = 0;
					} else {
						prev_click_right = x_event.xbutton.time;
					}
				} else {
					prev_click_right = x_event.xbutton.time;
				}
				break;
			default:
				prev_click_left = 0;
				prev_click_right = 0;
				break;
			}
			break; /* ButtonPress */
		default: /* do nothing */
			break;
		} /* switch (x_event.type) */
	} /* main clock window event loop */

	/* free resources */
	xonclock_clear();

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	parse_rgba_string
 * $Desc:	parses color string which should be 'R/G/B/ALFA' formatted
 * $Args:	(i) stream - string to be parsed (it'll be changed. don't
 *		use it after)
 *		(o) red, green, blue, alpha - color components
 * $Ret:	-1 - bad format, 0 - all OK
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int parse_rgba_string(char *stream,
		      unsigned char *red,
		      unsigned char *green,
		      unsigned char *blue,
		      unsigned char *alpha)
{
	char *tok_red = NULL;
	char *tok_green = NULL;
	char *tok_blue = NULL;
	char *tok_alpha = NULL;
	unsigned int ired, igreen, iblue, ialpha;

	tok_red = strtok(stream, "/");
	tok_green = strtok(NULL, "/");
	tok_blue = strtok(NULL, "/");
	tok_alpha = strtok(NULL, "/");

	/* check for bad format */
	if (tok_red == NULL	||
	    tok_green == NULL	||
	    tok_blue == NULL	||
	    tok_alpha == NULL) {
		return -1;
	}

	ired = (unsigned int)atoi(tok_red);
	igreen = (unsigned int)atoi(tok_green);
	iblue = (unsigned int)atoi(tok_blue);
	ialpha = (unsigned int)atoi(tok_alpha);

	if (ired > 255) ired = 255;
	if (igreen > 255) igreen = 255;
	if (iblue > 255) iblue = 255;
	if (ialpha > 255) ialpha = 255;

	if (red != NULL)
		*red = (unsigned char)ired;
	if (green != NULL)
		*green = (unsigned char)igreen;
	if (blue != NULL)
		*blue = (unsigned char)iblue;
	if (alpha != NULL)
		*alpha = (unsigned char)ialpha;

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	sig_time
 * $Desc:	handles SIGTIME signal
 * $Args:	(i) signum - signal number
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void sig_time(int signum)
{
	sigprocmask(SIG_BLOCK, &block_timer, NULL);
	if (!sigtime_in_progress && sf_window && sf_display) {
		XO_FLAG_ON(sigtime_in_progress);
		XO_FLAG_ON(sigtime);
		itime.it_value.tv_sec = 1;
		itime.it_value.tv_usec = 0;
		itime.it_interval.tv_sec = 0;
		itime.it_interval.tv_usec = 0;
		if (setitimer(ITIMER_REAL, &itime, &otime) == -1) {
			xonclock_clear();
			exit(1);
		}
		expose_req = 1;	/* just set it every time */
		XO_FLAG_OFF(sigtime_in_progress);
	}
	sigprocmask(SIG_UNBLOCK, &block_timer, NULL);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	sig_term
 * $Desc:	handles SIGTERM signal
 * $Args:	(i) signum - signal number
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void sig_term(int signum)
{
	if (!sigterm_in_progress) {
		XO_FLAG_ON(sigterm_in_progress);
		if (sigterm_reqcount >= 3) {
			msg("SIGTERM signal was captured 3 times."
				" Forse exit");
			xonclock_clear();
			exit(1);
		}
		if (cache_is_busy() == 0) {
			xonclock_clear();
			exit(0);
		} else {
			/* just set a flag and hope that it will be
			 * processed in main window loop (above) */
			XO_FLAG_ON(sigterm_requested);
		}
		XO_FLAG_OFF(sigterm_in_progress);
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	sig_int
 * $Desc:	handles SIGINT signal
 * $Args:	(i) signum - signal number
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void sig_int(int signum)
{
	if (!sigterm_in_progress) {
		if (!sigint_in_progress) {
			if (kill(getpid(), SIGTERM) == -1) {
				msg("Unable to send SIGTERM.");
			}
			XO_FLAG_OFF(sigint_in_progress);
		}
	}
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	xonclock_clear
 * $Desc:	frees allocated resources
 * $Comments:	it checks flags to find which resource is need to be
 *		freed
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void xonclock_clear(void)
{
	void *retval = NULL;

	if (sf_cache == 1) {
		cache_free(); /* -> cache.c */
		XO_FLAG_OFF(sf_cache);
	}
	if (sf_xo_main_hint == 1) {
		XFree(xo_main_hint);
		XO_FLAG_OFF(sf_xo_main_hint);
	}
	if (sf_window == 1) {
		XDestroyWindow(display, window);
		XO_FLAG_OFF(sf_window);
	}
	if (sf_pix_skin) {
		XFreePixmap(display, pix_skin);
		XO_FLAG_OFF(sf_pix_skin);
	}
	if (sf_pix_mask) {
		XFreePixmap(display, pix_mask);
		XO_FLAG_OFF(sf_pix_skin);
	}
	if (sf_skin_rgb) {
		free(skin_rgb);
		XO_FLAG_OFF(sf_skin_rgb);
	}
	if (sf_skin_alpha) {
		free(skin_alpha);
		XO_FLAG_OFF(sf_skin_alpha);
	}
	if (sf_rgb_merged) {
		free(rgb_merged);
		XO_FLAG_OFF(sf_rgb_merged);
	}
	if (sf_alpha_merged) {
		free(alpha_merged);
		XO_FLAG_OFF(sf_alpha_merged);
	}
	if (sf_bg_rgb) {
		free(bg_rgb);
		XO_FLAG_OFF(sf_bg_rgb);
	}
	if (sf_display == 1) {
		XCloseDisplay(display);
		XO_FLAG_OFF(sf_display);
	}
	if (sf_thread) {
		XO_FLAG_ON(expose_thread_exit_req);
		pthread_join(expose_thread_id, &retval);
		XO_FLAG_OFF(sf_thread);
	}
	free_opt_buf();
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	print_version
 * $Desc:	just print program version
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void print_version(void)
{
	printf("%s-%s\n", prog_name, prog_version);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * $Name:	expose_on_alarm
 * $Desc:	that is a separate thread that waits for signal and
 *		then just sends Expose event to redraw the clock
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void * expose_on_alarm(void *args)
{
	struct timespec req;
	XExposeEvent x_ev_exp = {};

	req.tv_sec = 0, req.tv_nsec = 150000000;
	x_ev_exp.type = Expose;
	x_ev_exp.display = display;
	x_ev_exp.send_event = True;

	while(1) {
		if (expose_thread_exit_req)
			return NULL;
		nanosleep(&req, NULL);
		if (expose_req && !updating_bg) {
			XSendEvent(display, window, False,
				   ExposureMask, (XEvent*)&x_ev_exp);
			XFlush(display);
			expose_req = 0;
		}
	}
}
