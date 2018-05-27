#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

struct config {
    int debug;
    char *src;
    bool windowed;
    bool geometry;
    float offX, offY;
    float width, height;
    float transparency;
    char *shaderName;
    unsigned int fps;
} cfg;

void printHelp();
bool parseArgs(int argc, char *argv[]);

#endif // _CONFIG_H_
