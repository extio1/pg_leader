#ifndef HA_ERROR_H
#define HA_ERROR_H

typedef enum error_code
{
    SUCCESS,

    //POSIX errors
    MALLOC_ERROR,
    FOPEN_ERROR,

    //parser errors
    WRONG_IPADDR_ERROR,

    //hash table errors
    INSERT_ERROR

} error_code_t;

#endif /* HA_ERROR_H */
