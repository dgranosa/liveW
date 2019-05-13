#include "xwin.h"

void initWindow(xwin *win);
void initBackground(xwin *win);

xwin *init_xwin()
{
    xwin *win = (xwin *)malloc(sizeof(struct xwin));

    if (!(win->display = XOpenDisplay(NULL))) {
		printf("Couldn't open X11 display\n");
		exit(1);
	}

    if (cfg.windowed)
        initWindow(win);
    else
        initBackground(win);

    if (cfg.plasma) {
        long value = XInternAtom(win->display, "_NET_WM_WINDOW_TYPE_DESKTOP", false);
        XChangeProperty(win->display, win->window,
                        XInternAtom(win->display, "_NET_WM_WINDOW_TYPE", false),
                        XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
        XMapWindow(win->display, win->window);
    }

    if (cfg.transparency < 1.0) {
        uint32_t cardinal_alpha = (uint32_t) (cfg.transparency * (uint32_t)-1) ;
        XChangeProperty(win->display, win->window,
                        XInternAtom(win->display, "_NET_WM_WINDOW_OPACITY", 0),
                        XA_CARDINAL, 32, PropModeReplace, (uint8_t*) &cardinal_alpha, 1);
    }
    return win;
}

void initWindow(xwin *win)
{
	int attr[] = {
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
	};

	win->screenNum = DefaultScreen(win->display);
	win->root = RootWindow(win->display, win->screenNum);

	win->offX = cfg.offX, win->offY = cfg.offY;
	if (cfg.geometry) {
		win->offX = cfg.offX, win->offY = cfg.offY;
		win->width = cfg.width, win->height = cfg.height;
	} else {
		win->width = DisplayWidth(win->display, win->screenNum),
		win->height = DisplayHeight(win->display, win->screenNum);
	}

	int elemc;
	win->fbcs = glXChooseFBConfig(win->display, win->screenNum, attr, &elemc);
	if (!win->fbcs) {
		printf("Couldn't get FB configs\n");
		exit(1);
	}

	for (int i = 0; i < elemc; i++) {
		win->vi = (XVisualInfo *)glXGetVisualFromFBConfig(win->display, win->fbcs[i]);
		if (!win->vi)
			   continue;

		win->pict = XRenderFindVisualFormat(win->display, win->vi->visual);
		XFree(win->vi);
		if (!win->pict)
			continue;

		win->fbc = win->fbcs[i];
		if (win->pict->direct.alphaMask > 0)
			break;
	}

	XFree(win->fbcs);

	win->vi = (XVisualInfo *)glXGetVisualFromFBConfig(win->display, win->fbc);
	if (!win->vi) {
		printf("Couldn't get a visual\n");
		exit(1);
	}

	// Window parameters
	win->swa.background_pixel = 0;
	win->swa.border_pixel = 0;
	win->swa.colormap = XCreateColormap(win->display, win->root, win->vi->visual, AllocNone);
	win->swa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	if (cfg.debug)
        printf("Window depth %d, %dx%d\n", win->vi->depth, win->width, win->height);

	win->window = XCreateWindow(win->display, win->root, win->offX, win->offY, win->width, win->height, 0, win->vi->depth, InputOutput, win->vi->visual, mask, &win->swa);
}

static Window find_subwindow(xwin *r, Window win, int w, int h)
{
    unsigned int i, j;
    Window troot, parent, *children;
    unsigned int n;

    for (i = 0; i < 10; i++) {
        XQueryTree(r->display, win, &troot, &parent, &children, &n);

        for (j = 0; j < n; j++) {
            XWindowAttributes attrs;

            if (XGetWindowAttributes(r->display, children[j], &attrs)) {
                if (attrs.map_state != 0 && (attrs.width == w && attrs.height == h)) {
                    win = children[j];
                    break;
                }
            }
        }

        XFree(children);
        if (j == n) {
            break;
        }
    }

    return win;
}

static Window find_desktop_window(xwin *r)
{
    Window root = RootWindow(r->display, r->screenNum);
    Window win = root;

    win = find_subwindow(r, root, -1, -1);

    win = find_subwindow(r, win, r->width, r->height);

    if (cfg.debug) {
        if (win != root) {
            printf("Desktop window (%lx) is subwindow of root window (%lx)\n", win, root);
        } else {
            printf("Desktop window (%lx) is root window\n", win);
        }
    }

    r->root = root;
    r->desktop = win;

    return win;
}

void initBackground(xwin *win)
{
	int attr[] = {
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
	};

	win->screenNum = DefaultScreen(win->display);
	win->root = RootWindow(win->display, win->screenNum);

	if (cfg.geometry) {
		win->offX = cfg.offX, win->offY = cfg.offY;
		win->width = cfg.width, win->height = cfg.height;
	} else {
		win->width = DisplayWidth(win->display, win->screenNum),
		win->height = DisplayHeight(win->display, win->screenNum);
	}

	if(!find_desktop_window(win)) {
		printf("Error: couldn't find desktop window\n");
		exit(1);
	}

	int elemc;
	win->fbcs = glXChooseFBConfig(win->display, win->screenNum, attr, &elemc);
	if (!win->fbcs) {
		printf("Couldn't get FB configs\n");
		exit(1);
	}

	for (int i = 0; i < elemc; i++) {
		win->vi = (XVisualInfo *)glXGetVisualFromFBConfig(win->display, win->fbcs[i]);
		if (!win->vi)
			   continue;

		win->pict = XRenderFindVisualFormat(win->display, win->vi->visual);
		XFree(win->vi);
		if (!win->pict)
			continue;

		win->fbc = win->fbcs[i];
		if (win->pict->direct.alphaMask > 0)
			break;
	}

	XFree(win->fbcs);

	win->vi = (XVisualInfo *)glXGetVisualFromFBConfig(win->display, win->fbc);
	if (!win->vi) {
		printf("Couldn't get a visual\n");
		exit(1);
	}

	// Window parameters
	win->swa.background_pixmap = ParentRelative;
	win->swa.background_pixel = 0;
	win->swa.border_pixmap = 0;
	win->swa.border_pixel = 0;
	win->swa.bit_gravity = 0;
	win->swa.win_gravity = 0;
	win->swa.override_redirect = True;
	win->swa.colormap = XCreateColormap(win->display, win->root, win->vi->visual, AllocNone);
	win->swa.event_mask = StructureNotifyMask | ExposureMask;
	unsigned long mask = CWOverrideRedirect | CWBackingStore | CWBackPixel | CWBorderPixel | CWColormap;

	if (cfg.debug)
        printf("Window depth %d, %dx%d\n", win->vi->depth, win->width, win->height);

	win->window = XCreateWindow(win->display, win->root, win->offX, win->offY, win->width, win->height, 0,
			win->vi->depth, InputOutput, win->vi->visual, mask, &win->swa);

	XLowerWindow(win->display, win->window);
}



void swapBuffers(xwin *win) {
	glXSwapBuffers(win->display, win->window);
    checkErrors("Swapping buffs");
}

