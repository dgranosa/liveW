#ifndef _PULSEFFT_H
#define _PULSEFFT_H

#include <stdio.h>
#include <complex.h>
#include <tgmath.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>

#include <fftw3.h>

/* TODO: Params to add to config file
 * buffer_samples
 * gravity          57
 * freqScale        58, 65, 73
 * logScale         58, 65
 * smootingFact     73
 * audioRate        70, 74
 */

// TODO: Add this vars to config
const int smoothing_prec = 1024;
float smooth[] = {1.0, 1.0, 1.0, 1.0, 1.0};
int height = 300 - 1;
float g = 1.0;
int highf = 20000, lowf = 20;
float logScale = 1.0;
double sensitivity = 1.0;

typedef struct pa_fft {
    pthread_t thread;
    int cont;

    // Pulseaudio
    pa_simple *s;
    char *dev;
    unsigned int rate;
    int error;
    pa_sample_spec ss;
    pa_channel_map map;

    // Buffers
    unsigned int samples;
    
    double *pa_buff;
    size_t pa_buff_size;
    unsigned int pa_samples;

    double *fft_buff;
    size_t fft_buff_size;
    unsigned int fft_samples;

    // FFTW
    fftw_complex *output;
    size_t output_size;
    unsigned int output_samples;
    fftw_plan plan;

} pa_fft;

void deinit_fft(pa_fft *pa_fft);

void separate_freq_bands(double *out, fftw_complex *in, int n, int *lcf, int *hcf, float *k, double sensitivity, int in_samples)
{
    double peak[n];
    double y[in_samples];
    double temp;

    for (int i = 0; i < n; i++) {
        peak[i] = 0;

        for (int j = lcf[i]; j <= hcf[i]; j++) {
            y[j] = sqrt(creal(in[j]) * creal(in[j]) + cimag(in[j]) * cimag(in[j]));
            peak[i] += y[j];
        }


        peak[i] = peak[i] / (hcf[i] - lcf[i] + 1);
        temp = peak[i] * sensitivity * k[i] / 100000;
        printf("%d: %lf %lf %f -> %lf\n", i, peak[i], sensitivity, k[i], temp);
        out[i] = temp / height;
    }
    printf("\n");
}

int16_t audio_out[65536];

void *pa_fft_thread(void *arg)
{
    pa_fft *t = (struct pa_fft *)arg;

    // Smoothing calculations
    float smoothing[smoothing_prec];
    float gravity = g * ((float)height / 2160) * pow((60 / (float)cfg.fps), 2.5);
    double freqconst = log(highf - lowf) / log(pow(smoothing_prec, logScale));
    printf("%lf\n", freqconst);

    float fc[smoothing_prec], x;
    int lcf[smoothing_prec], hcf[smoothing_prec];

    for (int i = 0; i < smoothing_prec; i++) {
        fc[i] = pow(powf(i, (logScale - 1.0) * ((double)i + 1.0) / ((double)smoothing_prec) + 1.0), freqconst) + lowf;
        x = fc[i] / (t->rate / 2);
        lcf[i] = x * (t->samples / 2);
        if (i != 0)
            hcf[i-1] = lcf[i] - 1;
    }
    hcf[smoothing_prec-1] = highf * t->samples / t->rate;

    for (int i = 0; i < smoothing_prec; i++) {
        smoothing[i] = pow(fc[i], 0.64);
        smoothing[i] *= (float)height / 100;
        smoothing[i] *= smooth[(int)(i / smoothing_prec * sizeof(smooth) / sizeof(*smooth))];
    }

    int16_t buf[t->samples / 2];
    
    int n = 0;

    for (int i = 0; i < t->samples; i++) {
        if (i < t->samples / 2)
            buf[i] = 0;
        audio_out[i] = 0;
        t->pa_buff[i] = 0;
    }

    while (t->cont) {
        if (pa_simple_read(t->s, buf, sizeof(buf), &t->error) < 0) {
            printf("ERROR: pa_simple_read() failed: %s\n", pa_strerror(t->error));
            t->cont = 0;
            continue;
        }

        for (int i = 0; i < t->samples / 2; i += 2) {
            audio_out[n] = (buf[i] + buf[i+1]) / 2;
            n++;
            if (n == t->samples - 1) n = 0;
        }

        for (int i = 0; i < t->samples + 2; i++) {
            if (i < t->samples) {
                t->pa_buff[i] = audio_out[i];
            } else {
                t->pa_buff[i] = 0;
            }
        }

        fftw_execute(t->plan);

        separate_freq_bands(t->fft_buff, t->output, smoothing_prec, lcf, hcf, smoothing, 0.049664, t->output_samples);

        /*for (int i = 0; i < smoothing_prec; i++) {
            if (t->pa_buff[i] > height) {
                sensitivity *= 0.985;
                break;
            }
            if (i == smoothing_prec - 1) sensitivity *= 1.002;
        }*/

        for (int i = 1; i < 10; i++) printf("%lf ", t->fft_buff[i]); printf("\n");
    }

    deinit_fft(t);

    return NULL;
}

static pa_mainloop* m_pulseaudio_mainloop;

