/* See LICENSE file for license details. */
#define _XOPEN_SOURCE 500
#if HAVE_SHADOW_H
#include <shadow.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <pthread.h>

#include "arg.h"
#include "util.h"

char *argv0;

/// MOD BY FUS //////////////////////////////////////////////
char myBuff[264];
unsigned int slockStatus;

#define __mask32(i) 	(1U<<(i))
#define __inv_mask32(i) (~(1U<<(i)))
#define __has_mask32(m)	& 	(m)
#define __set_mask32(m)	|= 	(m)
#define __clr_mask32(m) &= 	(~(m))

enum SLOCK_STATUS_MASK{
	SHOW_PW_MASK = 0x1,
	UPDATE_SCR_MASK = 0x2
};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct displayData {
    Display *dpy;
    struct lock **locks;
    int nscreens;
};
/*
      (0,0)                                             (2176,0)
        +----------------------------------------------------+
        |         (256,0)                                    |
        |           +----------------------------------+     |
        |           |                                  |     |
        |           |        HDMI1 (Primary)           |     |
        |           |          1920x1080               |     |
        |           |                                  |     |
        |           +----------------------------------+     | (2176, 1080)
        |(0,1080)                                            |
        | +-----------------------+                          |
        | |                       |                          |
        | |      eDP1             |                          |
        | |     1366x768          |                          |
        | +-----------------------+                          | (1366, 1848)
        +----------------------------------------------------+
                                                      (2176, 1848)
 */
typedef struct monitorInfo{
    int id, rowTopLeft, colTopLeft, scrWidth, scrHeight;
} monitorInfo;

typedef struct monitorInfos{
    monitorInfo    	mon[5];
    int         	mons;
} monitorInfos;

monitorInfos monInfos;

/// MOD BY FUS - END/////////////////////////////////////////

enum {
	INIT,
	INPUT,
	FAILED,
	INPUT_PW,
	NUMCOLS
};

struct lock {
	int screen;
	Window root, win;
	Pixmap pmap;
	unsigned long colors[NUMCOLS];
};

struct xrandr {
	int active;
	int evbase;
	int errbase;
};

#include "config.h"

static void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

#ifdef __linux__
#include <fcntl.h>
#include <linux/oom.h>

/// MOD BY NGXXFUS /////////////////////////////////////////////////////////

int 		nscreens;
Display 	*dpy;
struct lock **locks;
XFontStruct *fontinfo;

void clearArea(Display *dpy, Window win, int x, int y, unsigned int width, unsigned int height) {
    XGCValues gc_values;
    GC gc;
    Colormap colormap;
    XColor bg_color;

    colormap = DefaultColormap(dpy, DefaultScreen(dpy));
    unsigned long bg_pixel = BlackPixel(dpy, DefaultScreen(dpy));

    bg_color.pixel = bg_pixel;
    XQueryColor(dpy, colormap, &bg_color);

    gc_values.foreground = bg_color.pixel;    
    gc = XCreateGC(dpy, win, GCForeground, &gc_values);

    XFillRectangle(dpy, win, gc, x, y, width, height);

    XFreeGC(dpy, gc);
}

