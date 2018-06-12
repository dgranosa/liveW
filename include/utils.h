#ifndef _UTIL_H
#define _UTIL_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

float getUnixTime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (float)((int)tv.tv_sec % 1000000) + (float)(tv.tv_usec) / 1.0e6;
}

char *getSystemTime()
{
	char *format;

	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	format = (char *)malloc(16);

	sprintf(format, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	return format;
}

int exec(char *cmd, char *buf, int size)
{
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp == NULL) {
		return -1;
	}

	fgets(buf, size, fp);

	pclose(fp);

	return 0;
}

int getSongInfo(char *artist, char *title)
{
	char info[128];
	info[0] = artist[0] = title[0] = '\0';

	
	if (exec("playerctl metadata artist", info, sizeof(info))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	if (strlen(info))
		strcpy(artist, info);
	
	if (exec("playerctl metadata title", info, sizeof(info))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	if (strlen(artist)) {
		strcpy(title, info);
		return 0;
	}

	bool dash = false;
	int artistPos = 0, titlePos = 0;
	for (int i = 0; i < strlen(info); i++) {
		if (info[i] == '-') {
			dash = true;
			i++;
			continue;
		}

		if (dash && (info[i] == '(' || info[i] == '['))
			break;

		if (!dash) {
			artist[artistPos++] = info[i];
		} else {
			title[titlePos++] = info[i];
		}
	}
	artist[artistPos] = title[titlePos] = '\0';

	if (!strlen(title)) {
		strcpy(title, artist);
		artist[0] = '\0';
	}

	if (strlen(title) && title[strlen(title)-1] == ' ')
		title[strlen(title)-1] = '\0';

	if (strlen(artist) && artist[strlen(artist)-1] == ' ')
		artist[strlen(artist)-1] = '\0';

	return 0;
}

#endif
