#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>  // mkdir
#include <errno.h>
#include "log.h"

#define LOG_DIR "/usr/local/bin/logs_bat"

FILE *logfp = NULL;
char current_date[16] = {0};   // YYYY-MM-DD

void log_open_file()
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char today[16];
    sprintf(today, "%04d-%02d-%02d",
            t->tm_year + 1900,
            t->tm_mon + 1,
            t->tm_mday);

    // 如果日期沒變，直接回
    if (strcmp(today, current_date) == 0 && logfp != NULL)
        return;

    // 關閉舊檔
    if (logfp) fclose(logfp);

    strcpy(current_date, today);

    // 建立 LOG_DIR 資料夾（如果不存在）
    struct stat st = {0};
    if (stat(LOG_DIR, &st) == -1) {
        if (mkdir(LOG_DIR, 0777) == -1) {
            perror("mkdir LOG_DIR failed");
            return;
        }
    }

    // 建立檔名
    char filename[256];
    sprintf(filename, "%s/%s.log", LOG_DIR, current_date);

    logfp = fopen(filename, "a");
    if (!logfp) {
        perror("open log file failed");
    }
}


void log_write(const char *fmt, ...)
{
    log_open_file();  // 自動切換檔案

    if (!logfp) return;

    // 加 timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(logfp, "[%02d:%02d:%02d] ",
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(logfp, fmt, args);
    va_end(args);

    fprintf(logfp, "\n");
    fflush(logfp);  // 立即寫入
}

void log_close()
{
    if (logfp) {
        fclose(logfp);
        logfp = NULL;
    }
}

