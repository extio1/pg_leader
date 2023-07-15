#ifndef HA_PARSER_H
#define HA_PARSER_H

#include "error.h"
#include "ha_types.h"

error_code_t parse_cluster_config(const char* path, cluster_t* cluster);
error_code_t parse_node_config(const char* path, node_t* node);

#endif /* HA_PARSER_H */