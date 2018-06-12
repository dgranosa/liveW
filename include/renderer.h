#ifndef _RENDERER_H
#define _RENDERER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "xwin.h"
#include "config.h"

typedef struct renderer {
	xwin *win;
    GLuint progID, progText;
    GLuint audioSamples;
    GLuint audioFFT;
	GLXContext ctx;
} renderer;

GLuint vertArray, posBuf;

FT_Library ft;
FT_Face face;

typedef struct character {
    GLuint	textureID;
    float	sizeX, sizeY;
    float	bearingX, bearingY;
    GLuint	advance;
} character;

character characters[128];

GLuint VAO, VBO;

renderer *init_rend();
void linkBuffers(renderer *r);
void render(renderer *r, float *sampleBuff, float *fftBuff, int buffSize);

#endif
