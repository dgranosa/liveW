#ifndef _OPENGL_H
#define _OPENGL_H

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

// This includes the new stuff, supplied by the application
#include "GL/glext.h"

int win_width, win_height;

void initGL();
void swapBuffers();

GLuint genRenderProg();

void checkErrors(const char *);

#endif