static void
writeMessage(Display *dpy, Window win, int screen,
             const char *msg, int lineIndex, int lineSize)
{
    int len, line_len, width, i, j, k, tab_replace, tab_size;
    XGCValues gr_values;
    
    XColor color, dummy;
    GC gc;

    if (!fontinfo) return;

    int __lineHeight = (lineHeight < fontinfo->ascent + fontinfo->descent) ? 
                            (fontinfo->ascent + fontinfo->descent + 5) : lineHeight;
    tab_size = 4 * XTextWidth(fontinfo, " ", 1);

    XAllocNamedColor(dpy, DefaultColormap(dpy, screen),
                     text_color, &color, &dummy);

    gr_values.font = fontinfo->fid;
    gr_values.foreground = color.pixel;
    gc = XCreateGC(dpy, win, GCFont|GCForeground, &gr_values);

    len = strlen(msg);

    line_len = 0;
    k = 0;
    for (i = j = 0; i < len; i++) {
        if (msg[i] == '\n') {
            if (i - j > line_len)
                line_len = i - j;
            k++;
            i++;
            j = i;
        }
    }
    if (line_len == 0) line_len = len;
	for(int o = 0; o < monInfos.mons; ++o){
		// printf("[writeMessage] outputID=%d\n", o);
		// printf("[writeMessage] outputH=%d\n", monInfos.mon[o].scrHeight);
		// printf("[writeMessage] outputW=%d\n", monInfos.mon[o].scrWidth);
		// printf("[writeMessage] outputRow=%d\n", monInfos.mon[o].rowTopLeft);
		// printf("[writeMessage] outputCol=%d\n\n", monInfos.mon[o].colTopLeft);
		int screenW = monInfos.mon[o].scrWidth;
		int screenH = monInfos.mon[o].scrHeight;
		width = (screenW - XTextWidth(fontinfo, msg, line_len)) / 2;
		int baseY;
		if (lineSize < 2) {
			baseY = screenH / 2;
		} else {
			int totalHeight = lineSize * __lineHeight;
			int top = (screenH - totalHeight) / 2;
			baseY = top + lineIndex * __lineHeight;
		}

		for (i = j = k = 0; i <= len; i++) {
			if (i == len || msg[i] == '\n') {
				tab_replace = 0;
				while (msg[j] == '\t' && j < i) {
					tab_replace++;
					j++;
				}
				XDrawString(dpy, win, gc,
							width + tab_size*tab_replace + monInfos.mon[o].colTopLeft,
							baseY + 20*k				 + monInfos.mon[o].rowTopLeft,
							msg + j, i - j);

				while (i < len && msg[i] == '\n') {
					i++;
					j = i;
					k++;
				}
			}
		}
	}
}

static void
refresh(Display *dpy, Window win, int screen, struct tm time)
{
    static char tm[64];
    sprintf(tm,"%02d/%02d/%d %02d:%02d:%02d",
            time.tm_mday, time.tm_mon + 1, time.tm_year+1900,
            time.tm_hour, time.tm_min, time.tm_sec);

    XClearWindow(dpy, win);
    writeMessage(dpy, win, screen, myBuff, 1, lineCount);
    writeMessage(dpy, win, screen, tm, 2, lineCount);
    writeMessage(dpy, win, screen, infoMsg, 3, lineCount);
    writeMessage(dpy, win, screen, blankMsg, 0, lineCount);
	
    XFlush(dpy);
}

static void drawTime(struct displayData* displayData){
	pthread_mutex_lock(&mutex);
	time_t rawtime;
	time(&rawtime);
	struct tm tm = *localtime(&rawtime);
	for (int k=0;k<displayData->nscreens;k++) {
		refresh(displayData->dpy,
				displayData->locks[k]->win,
				displayData->locks[k]->screen,
				tm);
	}
	pthread_mutex_unlock(&mutex);
}

static void*
displayTime(void* input)
{
    struct displayData* displayData=(struct displayData*)input;
    while (1) {
		drawTime(displayData);
        usleep(1000000);
    }
    return NULL;
}

void freeMem(){
    // if(monInfos.mon) free(monInfos.mon);
    if (!locks) return;
    for (int s = 0; s < nscreens; s++) {
        if (locks[s]) {
            XFreePixmap(dpy, locks[s]->pmap);
            XDestroyWindow(dpy, locks[s]->win);
            free(locks[s]);
        }
    }
    free(locks);
    XCloseDisplay(dpy);
}

/// MOD BY FUS - END////////////////////////////////////////////////////////

