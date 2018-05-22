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

enum w_type {
    WINDOW_TRIANGLE,
    WINDOW_HANNING,
    WINDOW_HAMMING,
    WINDOW_BLACKMAN,
    WINDOW_BLACKMAN_HARRIS,
    WINDOW_WELCH,
    WINDOW_FLAT,
};

typedef struct pa_fft {
    pthread_t thread;
    bool stop;
    int cont;

    pa_simple *s;
    const char *dev;
    int error;
    pa_sample_spec ss;
    pa_channel_map map;

    float *pa_buf;
    size_t pa_buf_size;
    unsigned int pa_samples;
    double *buffer;
    size_t buffer_size;
    unsigned int buffer_samples;
    float **frame_avg_mag;
    unsigned int size_avg;
    unsigned int frame_avg;

    int fft_flags;
    unsigned int start_low;
    enum w_type win_type;
    fftw_complex *output;
    unsigned int output_size;
    unsigned int fft_memb;
    double fft_fund_freq;
    fftw_plan plan;

    float fftBuff[512];
} pa_fft;

void deinit_fft(pa_fft *pa_fft) {
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
    free(pa_fft->buffer);
    free(pa_fft->pa_buf);

    pthread_join(pa_fft->thread, NULL);

    free(pa_fft);
}

static inline void avg_buf_init(pa_fft *pa_fft)
{
    pa_fft->frame_avg_mag = malloc(pa_fft->fft_memb*sizeof(float *));
    for (int i = 0; i < pa_fft->fft_memb; i++)
        pa_fft->frame_avg_mag[i] = calloc(pa_fft->frame_avg, sizeof(float));
}

static inline void weights_init(float *dest, int samples, enum w_type w)
{
    switch(w) {
        case WINDOW_TRIANGLE:
            for (int i = 0; i < samples; i++)
                dest[i] = 1 - 2*fabsf((i - ((samples - 1)/2.0f))/(samples - 1));
            break;
        case WINDOW_HANNING:
            for (int i = 0; i < samples; i++)
                dest[i] = 0.5f*(1 - cos((2*M_PI*i)/(samples - 1)));
            break;
        case WINDOW_HAMMING:
            for (int i = 0; i < samples; i++)
                dest[i] = 0.54 - 0.46*cos((2*M_PI*i)/(samples - 1));
            break;
        case WINDOW_BLACKMAN:
            for (int i = 0; i < samples; i++) {
                const float c1 = cos((2*M_PI*i)/(samples - 1));
                const float c2 = cos((4*M_PI*i)/(samples - 1));
                dest[i] = 0.42659 - 0.49656*c1 + 0.076849*c2;
            }
            break;
        case WINDOW_BLACKMAN_HARRIS:
            for (int i = 0; i < samples; i++) {
                const float c1 = cos((2*M_PI*i)/(samples - 1));
                const float c2 = cos((4*M_PI*i)/(samples - 1));
                const float c3 = cos((6*M_PI*i)/(samples - 1));
                dest[i] = 0.35875 - 0.48829*c1 + 0.14128*c2 - 0.001168*c3;
            }
            break;
        case WINDOW_FLAT:
            for (int i = 0; i < samples; i++)
                dest[i] = 1.0f;
            break;
        case WINDOW_WELCH:
            for (int i = 0; i < samples; i++)
                dest[i] = 1 - pow((i - ((samples - 1)/2.0f))/((samples - 1)/2.0f), 2.0f);
            break;
        default:
            for (int i = 0; i < samples; i++)
                dest[i] = 0.0f;
            break;
    }
    float sum = 0.0f;
    for (int i = 0; i < samples; i++)
        sum += dest[i];
    for (int i = 0; i < samples; i++)
        dest[i] /= sum;
}

static inline void apply_win(double *dest, float *src, float *weights,
                             int samples)
{
    for (int i = 0; i < samples; i++)
        dest[i] = src[i]*weights[i];
}

