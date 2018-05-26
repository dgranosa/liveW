#ifndef _XWIN_H
#define _XWIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#include "opengl.h"
#include "config.h"

typedef struct xwin {
	Display *display;
	Window root;
	Window window;
	Window desktop;
	GLint screenNum;

	XSetWindowAttributes swa;
	GLXFBConfig fbc, *fbcs;
	XVisualInfo *vi;
	XRenderPictFormat *pict;
	Colormap cmap;

	GLint width, height;
} xwin;

xwin *init_xwin();
void swapBuffers();

#endif
