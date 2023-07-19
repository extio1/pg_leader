#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern FILE* logfile;
extern time_t now_logger_time;

FILE* init_lead_logger(const char*);
void destruct(void);

#define leadlog(tag, ...) time(&now_logger_time);                                           \
                          fprintf(logfile, "%s [%s]:", ctime(&now_logger_time), tag);       \
                          fprintf(logfile, __VA_ARGS__);                                    \
                          fprintf(logfile, "\n");                                           \
                          fflush(logfile);                                                  \

#endif /* LOGGER_H */