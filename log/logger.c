#include "logger.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

FILE* logfile;
time_t now_logger_time;
bool enable;

FILE* init_lead_logger(const char* path, const bool _enable){
    enable = _enable;
    if(enable){
        logfile = fopen(path, "w");
        return logfile;
    }
    return NULL;
}

void destruct(void){
    if(enable)
        fclose(logfile);
}