static inline float frame_average(float mag, float *buf, int avgs, int no_mod)
{
    if (!avgs)
        return mag;
    float val = mag;
    for (int i = 0; i < avgs; i++)
        val += buf[i];
    val /= avgs + 1;
    if (no_mod)
        return val;
    for (int i = avgs - 1; i > 0; i--)
        buf[i] = buf[i-1];
    buf[0] = mag;
    return val;
}

void *pa_fft_thread(void *arg) {
    pa_fft *t = (struct pa_fft *)arg;
    float weights[t->buffer_samples];

    avg_buf_init(t);
    weights_init(weights, t->fft_memb, t->win_type);

    while (t->cont) {
        if (pa_simple_read(t->s, &t->pa_buf[0],
            t->pa_buf_size, &t->error) < 0) {
            printf("ERROR: pa_simple_read() failed: %s\n",
                    pa_strerror(t->error));
            t->cont = 0;
            continue;
        }

        apply_win(t->buffer, t->pa_buf, weights, t->buffer_samples);
        fftw_execute(t->plan);

        double mag_max = 0.0f;
        for (int i = t->start_low; i < t->fft_memb; i++) {
            fftw_complex num = t->output[i];
            double mag = creal(num)*creal(num) + cimag(num)*cimag(num);
            mag = log10(mag)/10;
            mag = frame_average(mag, t->frame_avg_mag[i], t->frame_avg, 1);
            mag_max = mag > mag_max ? mag : mag_max;
        }

        for (int i = t->start_low; i < t->fft_memb; i++) {
            fftw_complex num = t->output[i];
            double mag = creal(num)*creal(num) + cimag(num)*cimag(num);
            mag = log10(mag)/10;
            mag = frame_average(mag, t->frame_avg_mag[i], t->frame_avg, 0);
            t->fftBuff[i - t->start_low] = ((float)mag + (float)mag_max + 0.5f) / 2.0f + 0.5f;
        }
    }

    deinit_fft(t);

    return NULL;
}

static inline void init_pulse(pa_fft *pa_fft)
{
    if (cfg.debug)
        printf("device = %s\n", pa_fft->dev);
    if (!pa_fft->dev) {
        printf("Warning: no device specified! It's highly possible "
                        "Pulseaudio will attempt to use the microphone!\n");
    }

    pa_fft->ss.format = PA_SAMPLE_FLOAT32LE;
    pa_fft->ss.rate = 44100;
    pa_fft->ss.channels = 1;
    pa_channel_map_init_mono(&pa_fft->map);

    if (!(pa_fft->s = pa_simple_new(NULL, "pa_fft", PA_STREAM_RECORD, pa_fft->dev,
                                    "record", &pa_fft->ss, &pa_fft->map, NULL,
                                    &pa_fft->error))) {
        printf("ERROR: pa_simple_new() failed: %s\n",
                pa_strerror(pa_fft->error));
        pa_fft->cont = 0;
        return;
    }
}

static inline void init_buffers(pa_fft *pa_fft)
{
    if (!pa_fft)
        return;

    /* Pulse buffer */
    pa_fft->pa_samples = pa_fft->buffer_samples/(1);
    pa_fft->pa_buf_size = sizeof(float)*pa_fft->buffer_samples;
    pa_fft->pa_buf = malloc(pa_fft->pa_buf_size);

    /* Input buffer */
    pa_fft->buffer_size = sizeof(double)*pa_fft->buffer_samples;
    pa_fft->buffer = malloc(pa_fft->buffer_size);

    /* FFTW buffer */
    pa_fft->output_size = sizeof(fftw_complex)*pa_fft->buffer_samples;
    pa_fft->output = fftw_malloc(pa_fft->output_size);
    pa_fft->fft_memb = (pa_fft->buffer_samples/2)+1;
    pa_fft->fft_fund_freq = (double)pa_fft->ss.rate/pa_fft->buffer_samples;
}

static inline void init_fft(pa_fft *pa_fft) {
    if (!pa_fft)
        return;

    pa_fft->plan = fftw_plan_dft_r2c_1d(pa_fft->buffer_samples, pa_fft->buffer,
                                        pa_fft->output, pa_fft->fft_flags);
}

#endif // _PULSEFFT_H