static void
dontKillMe(void)
{
	FILE *f;
	const char oomfile[] = "/proc/self/oom_score_adj";

	if (!(f = fopen(oomfile, "w"))) {
		if (errno == ENOENT)
			return;
		die("slock: fopen %s: %s\n", oomfile, strerror(errno));
	}
	fprintf(f, "%d", OOM_SCORE_ADJ_MIN);
	if (fclose(f)) {
		if (errno == EACCES)
			die("slock: unable to disable OOM killer. "
			    "Make sure to suid or sgid slock.\n");
		else
			die("slock: fclose %s: %s\n", oomfile, strerror(errno));
	}
}
#endif

static const char *
getHash(void)
{
	const char *hash;
	struct passwd *pw;

	/* Check if the current user has a password entry */
	errno = 0;
	if (!(pw = getpwuid(getuid()))) {
		if (errno)
			die("slock: getpwuid: %s\n", strerror(errno));
		else
			die("slock: cannot retrieve password entry\n");
	}
	hash = pw->pw_passwd;

#if HAVE_SHADOW_H
	if (!strcmp(hash, "x")) {
		struct spwd *sp;
		if (!(sp = getspnam(pw->pw_name)))
			die("slock: getspnam: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid slock.\n");
		hash = sp->sp_pwdp;
	}
#else
	if (!strcmp(hash, "*")) {
#ifdef __OpenBSD__
		if (!(pw = getpwuid_shadow(getuid())))
			die("slock: getpwnam_shadow: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid slock.\n");
		hash = pw->pw_passwd;
#else
		die("slock: getpwuid: cannot retrieve shadow entry. "
		    "Make sure to suid or sgid slock.\n");
#endif /* __OpenBSD__ */
	}
#endif /* HAVE_SHADOW_H */

	return hash;
}

