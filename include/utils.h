#ifndef _UTIL_H
#define _UTIL_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>

#include "config.h"

typedef struct SongInfo {
	pthread_t thread;
	char artist[128];
	char title[128];
    int position, length;

	bool newAlbumArt;
	int width, height, nrChannels;
	unsigned char *albumArt;

    int *cont;
} SongInfo;

float getUnixTime();
char *getSystemTime();
int exec(char *cmd, char *buf, int size);
int getSongInfo(char *artist, char *title, int *position, int *length);
void *updateSongInfo(void *arg);

#endif
