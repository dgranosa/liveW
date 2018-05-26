#ifndef _RENDERER_H
#define _RENDERER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xwin.h"
#include "config.h"

typedef struct renderer {
	xwin *win;
	GLXContext ctx;

    GLuint progID;

    GLuint audioSamples;
    GLuint audioFFT;
    GLuint image;
} renderer;

renderer *init_rend();
void linkBuffers(renderer *r);
void render(renderer *r, float *sampleBuff, float *fftBuff, int buffSize);

#endif