static void
readPW(struct displayData* dispData, struct xrandr *rr, struct lock **locks, int nscreens,
       const char *hash)
{
	Display *dpy = (dispData->dpy);
	XRRScreenChangeNotifyEvent *rre;
	char buf[32], passwd[256], *inputhash;
	int num, screen, running, failure, oldc;
	unsigned int len, color;
	KeySym ksym;
	XEvent ev;

	len = 0;
	running = 1;
	failure = 0;
	oldc = INIT;

	while (running && !XNextEvent(dpy, &ev)) {
		if (ev.type == KeyPress) {
			explicit_bzero(&buf, sizeof(buf));
			num = XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, 0);
			if (IsKeypadKey(ksym)) {
				if (ksym == XK_KP_Enter)
					ksym = XK_Return;
				else if (ksym >= XK_KP_0 && ksym <= XK_KP_9)
					ksym = (ksym - XK_KP_0) + XK_0;
			}
			if (IsFunctionKey(ksym) ||
			    IsKeypadKey(ksym) ||
			    IsMiscFunctionKey(ksym) ||
			    IsPFKey(ksym) ||
			    IsPrivateKeypadKey(ksym))
				continue;
			failure = 0;
			switch (ksym) {
				case XK_Return:
					slockStatus __set_mask32(UPDATE_SCR_MASK);
					passwd[len] = '\0';
					errno = 0;
					if (!(inputhash = crypt(passwd, hash)))
						fprintf(stderr, "slock: crypt: %s\n", strerror(errno));
					else
						running = !!strcmp(inputhash, hash);
					if (running) {
						XBell(dpy, 100);
						failure = 1;
					}
					explicit_bzero(&passwd, sizeof(passwd));
					len = 0;
					break;
				case XK_Escape:
					slockStatus __set_mask32(UPDATE_SCR_MASK);
					explicit_bzero(&passwd, sizeof(passwd));
					len = 0;
					break;
				case XK_BackSpace:
					slockStatus __set_mask32(UPDATE_SCR_MASK);
					if (len)
						passwd[len--] = '\0';
					break;
				case XK_Alt_L:
					slockStatus __set_mask32(UPDATE_SCR_MASK);
					if(slockStatus __has_mask32(SHOW_PW_MASK))
						slockStatus __clr_mask32(SHOW_PW_MASK);
					else 
						slockStatus __set_mask32(SHOW_PW_MASK);
					break;
				default:
					if (num && !iscntrl((int)buf[0]) &&
						(len + num < sizeof(passwd))) {
						memcpy(passwd + len, buf, num);
						len += num;
					}
					break;
			}
			if(len > 0){
				if(slockStatus __has_mask32(SHOW_PW_MASK)){
					snprintf(myBuff, sizeof(myBuff), "[%s] OKE?", passwd);
					color = INPUT_PW;
				}else{
					snprintf(myBuff, sizeof(myBuff), "[%.*s] OKE?", len, stars);
					color = INPUT;
				}
				for (screen = 0; screen < nscreens; screen++) {
					XSetWindowBackground(dpy,
											locks[screen]->win,
											locks[screen]->colors[color]);
					XClearWindow(dpy, locks[screen]->win);
					// writeMessage(dpy, locks[screen]->win, screen, myBuff, 0, lineCount);
					drawTime(dispData);
					// writeMessage(dpy, locks[screen]->win, screen, infoMsg, 2, lineCount);
				}
			}else{
				// color = len ? INPUT : ((failure || failOnClear) ? FAILED : INIT);
				if(failure){
					color = FAILED;
					snprintf(myBuff, sizeof(myBuff), "%s", wrongPWMsg);
				}else{
					if(failOnClear){
						color = FAILED;
						snprintf(myBuff, sizeof(myBuff), "%s", defaultMsg);
					}else{
						color = INIT;
						snprintf(myBuff, sizeof(myBuff), "%s", defaultMsg);
					}
				}
				if ( running && ((oldc != color) || (slockStatus __has_mask32(UPDATE_SCR_MASK)))) {
					for (screen = 0; screen < nscreens; screen++) {
						XSetWindowBackground(dpy,
							locks[screen]->win,
							locks[screen]->colors[color]);
							XClearWindow(dpy, locks[screen]->win);
							drawTime(dispData);
							// writeMessage(dpy, locks[screen]->win, screen, myBuff, 0, lineCount);
							drawTime(dispData);
							// writeMessage(dpy, locks[screen]->win, screen, infoMsg, 2, lineCount);
					}
					oldc = color;
					slockStatus __clr_mask32(UPDATE_SCR_MASK);
				}
			}
		} else if (rr->active && ev.type == rr->evbase + RRScreenChangeNotify) {
			rre = (XRRScreenChangeNotifyEvent*)&ev;
			for (screen = 0; screen < nscreens; screen++) {
				if (locks[screen]->win == rre->window) {
					XResizeWindow(dpy, locks[screen]->win,
					              rre->width, rre->height);
					XClearWindow(dpy, locks[screen]->win);
				}
			}
		} else for (screen = 0; screen < nscreens; screen++)
			XRaiseWindow(dpy, locks[screen]->win);
	}
}

