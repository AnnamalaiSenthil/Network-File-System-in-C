#ifndef __SS_FUNCTIONS_H__
#define __SS_FUNCTIONS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

int createDirectory(const char *dirPath);
int createFile(const char *filePath);
int createFilePath(const char *inputPath);
int createDirPath(const char *inputPath);
int removeLastFile(const char *inputPath);
int removeLastDir(const char *inputPath);
int deleteFolder(char *folderPath);
int deleteFile(const char *filePath);

#endif