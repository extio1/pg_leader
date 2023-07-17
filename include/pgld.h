#ifndef PGHA_H
#define PGHA_H

#include "postgres.h"
#include "error.h"

/*
 *
 */
PGDLLEXPORT pl_error_t node_routine(Datum);

#endif  /* PGHA_H */