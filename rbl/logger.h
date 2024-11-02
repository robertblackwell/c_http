#ifndef rbl_logger_h
#define rbl_logger_h

#include <stdio.h>
extern void rbl_log_function(char* level, const char* funcname, const char* filename, int line, char* message);

// RBL_LOG_ALLOW_GLOBAL - is a global switch to turn logging off globally
#define CHLOG_GLOBALX

// CHLOG_ON - each c/c++ file can turn logging on by defining CHLOG_ON before including logger.h
// this only turns logging on if the global switch is defined
//
// RBL_LOG_ERROR - error logging is always on
//
#define RBL_LOG_ERROR(f_, ...) do {\
		char* s; \
		int c  = asprintf(&s, f_, ##__VA_ARGS__); \
		rbl_log_function("ERR", __FUNCTION__, __FILE__, __LINE__, s); \
		free(s); \
	} while(0);

#define RBL_LOGFMT(f_, ...) do {\
		char* s; \
		int c  = asprintf(&s, f_, ##__VA_ARGS__); \
		rbl_log_function("LOG", __FUNCTION__, __FILE__, __LINE__, s); \
		free(s); \
	} while(0);


#if defined(RBL_LOG_ENABLE) && defined(RBL_LOG_ALLOW_GLOBAL)
	#define RBL_LOG_PRINTF(f_, ...) printf((f_), ##__VA_ARGS__)
	#define RBL_LOG_FMT(f_, ...) do {\
		char* s; \
		int c  = asprintf(&s, f_, ##__VA_ARGS__); \
		rbl_log_function("LOG", __FUNCTION__, __FILE__, __LINE__, s); \
		free(s); \
	} while(0);

    #define RBL_LOG_ENTRY() do {\
		rbl_log_function("ENTR", __FUNCTION__, __FILE__, __LINE__, ""); \
	} while(0);

    #define RBL_LOG_MSG(m)  do {\
		rbl_log_function("MSG", __FUNCTION__, __FILE__, __LINE__, m); \
	} while(0);

#else
	#define RBL_LOG_PRINTF(f_, ...)
	#define RBL_LOG_FMT(f_, ...)
    #define RBL_LOG_ENTRY()
    #define RBL_LOG_MSG(m)
#endif
#endif