#ifndef c_eg_utils_h
#define c_eg_utils_h
#include <stdlib.h>
#include <assert.h>

#define ASSERT_NOT_NULL(ptr) assert(ptr != NULL);

char* make_upper(char* src);

#endif