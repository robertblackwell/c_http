#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <http-parser/http_parser.h>
#include <c_http/common/alloc.h>
#include <c_http/unittest.h>
#include <c_http/common/cbuffer.h>
#include <c_http/logger.h>
#include <c_http/common/list.h>
#include <c_http/common/http_parser/rdsocket.h>
#include <c_http/sync/sync_server.h>
#include <c_http/common/hdrlist.h>
#include <c_http/common/message.h>

void testfunc()
{
    printf("testfunc \n");
}
typedef void(*f_t)();

typedef struct T_s {
    f_t f;
} T, *TRef;

int test_url_01()
{
    T t;
    t.f = &(testfunc);
    f_t afunc = &(testfunc);
    afunc();
    t.f();
    return 0;
}
int test_rdsock()
{
    RdSocket realrdsock = RealSocket(33);
    RdSocket datasourcerdsock = DataSourceSocket(NULL);
    ReadFunc rf3 = (realrdsock.read_f);
    int b3 = rf3(NULL, (void*)200, 300);
    int r1 = realrdsock.read_f(realrdsock.ctx, NULL, 10);
}
int main()
{
//    UT_ADD(test_url_01);
    UT_ADD(test_rdsock);
    int rc = UT_RUN();
    return rc;
}