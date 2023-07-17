#ifndef HA_ERROR_H
#define HA_ERROR_H

#include <stdint.h>
#include <stddef.h>

typedef enum error_code
{
    SUCCESS,

    //POSIX errors
    MALLOC_ERROR,
    FOPEN_ERROR,

    //parser errors
    WRONG_IPADDR_ERROR,

    //hash table errors
    INSERT_ERROR,

    //network errors
    SOCKET_CREATION_ERROR,
    SOCKET_BIND_ERROR,
    SOCKET_LISTEN_ERROR,
    SOCKET_ACCEPT_ERROR,
    SOCKET_CONNECT_ERROR,

    //collection errors
    OUT_OF_BOUND_ERROR,

} error_code_t;

typedef struct error
{
    error_code_t code;
    size_t line;
    const char* message;
    const char* file;
} pl_error_t;

/*
 * THESE MACROSES ARE NOT THREAD-SAFE 
*/

extern pl_error_t pl_error;
extern char error_string[1024];
extern char message_string[1024];

/*
 * Note: error is type pl_error_t.
 */
#define RETURN_SUCCESS() return (pl_error_t){.code = 0,                     \
                                             .line = __LINE__,              \
                                             .file = __FILE__,              \
                                             .message = "everything is OK"  \
                                             }

#define error_info(error) sprintf(error_string, "error code:%d, line:%ld file:%s message:'%s'",  \
                          error.code, error.line, error.file, error.message);                    \

#define THROW(_code, ...) sprintf(message_string, __VA_ARGS__);                     \
                          return (pl_error_t){.code = _code,                        \
                                             .line = __LINE__,                      \
                                             .file = __FILE__,                      \
                                             .message = message_string              \
                                             } \

#define POSIX_THROW(_code) return (pl_error_t){.code = _code,                   \
                                               .line = __LINE__,                \
                                               .file = __FILE__,                \
                                               .message = strerror(errno)       \
                                               }                                \
                    
/*
 * Note: expr must return pl_error_t.
 *
 * calls elog with LOG level. Don't interrupt the program execution
 */
#define SAFE(expr)  pl_error = expr;                                                   \
                    if((pl_error.code) != SUCCESS) {                                   \
                        error_info(pl_error);                                          \
                        elog(LOG, "%s", error_string);                                 \
                    }                                                                  \

/*
 * Note: expr must return pl_error_t.
 *
 * calls elog with FATAL level. Abort proccess
 */
#define STRICT(expr) pl_error = expr;                                                \
                     if((pl_error.code) != SUCCESS) {                                \
                         error_info(pl_error);                                       \
                         elog(FATAL, "%s", error_string);                            \
                     }                                                               \

void print_error_info(error_code_t, const char*);

#endif /* HA_ERROR_H */