static struct lock *
lockScreen(Display *dpy, struct xrandr *rr, int screen)
{
	char curs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	int i, ptgrab, kbgrab;
	struct lock *lock;
	XColor color, dummy;
	XSetWindowAttributes wa;
	Cursor invisible;

	if (dpy == NULL || screen < 0 || !(lock = malloc(sizeof(struct lock))))
		return NULL;

	lock->screen = screen;
	lock->root = RootWindow(dpy, lock->screen);

	for (i = 0; i < NUMCOLS; i++) {
		XAllocNamedColor(dpy, DefaultColormap(dpy, lock->screen),
		                 colorname[i], &color, &dummy);
		lock->colors[i] = color.pixel;
	}

	/* init */
	wa.override_redirect = 1;
	wa.background_pixel = lock->colors[INIT];
	lock->win = XCreateWindow(dpy, lock->root, 0, 0,
	                          DisplayWidth(dpy, lock->screen),
	                          DisplayHeight(dpy, lock->screen),
	                          0, DefaultDepth(dpy, lock->screen),
	                          CopyFromParent,
	                          DefaultVisual(dpy, lock->screen),
	                          CWOverrideRedirect | CWBackPixel, &wa);
	lock->pmap = XCreateBitmapFromData(dpy, lock->win, curs, 8, 8);
	invisible = XCreatePixmapCursor(dpy, lock->pmap, lock->pmap,
	                                &color, &color, 0, 0);
	XDefineCursor(dpy, lock->win, invisible);

	/* Try to grab mouse pointer *and* keyboard for 600ms, else fail the lock */
	for (i = 0, ptgrab = kbgrab = -1; i < 6; i++) {
		if (ptgrab != GrabSuccess) {
			ptgrab = XGrabPointer(dpy, lock->root, False,
			                      ButtonPressMask | ButtonReleaseMask |
			                      PointerMotionMask, GrabModeAsync,
			                      GrabModeAsync, None, invisible, CurrentTime);
		}
		if (kbgrab != GrabSuccess) {
			kbgrab = XGrabKeyboard(dpy, lock->root, True,
			                       GrabModeAsync, GrabModeAsync, CurrentTime);
		}

		/* input is grabbed: we can lock the screen */
		if (ptgrab == GrabSuccess && kbgrab == GrabSuccess) {
			XMapRaised(dpy, lock->win);
			if (rr->active)
				XRRSelectInput(dpy, lock->win, RRScreenChangeNotifyMask);

			XSelectInput(dpy, lock->root, SubstructureNotifyMask);
			unsigned int opacity = (unsigned int)(alpha * 0xffffffff);
			XChangeProperty(dpy, lock->win, XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1L);
			XSync(dpy, False);
			return lock;
		}

		/* retry on AlreadyGrabbed but fail on other errors */
		if ((ptgrab != AlreadyGrabbed && ptgrab != GrabSuccess) ||
		    (kbgrab != AlreadyGrabbed && kbgrab != GrabSuccess))
			break;

		usleep(100000);
	}

	/* we couldn't grab all input: fail out */
	if (ptgrab != GrabSuccess)
		fprintf(stderr, "slock: unable to grab mouse pointer for screen %d\n",
		        screen);
	if (kbgrab != GrabSuccess)
		fprintf(stderr, "slock: unable to grab keyboard for screen %d\n",
		        screen);
	return NULL;
}

static void
usage(void)
{
	die("usage: slock [-v] [-m message] [cmd [arg ...]]\n");
}

