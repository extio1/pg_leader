#ifndef HA_PARSER_H
#define HA_PARSER_H

#include "error.h"
#include "ld_types.h"

#include <stdbool.h>

pl_error_t parse_cluster_config(cluster_t* cluster);
pl_error_t parse_node_config(node_t* node);
pl_error_t parse_timeout_config(node_t* node);
pl_error_t parse_logger_config(char** path, bool* enable);
pl_error_t parse_visualization_config(char** path);

#endif /* HA_PARSER_H */