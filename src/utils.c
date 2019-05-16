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
	char newArtist[128], newTitle[128];
	buff[0] = newArtist[0] = newTitle[0] = '\0';

    // Get artist
	if (exec("playerctl metadata artist", buff, sizeof(buff))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	if (strlen(buff))
		strcpy(newArtist, buff);
	
    // Get title
	if (exec("playerctl metadata title", buff, sizeof(buff))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	if (strlen(newArtist)) {
		strcpy(artist, newArtist);
		strcpy(title, buff);
		return 1;
	}

	if (!strlen(buff)) {
		artist[0] = title[0] = '\0';
		exec("rm --force image.jpg", buff, sizeof(buff));
		return 2;
	}

	bool dash = false;
	int artistPos = 0, titlePos = 0;
	for (int i = 0; i < strlen(buff); i++) {
		if (i && buff[i-1] == ' ' && buff[i] == '-' && buff[i+1] == ' ') {
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

	if (strlen(newTitle) && newTitle[strlen(newTitle)-1] == ' ')
		newTitle[strlen(newTitle)-1] = '\0';

	if (strlen(newArtist) && newArtist[strlen(newArtist)-1] == ' ')
		newArtist[strlen(newArtist)-1] = '\0';

	if (!strcmp(artist, newArtist) && !strcmp(title, newTitle))
		return 0;

	strcpy(artist, newArtist);
	strcpy(title, newTitle);

	return 1;
}

void getAlbumArt(SongInfo *songInfo) {
	char *cmd = NULL;
	char buff[128];

    if (!cfg.onlyYT) {
        exec("playerctl metadata mpris:artUrl", buff, sizeof(buff));

        if (strlen(buff)) {
            cmd = (char *)malloc(strlen("curl -Ls --output image.jpg ") + strlen(buff));

            sprintf(cmd, "curl -Ls --output image.jpg %s", buff);

            if (!exec(cmd, buff, sizeof(buff))) {
		        songInfo->newAlbumArt = true;
                return;
            }
        }

		cmd = (char *)malloc(20 + strlen(songInfo->artist) + strlen(songInfo->title));

		sprintf(cmd, "./albumArt \"%s %s\"", songInfo->artist, songInfo->title);

		if (exec(cmd, buff, sizeof(buff))) {
			printf("albumArt script not found\n");
			return;
		}
	
		if (cfg.debug && strlen(buff))
			printf("albumArt script output (%d): %s\n", (int)strlen(buff), buff);

        if (buff[0] != 'A') {
            songInfo->newAlbumArt = true;
            if (cfg.debug)
                printf("Found album art using discorg\n");

            return;
        }
    }

    if (cfg.debug)
        printf("No album art found, using youtube thumbnail\n");

    char videoID[12];

    if (exec("playerctl metadata xesam:url", buff, sizeof(buff)) || !strlen(buff)) {
        if (cfg.debug)
            printf("No youtube video ID\n");
        return;
    }

    int buffLenght = strlen(buff);
    if (cfg.debug)
        printf("TrackID (%d): %s\n", buffLenght, buff);

    if (buffLenght < 17) {
        if (cfg.debug)
            printf("No youtube video ID\n");
        return;
    }

    int start = buffLenght;

    if (cfg.plasma) {
        while (--start && ((buff[start] != '?' && buff[start] != '&') || buff[start+1] != 'v' || buff[start+2] != '='));

        if (!start)
            return;
        start += 2;

        for (int i = start + 1, j = 0; i < buffLenght && j < 12; i++, j++)
            videoID[j] = buff[i];
    } else {
        while (buff[--start] != '/');

        for (int i = start + 1, j = 0; i < buffLenght - 1 && j < 12; i++, j++) {
            if (buff[i] == '_') {
                if (buff[++i] == 'd')
                    videoID[j] = '-';
                else if (buff[++i] == 'u')
                    videoID[j] = '_';
            } else {
                videoID[j] = buff[i];
            }
        }
    }
    
    videoID[11] = '\0';

    if (cfg.debug) 
        printf("Video ID: %s\n", videoID);

    free(cmd);
    cmd = (char *)malloc(strlen("curl -o image.jpg -s https://i.ytimg.com/vi/zuJV-DAv_wE/maxresdefault.jpg") + 1);
    sprintf(cmd, "curl -o image.jpg -s https://i.ytimg.com/vi/%s/maxresdefault.jpg", videoID);

    if (exec(cmd, buff, sizeof(buff))) {
        if (cfg.debug)
            printf("Problem with downloding youtube thumbnail\n");
        return;
    }

    songInfo->newAlbumArt = true;

	free(cmd);
}

void *updateSongInfo(void *arg)
{
	SongInfo *songInfo = (struct SongInfo *)arg;

	while (*songInfo->cont) {
		int status = getSongInfo(songInfo->artist, songInfo->title);
		if (status == -1)
			return NULL;
		
		if (cfg.debug)
			printf("Artist: %s\nTitle: %s\n", songInfo->artist, songInfo->title);

		if (status == 1)
			getAlbumArt(songInfo);
		else if (status == 2)
			songInfo->newAlbumArt = true;

		sleep(1);
	}

	return NULL;
}
