#include "renderer.h"

renderer *init_rend()
{
    renderer *r = (renderer *)malloc(sizeof(struct renderer));

    r->win = init_xwin();

	int gl3attr[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
    };

	r->ctx = glXCreateContextAttribsARB(r->win->display, r->win->fbc, NULL, True, gl3attr);

	if (!r->ctx) {
		printf("Couldn't create an OpenGL context\n");
		exit(1);
	}

	XTextProperty windowName;
	windowName.value = (unsigned char *) "Live wallpaper";
	windowName.encoding = XA_STRING;
	windowName.format = 8;
	windowName.nitems = strlen((char *) windowName.value);

	XSetWMName(r->win->display, r->win->window, &windowName);

	XMapWindow(r->win->display, r->win->window);
	glXMakeCurrent(r->win->display, r->win->window, r->ctx);

	if (cfg.debug) {
        printf("OpenGL:\n\tvendor %s\n\trenderer %s\n\tversion %s\n\tshader language %s\n",
			glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION),
			glGetString(GL_SHADING_LANGUAGE_VERSION));
	}

	int extCount;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extCount);
	Bool found = False;
	for (int i = 0; i < extCount; ++i)
		if (!strcmp((const char*)glGetStringi(GL_EXTENSIONS, i), "GL_ARB_compute_shader")) {
			if (cfg.debug)
				printf("Extension \"GL_ARB_compute_shader\" found\n");
			found = True;
			break;
		}

	if (!found) {
		printf("Extension \"GL_ARB_compute_shader\" not found\n");
		exit(1);
	}

	glViewport(0, 0, r->win->width, r->win->height);

	checkErrors("Window init");

	return r;
}

void linkBuffers(renderer *r)
{
	glUseProgram(r->progID);

    glGenVertexArrays(1, &vertArray);
	glBindVertexArray(vertArray);

	glGenBuffers(1, &posBuf);
    glBindBuffer(GL_ARRAY_BUFFER, posBuf);
	float data[] = {
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, data, GL_STREAM_DRAW);
	GLint posPtr = glGetAttribLocation(r->progID, "pos");
    glVertexAttribPointer(posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posPtr);

	checkErrors("Linking buffers");

	glGenTextures(1, &r->audioSamples);
    glBindTexture(GL_TEXTURE_1D, r->audioSamples);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &r->audioFFT);
    glBindTexture(GL_TEXTURE_1D, r->audioFFT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUniform1i(glGetUniformLocation(r->progID, "samples"), 0);
    glUniform1i(glGetUniformLocation(r->progID, "fft"), 1);

    checkErrors("Linking textures");

	glUseProgram(r->progText);

	if (FT_Init_FreeType(&ft)) {
    	printf("ERROR::FREETYPE: Could not init FreeType Library\n");
		exit(1);
	}
   
   	char *fontPath = NULL;
	fontPath = (char*)malloc((strlen("fonts/") + strlen(cfg.fontName)) * sizeof(char));
	sprintf(fontPath, "fonts/%s", cfg.fontName);

	if (FT_New_Face(ft, fontPath, 0, &face)) {
    	printf("ERROR::FREETYPE: Failed to load font\n");
		exit(1);
	}
	free(fontPath);

	FT_Set_Pixel_Sizes(face, 0, 48);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (GLubyte c = 0; c < 128; c++)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			printf("ERROR::FREETYTPE: Failed to load Glyph\n");
			continue;
		}


		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		character chr = {
			texture,
			face->glyph->bitmap.width, face->glyph->bitmap.rows,
			face->glyph->bitmap_left, face->glyph->bitmap_top,
			face->glyph->advance.x
		};
		characters[c] = chr;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
 
	checkErrors("Loading font");
}

void renderText(renderer *r, char *text, GLfloat x, GLfloat y, GLfloat scale, float *color) //TODO: Make position to be relative, support for unicode, better rasterisation
{
	glUseProgram(r->progText);
    glUniform3f(glGetUniformLocation(r->progText, "textColor"), color[0], color[1], color[2]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

	float lenght = 0;

	for (int i = 0; i < strlen(text); i++) {
		if (text[i] < 0 || text[i] > 127)
			continue;

		character ch = characters[(int)text[i]];

		lenght += (ch.advance >> 6) * scale;
	}

	if (x == -1.0)
		x = (r->win->width - lenght) / 2.0;

    for (int i = 0; i < strlen(text); i++)
    {
		if (text[i] < 0 || text[i] > 127)
			continue;

        character ch = characters[(int)text[i]];

        GLfloat xpos = x + ch.bearingX * scale;
        GLfloat ypos = y - (ch.sizeY - ch.bearingY) * scale;

        GLfloat w = ch.sizeX * scale;
        GLfloat h = ch.sizeY * scale;

		xpos = (xpos / r->win->width ) * 2.0 - 1.0;
		ypos = (ypos / r->win->height) * 2.0 - 1.0;
		w    = (w    / r->win->width ) * 2.0;
		h    = (h    / r->win->height) * 2.0;

        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.advance >> 6) * scale;
	}
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render(renderer *r, float *sampleBuff, float *fftBuff, int buffSize)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	if (cfg.dontDrawIfNoSound) {
		bool noNewSound = true;
		for (int i = 0; i < buffSize; i++)
			if (*(sampleBuff + i))
				noNewSound = false;

			if (noNewSound) {
				swapBuffers(r->win);
				return;
			}
	}

    glDisable(GL_BLEND);

	glUseProgram(r->progID);
	glBindVertexArray(vertArray);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, r->audioSamples);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R16, buffSize, 0, GL_RED, GL_FLOAT, sampleBuff);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, r->audioFFT);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R16, buffSize/2, 0, GL_RED, GL_FLOAT, fftBuff);

    GLint timeLoc = glGetUniformLocation(r->progID, "time");
    if (timeLoc != -1) glUniform1f(timeLoc, getUnixTime());

    GLint resolutionLoc = glGetUniformLocation(r->progID, "resolution");
    if (resolutionLoc != -1) glUniform2f(resolutionLoc, (float)r->win->width, (float)r->win->height);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	char *time;
	time = getSystemTime();


	if (cfg.debug)
		printf("%s\n%s\n\n", r->songInfo.artist, r->songInfo.title);

	if (cfg.shaderName && !strcmp(cfg.shaderName, "black")) {
		float textColor[] = {0.0, 0.0, 0.0};
		renderText(r, r->songInfo.artist, -1.0f, 550.0f, 2.5f, textColor);
		renderText(r, r->songInfo.title, -1.0f, 450.0f, 1.5f, textColor);
		renderText(r, time, -1.0f, 800.0f, 3.0f, textColor);
	} else
	if (cfg.shaderName && !strcmp(cfg.shaderName, "cat")) {
		float textColor[] = {1.0, 1.0, 1.0};
		renderText(r, r->songInfo.artist, 280.0f, 420.0f, 2.0f, textColor);
		renderText(r, r->songInfo.title, 280.0f, 350.0f, 1.5f, textColor);
	} else {
		float textColor[] = {0.0, 0.0, 0.0};
		renderText(r, time, -1.0f, 800.0f, 3.0f, textColor);
	}

	checkErrors("Draw screen");
	swapBuffers(r->win);
}

void checkErrors(const char *desc) {
	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		printf("OpenGL error in \"%s\": %s (%d)\n", desc, gluErrorString(e), e);
		exit(1);
	}
}
