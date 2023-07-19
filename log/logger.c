#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

FILE* logfile;
time_t now_logger_time;

FILE* init_lead_logger(const char* path){
    logfile = fopen(path, "w");
    return logfile;
}

void destruct(void){
    fclose(logfile);
}
