#include "../include/error.h"

#include "postgres.h"
#include <stdio.h>

void print_error_info(error_code_t error, const char* msg){
    switch(error){
        case MALLOC_ERROR:
            elog(ERROR, "malloc error:%s", msg);
            break;
        case FOPEN_ERROR:
            elog(ERROR, "fopen error:%s", msg);
            break;
        default:
            elog(ERROR, "some error occured");
            break;
    }
}