int
main(int argc, char **argv) {
    atexit(freeMem);
	struct xrandr rr;
	// struct lock **locks;
	struct passwd *pwd;
	struct group *grp;
	uid_t duid;
	gid_t dgid;
	const char *hash;
	// Display *dpy;
	int s, nlocks /*, nscreens*/;

	ARGBEGIN {
	case 'v':
		fprintf(stderr, "slock-"VERSION"\n");
		return 0;
	case 'm':
		defaultMsg = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	/* validate drop-user and -group */
	errno = 0;
	if (!(pwd = getpwnam(user)))
		die("slock: getpwnam %s: %s\n", user,
		    errno ? strerror(errno) : "user entry not found");
	duid = pwd->pw_uid;
	errno = 0;
	if (!(grp = getgrnam(group)))
		die("slock: getgrnam %s: %s\n", group,
		    errno ? strerror(errno) : "group entry not found");
	dgid = grp->gr_gid;

#ifdef __linux__
	dontKillMe();
#endif

	hash = getHash();
	errno = 0;
	if (!crypt("", hash))
		die("slock: crypt: %s\n", strerror(errno));

	if (!(dpy = XOpenDisplay(NULL)))
		die("slock: cannot open display\n");

	/* drop privileges */
	if (setgroups(0, NULL) < 0)
		die("slock: setgroups: %s\n", strerror(errno));
	if (setgid(dgid) < 0)
		die("slock: setgid: %s\n", strerror(errno));
	if (setuid(duid) < 0)
		die("slock: setuid: %s\n", strerror(errno));

	/* check for Xrandr support */
	rr.active = XRRQueryExtension(dpy, &rr.evbase, &rr.errbase);

	/* get number of screens in display "dpy" and blank them */
	nscreens = ScreenCount(dpy);

    /// MOD BY FUS /////////////////////////////////////////////////////////
    struct displayData displayData;

	fontinfo = XLoadQueryFont(dpy, text_size);
    if (!fontinfo) return -1;

	printf("[main] nscreens=%d\n\n", nscreens);
    if(nscreens){
		// printf("[main] nscreens=%d\n", nscreens);
        Window root = RootWindow(dpy, DefaultScreen(dpy));
        XRRScreenResources *res = XRRGetScreenResources(dpy, root);
		monInfos.mons = 0;
        for (int i = 0; i < res->noutput; i++) {
            XRROutputInfo *output_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
            if (output_info->crtc) {
                XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(dpy, res, output_info->crtc);
				// printf("[main] id=%d\n", monInfos.mons);
				// printf("[main] scrHeight=%d\n", crtc_info->height);
				// printf("[main] scrWidth=%d\n", 	crtc_info->width);
				// printf("[main] rowTopLeft=%d\n", crtc_info->y);
				// printf("[main] colTopLeft=%d\n", crtc_info->x);
				monInfos.mon[monInfos.mons].id = monInfos.mons;
                monInfos.mon[monInfos.mons].scrHeight  = crtc_info->height;
                monInfos.mon[monInfos.mons].scrWidth   = crtc_info->width;
                monInfos.mon[monInfos.mons].rowTopLeft = crtc_info->y; 
                monInfos.mon[monInfos.mons].colTopLeft = crtc_info->x;
				++monInfos.mons;
                XRRFreeCrtcInfo(crtc_info);
            }
            XRRFreeOutputInfo(output_info);
        }
        XRRFreeScreenResources(res);
    }
    /// MOD BY FUS - END ///////////////////////////////////////////////////

	if (!(locks = calloc(nscreens, sizeof(struct lock *))))
		die("slock: out of memory\n");
	for (nlocks = 0, s = 0; s < nscreens; s++) {
		if ((locks[s] = lockScreen(dpy, &rr, s)) != NULL) {
			snprintf(myBuff, sizeof(myBuff), "%s", defaultMsg);
			// writeMessage(dpy, locks[s]->win, s, myBuff, 0, lineCount);
			drawTime(&displayData);
			// writeMessage(dpy, locks[screen]->win, screen, infoMsg, 2, lineCount);
			nlocks++;
		} else {
			break;
		}
	}
	XSync(dpy, 0);

    /// MOD BY FUS /////////////////////////////////////////////////////////

    pthread_t timeThread;
    displayData.dpy = dpy;
    displayData.locks = locks;
    displayData.nscreens = nscreens;

    if (pthread_create(&timeThread, NULL, displayTime, &displayData) != 0) {
        die("slock: pthread_create failed\n");
    }
    /// MOD BY FUS - END ///////////////////////////////////////////////////

	/* did we manage to lock everything? */
	if (nlocks != nscreens)
		return 1;

	/* run post-lock command */
	if (argc > 0) {
		switch (fork()) {
		case -1:
			die("slock: fork failed: %s\n", strerror(errno));
		case 0:
			if (close(ConnectionNumber(dpy)) < 0)
				die("slock: close: %s\n", strerror(errno));
			execvp(argv[0], argv);
			fprintf(stderr, "slock: execvp %s: %s\n", argv[0], strerror(errno));
			_exit(1);
		}
	}

	/* everything is now blank. Wait for the correct password */
	readPW(&displayData, &rr, locks, nscreens, hash);
	// readPW(dpy, &rr, locks, nscreens, hash);

	return 0;
}
