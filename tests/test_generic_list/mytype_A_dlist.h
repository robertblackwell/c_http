#ifndef C_HTTP_MYTYPE_A_DLIST_H
#define C_HTTP_MYTYPE_A_DLIST_H
#include "mytype_A.h"

#undef TYPE
#undef TYPED
#undef PREFIX
#define TYPE MyTypeA
#define TYPED(THING) MyTypeA##THING
#define PREFIX(THING) MyTypeAChain##THING
#include <http_in_c/common/generics/dlist_template.h>


#endif