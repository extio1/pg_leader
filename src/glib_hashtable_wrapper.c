/*

#include "../include/hashtable.h"
#include "../include/error.h"

//#include <glib.h>

error_code_t
init(hash_table* table){
    //table = g_hash_table_new(g_int_hash, g_int_equal);
    return SUCCESS;
}

error_code_t
insert(hash_table* table, key_t* key, value_t* value){
   // if( g_hash_table_insert(table, key, value) == TRUE){
        return SUCCESS;
    //} else {
    //    return INSERT_ERROR;
    //}
}

value_t*
get(hash_table* table, const key_t* key){
    return g_hash_table_lookup(table, key);
}

*/