static void cb(__attribute__((unused)) pa_context* pulseaudio_context,
               const pa_server_info* i,
               void* userdata) {

    // Obtain default sink name
    pa_fft* pa_fft = (struct pa_fft*) userdata;
    pa_fft->dev = malloc(sizeof(char) * 1024);

    strcpy(pa_fft->dev, i->default_sink_name);

    // Append `.monitor` suffix
    pa_fft->dev = strcat(pa_fft->dev, ".monitor");

    // Quiting mainloop
    pa_context_disconnect(pulseaudio_context);
    pa_context_unref(pulseaudio_context);
    pa_mainloop_quit(m_pulseaudio_mainloop, 0);
    pa_mainloop_free(m_pulseaudio_mainloop);
}


static void pulseaudio_context_state_callback(pa_context* pulseaudio_context, void* userdata) {
    // Ensure loop is ready
    switch (pa_context_get_state(pulseaudio_context)) {
        case PA_CONTEXT_UNCONNECTED:  break;
        case PA_CONTEXT_CONNECTING:   break;
        case PA_CONTEXT_AUTHORIZING:  break;
        case PA_CONTEXT_SETTING_NAME: break;
        case PA_CONTEXT_READY: // Extract default sink name
            pa_operation_unref(pa_context_get_server_info(pulseaudio_context, cb, userdata));
            break;
        case PA_CONTEXT_FAILED:
            printf("failed to connect to pulseaudio server\n");
            exit(EXIT_FAILURE);
            break;
        case PA_CONTEXT_TERMINATED:
            pa_mainloop_quit(m_pulseaudio_mainloop, 0);
            break;
    }
}

void get_pulse_default_sink(pa_fft* pa_fft) {
    pa_mainloop_api* mainloop_api;
    pa_context* pulseaudio_context;
    int ret;

    // Create a mainloop API and connection to the default server
    m_pulseaudio_mainloop = pa_mainloop_new();

    mainloop_api = pa_mainloop_get_api(m_pulseaudio_mainloop);
    pulseaudio_context = pa_context_new(mainloop_api, "liveW device list");


    // Connect to the PA server
    pa_context_connect(pulseaudio_context, NULL, PA_CONTEXT_NOFLAGS,
                   NULL);

    // Define a callback so the server will tell us its state
    pa_context_set_state_callback(pulseaudio_context,
                              pulseaudio_context_state_callback,
                              (void*)pa_fft);

    // Start mainloop to get default sink

    // Start with one non blocking iteration in case pulseaudio is not able to run
    if (!(ret = pa_mainloop_iterate(m_pulseaudio_mainloop, 0, &ret))){
    printf("Could not open pulseaudio mainloop to find default device name: %d\n"
           "Check if pulseaudio is running\n",
           ret);

        exit(EXIT_FAILURE);
    }

    pa_mainloop_run(m_pulseaudio_mainloop, &ret);
}

void init_pulse(pa_fft *pa_fft)
{
    if (!pa_fft->dev)
        get_pulse_default_sink(pa_fft);

    if (cfg.debug)
        printf("device = %s\n", pa_fft->dev);

    pa_fft->ss.format = PA_SAMPLE_S16LE;
    pa_fft->ss.rate = pa_fft->rate;
    pa_fft->ss.channels = 2;

    const pa_buffer_attr pb = {
        .maxlength = (uint32_t) -1,
        .fragsize = pa_fft->samples
    };

    if (!(pa_fft->s = pa_simple_new(NULL, "liveW", PA_STREAM_RECORD,
                                    pa_fft->dev, "liveW audio source",
                                    &pa_fft->ss, NULL, &pb,
                                    &pa_fft->error))) {
        printf("ERROR: pa_simple_new() failed: %s\n",
                pa_strerror(pa_fft->error));
        pa_fft->cont = 0;
        return;
    }
}

void init_buffers(pa_fft *pa_fft)
{
    if (!pa_fft)
        return;

    // Pulse buffer
    pa_fft->pa_samples = pa_fft->samples + 2;
    pa_fft->pa_buff_size = sizeof(double) * pa_fft->pa_samples;
    pa_fft->pa_buff = malloc(pa_fft->pa_buff_size);

    // FFTW buffer
    pa_fft->output_samples = (pa_fft->samples / 2) + 1;
    pa_fft->output_size = sizeof(fftw_complex) * pa_fft->output_samples;
    pa_fft->output = fftw_malloc(pa_fft->output_size);

    // Output buffer
    pa_fft->fft_samples = pa_fft->samples + 2;
    pa_fft->fft_buff_size = sizeof(double) * pa_fft->fft_samples;
    pa_fft->fft_buff = malloc(pa_fft->fft_buff_size);
}

void init_fft(pa_fft *pa_fft)
{
    if (!pa_fft)
        return;

    pa_fft->plan = fftw_plan_dft_r2c_1d(pa_fft->pa_samples, pa_fft->pa_buff,
                                        pa_fft->output, FFTW_MEASURE);
}

void deinit_fft(pa_fft *pa_fft)
{
    if (!pa_fft)
        return;

    if (pa_fft->cont != 2) {
        pa_fft->cont = 0;
        sleep(1);
    }

    fftw_destroy_plan(pa_fft->plan);
    fftw_free(pa_fft->output);

    if (pa_fft->cont != 2)
        pa_simple_free(pa_fft->s);
    free(pa_fft->pa_buff);
    free(pa_fft->fft_buff);

    pthread_join(pa_fft->thread, NULL);

    free(pa_fft);
}

#endif // _PULSEFFT_H
