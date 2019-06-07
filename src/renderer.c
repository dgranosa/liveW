#include "renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

void loadCharacter(unsigned long c)
{
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
    {
        printf("ERROR::FREETYTPE: Failed to load Glyph\n");
        return;
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

    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    character *chr = (character *) malloc(sizeof(character));
    chr->textureID = texture;
    chr->sizeX = face->glyph->bitmap.width;
    chr->sizeY = face->glyph->bitmap.rows;
    chr->bearingX = face->glyph->bitmap_left;
    chr->bearingY = face->glyph->bitmap_top;
    chr->advance = face->glyph->advance.x;
    characters[c] = chr;

	checkErrors("Loading character");
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

	glGenTextures(1, &r->albumArt);
	glBindTexture(GL_TEXTURE_2D, r->albumArt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glUniform1i(glGetUniformLocation(r->progID, "samples"), 0);
    glUniform1i(glGetUniformLocation(r->progID, "fft"), 1);
    glUniform1i(glGetUniformLocation(r->progID, "albumArt"), 2);

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

	FT_Set_Pixel_Sizes(face, 0, 96);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (unsigned long c = 0; c < 127; c++)
        loadCharacter(c);

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

void renderText(renderer *r, char *_text, GLfloat x, GLfloat y, GLfloat scale, float *color)
{
    if (strlen(_text) == 0)
        return;

    size_t len = mbstowcs(NULL, _text, 0);
    wchar_t *text = malloc(sizeof(wchar_t) * (len + 1));
    size_t size = mbstowcs(text, _text, len);

	glUseProgram(r->progText);
    glUniform3f(glGetUniformLocation(r->progText, "textColor"), color[0], color[1], color[2]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

	if (x == -1.0) {
        float length = 0;

	    for (int i = 0; i < size; i++) {
            if (text[i] < 0 || text[i] > 65536)
                continue;

            if (!characters[(int)text[i]])
                loadCharacter((unsigned long)text[i]);
            character *ch = characters[(int)text[i]];

    		length += (ch->advance >> 6) * scale;
    	}

		x = (r->win->width - length) / 2.0 / r->win->width;
    }

	x *= r->win->width;
	y *= r->win->height;

    for (int i = 0; i < size; i++)
    {
        if (text[i] < 0 || text[i] > 65536)
            continue;

        if (!characters[(int)text[i]])
            loadCharacter((unsigned long)text[i]);
        character *ch = characters[(int)text[i]];

        GLfloat xpos = x + ch->bearingX * scale;
        GLfloat ypos = y - (ch->sizeY - ch->bearingY) * scale;

        GLfloat w = ch->sizeX * scale;
        GLfloat h = ch->sizeY * scale;

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
        glBindTexture(GL_TEXTURE_2D, ch->textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch->advance >> 6) * scale;
	}
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(text);
}

void render(renderer *r, float *sampleBuff, float *fftBuff, int buffSize)
{
    // Clear screen
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

    // Doesn't render if there is no sound
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

    // Configure & link opengl
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_CONSTANT_ALPHA);

	glUseProgram(r->progID);
	glBindVertexArray(vertArray);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, r->audioSamples);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R16, buffSize, 0, GL_RED, GL_FLOAT, sampleBuff);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, r->audioFFT);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R16, buffSize/2+1, 0, GL_RED, GL_FLOAT, fftBuff);

    // Load album art if toggled
	if (r->songInfo.newAlbumArt) {
		stbi_set_flip_vertically_on_load(true);
		r->songInfo.albumArt = stbi_load("image.jpg", &r->songInfo.width, &r->songInfo.height, &r->songInfo.nrChannels, 0);
		if (r->songInfo.albumArt) {
			glActiveTexture(GL_TEXTURE2);
    		glBindTexture(GL_TEXTURE_2D, r->albumArt);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, r->songInfo.width, r->songInfo.height, 0, GL_RGB, GL_UNSIGNED_BYTE, r->songInfo.albumArt);
		} else {
			if (cfg.debug)
				printf("Couldn't load album art\n");
			glActiveTexture(GL_TEXTURE2);
    		glBindTexture(GL_TEXTURE_2D, r->albumArt);

			unsigned char tmp[3] = {255, 255, 255};

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, tmp);

		}

		r->songInfo.newAlbumArt = false;
		stbi_image_free(r->songInfo.albumArt);
	}

    // Set uniforms for shaders
    GLint timeLoc = glGetUniformLocation(r->progID, "time");
    if (timeLoc != -1) glUniform1f(timeLoc, getUnixTime());

    GLint resolutionLoc = glGetUniformLocation(r->progID, "resolution");
    if (resolutionLoc != -1) glUniform2f(resolutionLoc, (float)r->win->width, (float)r->win->height);

    GLint positionLoc = glGetUniformLocation(r->progID, "position");
    if (positionLoc != -1) glUniform1f(positionLoc, r->songInfo.position / ((float)r->songInfo.length + 0.01));

    // Draw screen
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	char *time;
	time = getSystemTime();

	if (cfg.shaderName && !strcmp(cfg.shaderName, "black")) {
		float textColor[] = {0.0, 0.0, 0.0};
		renderText(r, r->songInfo.artist, -1.0f, 0.5371f, 1.25f, textColor);
		renderText(r, r->songInfo.title, -1.0f, 0.4395f, 0.75f, textColor);
		renderText(r, time, -1.0f, 0.7813f, 3.0f, textColor);
	} else
	if (cfg.shaderName && !strcmp(cfg.shaderName, "cat")) {
		float textColor[] = {1.0, 1.0, 1.0};
		renderText(r, r->songInfo.artist, 0.206f, 0.4102f, 1.0f, textColor);
		renderText(r, r->songInfo.title, 0.206f, 0.3418f, 0.75f, textColor);
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
