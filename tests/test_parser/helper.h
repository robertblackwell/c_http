#ifndef c_http_tests_test_parser_helper_h
#define c_http_tests_test_parser_helper_h
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <http_in_c/unittest.h>
#include <http_in_c/logger.h>
#include <http_in_c/common/list.h>
#include <http_in_c/http/kvpair.h>
#include <http_in_c/http/header_list.h>
#include <http_in_c/http/message.h>

int run_list (ListRef tests);

#define CHECK_HEADER(h, K, V) do {\
    KVPairRef hlref = HdrList_find(h, HEADER_HOST); \
    UT_NOT_EQUAL_PTR(hlref, NULL); \
    UT_EQUAL_CSTR(KVPair_label(hlref), HEADER_HOST); \
    UT_EQUAL_CSTR(KVPair_value(hlref), "ahost"); \
} while(0);

#define CHECK_BODY(M, S) do {\
    BufferChainRef body = Message_get_body(M); \
    bool x##M = BufferChain_eq_cstr(body, S); \
    UT_EQUAL_INT(x##M, 1); \
} while(0);

#include "test_harness.h"

int test_requests();
int test_responses();
#endif