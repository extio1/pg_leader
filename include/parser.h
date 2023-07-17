#ifndef HA_PARSER_H
#define HA_PARSER_H

#include "error.h"
#include "ld_types.h"

pl_error_t parse_cluster_config(const char* path, cluster_t* cluster);
pl_error_t parse_node_config(const char* path, node_t* node);
pl_error_t parse_timeout_config(const char* path, node_t* node);

#endif /* HA_PARSER_H */