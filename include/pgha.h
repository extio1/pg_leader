#ifndef PGHA_H
#define PGHA_H

#include "postgres.h"
#include "error.h"

/*
 *
 */
PGDLLEXPORT error_code_t node_routine(Datum);

#endif  /* PGHA_H */