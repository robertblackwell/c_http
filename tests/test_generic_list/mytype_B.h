#ifndef C_HTTP_MYTYPE_B_H
#define C_HTTP_MYTYPE_B_H

typedef struct MyTypeB_s {
    float b;
} MyTypeB, *MyTypeBRef;

MyTypeBRef MyTypeB_new();
void MyTypeB_free(MyTypeBRef this);
void MyTypeB_dealloc(void** this_ptr);
void MyTypeB_dispose(MyTypeB** ptr);
void MyTypeB_display(MyTypeBRef this);
#endif