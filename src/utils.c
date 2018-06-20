#include "utils.h"

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
	char buff[256];
	char newArtist[64], newTitle[64];
	buff[0] = newArtist[0] = newTitle[0] = '\0';

	
	if (exec("playerctl metadata artist", buff, sizeof(buff))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	if (strlen(buff))
		strcpy(newArtist, buff);
	
	if (exec("playerctl metadata title", buff, sizeof(buff))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	if (strlen(newArtist)) {
		strcpy(artist, newArtist);
		strcpy(title, buff);
		return 0;
	}

	bool dash = false;
	int artistPos = 0, titlePos = 0;
	for (int i = 0; i < strlen(buff); i++) {
		if (buff[i] == '-') {
			dash = true;
			i++;
			continue;
		}

		if (dash && (buff[i] == '(' || buff[i] == '['))
			break;

		if (!dash) {
			newArtist[artistPos++] = buff[i];
		} else {
			newTitle[titlePos++] = buff[i];
		}
	}
	newArtist[artistPos] = newTitle[titlePos] = '\0';

	if (!strlen(newTitle)) {
		strcpy(newTitle, newArtist);
		newArtist[0] = '\0';
	}

	strcpy(artist, newArtist);
	strcpy(title, newTitle);

	if (strlen(title) && title[strlen(title)-1] == ' ')
		title[strlen(title)-1] = '\0';

	if (strlen(artist) && artist[strlen(artist)-1] == ' ')
		artist[strlen(artist)-1] = '\0';

	return 0;
}

void *updateSongInfo(void *arg)
{
	while (true) {
		SongInfo *songInfo = (struct SongInfo *)arg;

		getSongInfo(songInfo->artist, songInfo->title);

		sleep(1);
	}

	return NULL;
}
