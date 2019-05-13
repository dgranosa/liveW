#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "utils.h"
#include "config.h"
#include "shader.h"
#include "renderer.h"
#include "pulsefft.h"

int main(int argc, char *argv[])
{
    // Parse arguments
    if (!parseArgs(argc, argv)) {
        printHelp();
        return 0;
    }

    // Init renderer
    renderer *rend = init_rend();

    // Load shaders
    char *vertPath = NULL, *fragPath = NULL;
    if (cfg.shaderName) {
        vertPath = (char*)malloc((strlen("Shaders//vert.glsl") + strlen(cfg.shaderName)) * sizeof(char));
        fragPath = (char*)malloc((strlen("Shaders//frag.glsl") + strlen(cfg.shaderName)) * sizeof(char));
        sprintf(vertPath, "Shaders/%s/vert.glsl", cfg.shaderName);
        sprintf(fragPath, "Shaders/%s/frag.glsl", cfg.shaderName);
    }
    rend->progID = loadShaders(vertPath, fragPath);

    // Load text shaders
    rend->progText = loadShaders("Shaders/text/vert.glsl", "Shaders/text/frag.glsl");

    // Configure fft
    struct pa_fft *ctx = calloc(1, sizeof(struct pa_fft));
    ctx->cont = 1;
    ctx->samples = 4096;
    ctx->dev = cfg.src;

    // Init pulseaudio && fft
    init_pulse(ctx);
    init_buffers(ctx);
    init_fft(ctx);

    if (!ctx->cont) {
        ctx->cont = 2;
        deinit_fft(ctx);
        return 1;
    }

    linkBuffers(rend);

    // Create threads for pulseaudio & getting song info
    pthread_create(&ctx->thread, NULL, pa_fft_thread, ctx);
    pthread_create(&rend->songInfo.thread, NULL, updateSongInfo, &rend->songInfo);

    while(ctx->cont) {
        render(rend, ctx->pa_buff, ctx->fft_buff, ctx->samples);

		usleep(1000000 / cfg.fps);
    }

    return 0;
}
