#include "../include/hashtable.h"
#include "../include/error.h"

hash_table*
init(void){
    return g_hash_table_new(g_int_hash, g_int_equal);
}

error_code_t
insert(hash_table*, key_t, value_t){
    return SUCCESS;
}

value_t
get(hash_table*, key_t){
}