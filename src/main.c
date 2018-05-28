#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "config.h"
#include "shader.h"
#include "renderer.h"
#include "pulsefft.h"

int main(int argc, char *argv[])
{
    if (!parseArgs(argc, argv)) {
        printHelp();
        return 0;
    }

    renderer *rend = init_rend();

    char *vertPath = NULL, *fragPath = NULL;
    if (cfg.shaderName) {
        vertPath = (char*)malloc((strlen("Shaders//vert.glsl") + strlen(cfg.shaderName)) * sizeof(char));
        fragPath = (char*)malloc((strlen("Shaders//frag.glsl") + strlen(cfg.shaderName)) * sizeof(char));
        sprintf(vertPath, "Shaders/%s/vert.glsl", cfg.shaderName);
        sprintf(fragPath, "Shaders/%s/frag.glsl", cfg.shaderName);
    }
    rend->progID = loadShaders(vertPath, fragPath);

    struct pa_fft *ctx = calloc(1, sizeof(struct pa_fft));
    ctx->cont = 1;
    ctx->buffer_samples = 1024;
    ctx->dev = cfg.src;
    ctx->frame_avg = 2;
    ctx->start_low = 1;
    ctx->win_type = WINDOW_HAMMING;
    ctx->fft_flags = FFTW_PATIENT | FFTW_DESTROY_INPUT;

    init_pulse(ctx);
    init_buffers(ctx);
    init_fft(ctx);

    if (!ctx->cont) {
        ctx->cont = 2;
        deinit_fft(ctx);
        return 1;
    }

    linkBuffers(rend);

    pthread_create(&ctx->thread, NULL, pa_fft_thread, ctx);

    while(ctx->cont) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

        render(rend, ctx->pa_buf, ctx->fftBuff, ctx->buffer_samples);

        checkErrors("Draw screen");
        swapBuffers(rend->win);

    	usleep(1000000 / cfg.fps);
    }

    return 0;
}
