#include <c_http/common/list.h>

typedef List PREFIX();
typedef ListRef PREFIX(Ref);
typedef ListIter PREFIX(Iter);
typedef ListIter PREFIX(Iterator);


PREFIX(Ref) PREFIX(_new) ();
void  PREFIX(_init)(PREFIX(Ref) lref);
void  PREFIX(_destroy)(PREFIX(Ref) lref);
void  PREFIX(_dispose)(PREFIX(Ref) *lref_adr);
void  PREFIX(_display)(const PREFIX(Ref) this);
int   PREFIX(_size)(const PREFIX(Ref) lref);
void  PREFIX(_add_back)(PREFIX(Ref) lref, TYPE* item);
void  PREFIX(_add_front)(PREFIX(Ref) lref, TYPE* item);
TYPE* PREFIX(_first)(const PREFIX(Ref) lref);
TYPE* PREFIX(_remove_first)(PREFIX(Ref) lref);
TYPE* PREFIX(_last)(const PREFIX(Ref) lref);
TYPE* PREFIX(_remove_last)(PREFIX(Ref) lref);
PREFIX(Iterator) PREFIX(_iterator)(const PREFIX(Ref) lref);
PREFIX(Iterator) PREFIX(_itr_next)(const PREFIX(Ref) lref, const PREFIX(Iterator) itr);
void PREFIX(_itr_remove) (PREFIX(Ref) lref, PREFIX(Iterator) *itr_adr);
TYPE* PREFIX(_itr_unpack) (PREFIX(Ref) lref, PREFIX(Iterator) itr);
PREFIX(Iterator) PREFIX(_find) (PREFIX(Ref) lref, void* needle);

