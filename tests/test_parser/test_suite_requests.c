
#include "helper.h"
/**
 * This file contains a number of parsing tests for request messages
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// PERR01  request with error no minor version
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int vfunc_perr01 (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    UT_EQUAL_INT(rref->rc, HPE_INVALID_VERSION);
    UT_EQUAL_PTR(rref->message, NULL);
    return 0;
//    UT_EQUAL_CSTR(Message_get_reason(m1), "OK 11Reason Phrase");
#ifdef A_ON
    CHECK(h.at_key(HeaderFields::Host));
    CHECK(h.at_key(HeaderFields::Connection));
    CHECK(h.at_key(HeaderFields::ProxyConnection));
    CHECK(h.at_key(HeaderFields::ContentLength));
    CHECK(h.at_key(HeaderFields::Host).get() == "ahost");
    CHECK(h.at_key(HeaderFields::Connection).get() == "keep-alive");
    CHECK(h.at_key(HeaderFields::ProxyConnection).get() == "keep-alive");
    CHECK(h.at_key(HeaderFields::ContentLength).get() == "11");
    CHECK(m1->get_body_buffer_chain()->to_string() == "01234567890");
    return 0;
#endif
}
static parser_test_t* test_case_perr01() {
// A0011
    static const char *description = "perr01 parser error no minor version";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "Content-length: 0\r\n\r\n",
            NULL
    };
    return parser_test_new(description, lines, vfunc_perr01);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// IOERR01  request simulated IO error
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ioerr01_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    UT_EQUAL_PTR(rref->message, NULL);
    UT_EQUAL_INT(rref->rc, HPE_USER);
    return 0;
//    UT_EQUAL_CSTR(Message_get_reason(m1), "OK 11Reason Phrase");
#ifdef A_ON
    CHECK(h.at_key(HeaderFields::Host));
    CHECK(h.at_key(HeaderFields::Connection));
    CHECK(h.at_key(HeaderFields::ProxyConnection));
    CHECK(h.at_key(HeaderFields::ContentLength));
    CHECK(h.at_key(HeaderFields::Host).get() == "ahost");
    CHECK(h.at_key(HeaderFields::Connection).get() == "keep-alive");
    CHECK(h.at_key(HeaderFields::ProxyConnection).get() == "keep-alive");
    CHECK(h.at_key(HeaderFields::ContentLength).get() == "11");
    CHECK(m1->get_body_buffer_chain()->to_string() == "01234567890");
    return 0;
#endif
}
static parser_test_t* test_case_ioerr01() {
    static const char *description = "A0012 simulated io error";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "error",
            NULL
    };
    return parser_test_new(description, lines, test_ioerr01_vfunc);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK001  response OK 200 with body and content length 10
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK001_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    HdrListRef h = Message_get_headerlist (m1);

//    UT_EQUAL_INT(Message_get_status (m1), 200);
//    UT_EQUAL_CSTR(Message_get_reason (m1), "OK 11Reason Phrase");
    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
    UT_EQUAL_CSTR(Message_get_target(m1), "/target");
    CHECK_HEADER(h, HEADER_HOST, "ahost");
    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
    CHECK_HEADER(h, HEADER_CONTENT_LENGTH, "11");
    BufferChainRef body1 = Message_get_body (m1);
    bool x = BufferChain_eq_cstr (body1, "01234567890");
    CHECK_BODY(m1, "01234567890");
    return 0;
}
static parser_test_t* test_case_ROK001() {
    static const char *description = "A001 response 200 with body and content length 10";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "Content-length: 11\r\n\r\n",
            (char *) "01234567890",
            NULL
    };
    return parser_test_new(description, lines, test_ROK001_vfunc);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK002  response OK 201 with body and content length 10 some body data in header buffer
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK002_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    HdrListRef h = Message_get_headerlist (m1);
    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
    UT_EQUAL_CSTR(Message_get_target(m1), "/target");

    CHECK_HEADER(h, HEADER_HOST, "ahost");
    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
    CHECK_HEADER(h, HEADER_CONTENT_LENGTH, "10");
    CHECK_BODY(m1, "ABCDEFGHIJ");

    return 0;
}
static parser_test_t* test_case_ROK002() {
    static const char *description = "ROK002 response 201 body length 10 SOME body data in header buffer";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "Content-length: 10\r\n\r\nAB",
            (char *) "CDEFGHIJ",
            NULL
    };
    return parser_test_new(description, lines, test_ROK002_vfunc);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK003  response OK 201 with body and content length 10 data in same buffer as black header line
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK003_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    HdrListRef h = Message_get_headerlist (m1);
    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
    UT_EQUAL_CSTR(Message_get_target(m1), "/target");

    CHECK_HEADER(h, HEADER_HOST, "ahost");
    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
    CHECK_HEADER(h, HEADER_CONTENT_LENGTH, "10");
    CHECK_BODY(m1, "ABCDEFGHIJ");

    KVPairRef hlref_host = HdrList_find (h, HEADER_HOST);
    UT_EQUAL_CSTR(KVPair_label (hlref_host), HEADER_HOST);
    UT_EQUAL_CSTR(KVPair_value (hlref_host), "ahost");

    KVPairRef hlref_connection = HdrList_find (h, HEADER_CONNECTION_KEY);
    UT_EQUAL_CSTR(KVPair_label (hlref_connection), HEADER_CONNECTION_KEY);
    UT_EQUAL_CSTR(KVPair_value (hlref_connection), "keep-alive");

    KVPairRef hlref_proxy_connection = HdrList_find (h, HEADER_PROXYCONNECTION);
    UT_EQUAL_CSTR(KVPair_label (hlref_proxy_connection), HEADER_PROXYCONNECTION);
    UT_EQUAL_CSTR(KVPair_value (hlref_proxy_connection), "keep-alive");

    KVPairRef hlref_content_length = HdrList_find (h, HEADER_CONTENT_LENGTH);
    UT_EQUAL_CSTR(KVPair_label (hlref_content_length), HEADER_CONTENT_LENGTH);
    UT_EQUAL_CSTR(KVPair_value (hlref_content_length), "10");
    return 0;
};
static parser_test_t* test_case_ROK003()
{
    static const char *description = "ROK003 response 201 body length 10 SOME body data in with blank line buffer EOH and EOM at same time";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "Content-length: 10",
            (char *) "\r\n\r\nABCDEFGHIJ",
            NULL
    };
    return parser_test_new(description, lines, test_ROK003_vfunc);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK004  response OK 200 with body and content length 10
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK004_vfunc (ListRef results) {
    test_output_t* rref = (test_output_t*) List_remove_first(results);
    MessageRef m1 = rref->message;
    HdrListRef h = Message_get_headerlist(m1);
    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
    UT_EQUAL_CSTR(Message_get_target(m1), "/target");

    CHECK_HEADER(h, HEADER_HOST, "ahost");
    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
    CHECK_HEADER(h, HEADER_TRANSFERENCODING, "chunked");
    CHECK_BODY(m1, "12345678901234567890XXXXX12345678901234567890HGHGH1234567890");
    return 0;
}
static parser_test_t* test_case_ROK004() {
    static const char *description = "ROK004 response 201 body chunked encoding NO body data in header buffer";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "Transfer-Encoding: chunked\r\n\r\n",
            (char *) "0a\r\n1234567890\r\n",
            (char *) "0f\r\n1234567890XXXXX\r\n",
            (char *) "0a\r\n1234567890\r\n",
            (char *) "0f\r\n1234567890HGHGH\r\n",
            (char *) "0a\r\n1234567890\r\n",
            (char *) "0\r\n",
            (char *) "\r\n",
            NULL
    };
    return parser_test_new(description, lines, test_ROK004_vfunc);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK005  response OK 201 chunked encoding - some body data in buffer with blank header line
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK005_vfunc (ListRef results);
static parser_test_t* test_case_ROK005() {
    static const char *description = "ROK005 response  201 body chunked encoding SOME body data in buffer with blank line after header";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "Transfer-Encoding: chunked\r\n",
            (char *) "\r\n0a\r\n1234567890\r\n",
            (char *) "0f\r\n1234567890XXXXX\r\n",
            (char *) "0a\r\n1234567890\r\n",
            (char *) "0f\r\n1234567890HGHGH\r\n",
            (char *) "0a\r\n1234567890\r\n",
            (char *) "0\r\n",
            (char *) "\r\n",
            NULL
    };
    return parser_test_new(description, lines, test_ROK005_vfunc);
}
static int test_ROK005_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    HdrListRef h = Message_get_headerlist (m1);
    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
    UT_EQUAL_CSTR(Message_get_target(m1), "/target");

    CHECK_HEADER(h, HEADER_HOST, "ahost");
    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
    CHECK_HEADER(h, HEADER_TRANSFERENCODING, "chunked");
    CHECK_BODY(m1, "12345678901234567890XXXXX12345678901234567890HGHGH1234567890");

    return 0;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK006  response OK 201 simple chunked body
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK006_vfunc (ListRef results);
static parser_test_t* test_case_ROK006() {
    static const char *description = "ROK006 simple 201 body chunked - chunks spread over different buffers";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep-alive\r\n",
            (char *) "Transfer-Encoding: chunked\r\n",
            (char *) "\r\n0a\r\n123456",
            (char *) "7890\r\n",
            (char *) "0f\r\n123456",
            (char *) "7890XXXXX\r\n0a\r\n1234567890\r\n",
            (char *) "0f\r\n1234567890HGHGH\r\n",
            (char *) "0a\r\n1234567890\r\n",
            (char *) "0\r\n",
            (char *) "\r\n",
            NULL
    };
    return parser_test_new(description, lines, test_ROK006_vfunc);
}
static int test_ROK006_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    HdrListRef h = Message_get_headerlist (m1);
    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
    UT_EQUAL_CSTR(Message_get_target(m1), "/target");

    CHECK_HEADER(h, HEADER_HOST, "ahost");
    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
    CHECK_HEADER(h, HEADER_TRANSFERENCODING, "chunked");
    CHECK_BODY(m1, "12345678901234567890XXXXX12345678901234567890HGHGH1234567890");

    return 0;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK007  request and response back to back
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK007_vfunc (ListRef results);
static parser_test_t* test_case_ROK007()
{
    static const char *description = "ROK007 request and response back to back ";
    static const char *lines[] = {
        (char *) "GET /target HTTP/1.1\r\n",
        (char *) "Host: ahost\r\n",
        (char *) "Connection: keep-alive\r\n",
        (char *) "Proxy-Connection: keep\0    ",
        (char *) "-alive\r\n\0    ",
        (char *) "Content-length: 10\r\n\r\n",
        (char *) "1234567",
        (char *) "890GET /target ",
        (char *) "HTTP/1.1\r\n",
        (char *) "Host: ahost\r\n",
        (char *) "Connection: keep-alive\r\n",
        (char *) "Proxy-Connection: keep-alive\r\n",
        (char *) "Content-length: 11\r\n",
        (char *) "\r\n",
        (char *) "ABCDEFGHIJK\0      ",
        NULL
    };
    return parser_test_new(description, lines, test_ROK007_vfunc);
}
static int test_ROK007_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    test_output_t* rref2 = (test_output_t*) List_remove_first (results);
    MessageRef m2 = rref2->message;
    UT_NOT_EQUAL_PTR(m1, m2);
    UT_NOT_EQUAL_PTR(m1, NULL);
    UT_NOT_EQUAL_PTR(m2, NULL);
    HdrListRef h1 = Message_get_headerlist (m1);
    HdrListRef h2 = Message_get_headerlist (m2);
    UT_NOT_EQUAL_PTR(h1, h2);
    UT_NOT_EQUAL_PTR(h1, NULL);
    UT_NOT_EQUAL_PTR(h2, NULL);
    {
        HdrListRef h = Message_get_headerlist (m1);
        UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
        UT_EQUAL_CSTR(Message_get_target(m1), "/target");

        CHECK_HEADER(h, HEADER_HOST, "ahost");
        CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
        CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
        CHECK_HEADER(h, HEADER_CONTENT_LENGTH, "10");
        BufferChainRef bcref = Message_get_body (m1);
        CHECK_BODY(m1, "1234567890");
    }
    {
        HdrListRef h = Message_get_headerlist (m2);
        UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
        UT_EQUAL_CSTR(Message_get_target(m1), "/target");

        CHECK_HEADER(h, HEADER_HOST, "ahost");
        CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
        CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
        CHECK_HEADER(h, HEADER_CONTENT_LENGTH, "11");

        BufferChainRef bcref = Message_get_body(m2);
        IOBufferRef iobref = BufferChain_compact(bcref);
        bool x01 = BufferChain_eq_cstr(bcref, "ABCDEFGHIJK");
        int y = x01;
        CHECK_BODY(m2, "ABCDEFGHIJK");
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK008  request GET with Transfer-Encoding: identity/dummy and No content-length
// Gives an error "invalid transfer encoding" Not convinced this is correct
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK008_vfunc (ListRef results);
static parser_test_t* test_case_ROK008() {
// A008
    static const char *description = "ROK008 No content-length, has transfer encoding but not chunked. Parsing should make content-length: 10";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Transfer-encoding: identity\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep\0    ",
            (char *) "-alive\r\n\0    ",
            (char *) "\r\n",
            (char *) "1234567890",
            (char *) NULL,
    };
    return parser_test_new(description, lines, test_ROK008_vfunc);
}
static int test_ROK008_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    UT_EQUAL_PTR(m1, NULL);
    UT_NOT_EQUAL_INT(rref->rc, HPE_OK);
    return 0;
//    HdrListRef h = Message_get_headerlist (m1);
//    int n = HdrList_size (h);
//    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
//    UT_EQUAL_CSTR(Message_get_target(m1), "/target");
//
//    KVPairRef hlr = HdrList_find (h, HEADER_HOST);
//    CHECK_HEADER(h, HEADER_HOST, "ahost");
//    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
//    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
//    const char* x = Message_get_header_value(m1, HEADER_CONTENT_LENGTH);
//    UT_EQUAL_PTR(x, NULL);
//    UT_EQUAL_PTR(Message_get_body(m1), NULL);
//
//    test_output_t* rref2 = (test_output_t*) List_remove_first(results);
//    UT_EQUAL_INT(0, List_size(results));
//    UT_EQUAL_PTR(rref2->message, NULL);
//    UT_NOT_EQUAL_INT(rref2->rc, HPE_OK);
//    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ROK009  request GET without Content-Length or Transfer-Encoding: chunked
// A stringt http1.1 parsing is allowed to reject this request.
// However as shown below - this sequence is parsed as two messages:
// -    the first one is a GET with no body
// -    the second is an invalid message
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static int test_ROK009_vfunc (ListRef results);
static parser_test_t* test_case_ROK009() {
    static const char *description = "ROK009 No content-length, no transfer encoding. Parses correctly";
    static const char *lines[] = {
            (char *) "GET /target HTTP/1.1\r\n",
            (char *) "Host: ahost\r\n",
            (char *) "Connection: keep-alive\r\n",
            (char *) "Proxy-Connection: keep\0    ",
            (char *) "-alive\r\n\0    ",
            (char *) "\r\n123",
            (char *) "4567890",
            (char *) NULL,
    };
    return parser_test_new(description, lines, test_ROK009_vfunc);
}
static int test_ROK009_vfunc (ListRef results)
{
    test_output_t* rref = (test_output_t*) List_remove_first (results);
    MessageRef m1 = rref->message;
    UT_NOT_EQUAL_PTR(m1, NULL);
    UT_EQUAL_INT(rref->rc, HPE_OK);
    HdrListRef h = Message_get_headerlist (m1);
    int n = HdrList_size (h);
    UT_EQUAL_INT(Message_get_method(m1), HTTP_GET);
    UT_EQUAL_CSTR(Message_get_target(m1), "/target");

    KVPairRef hlr = HdrList_find (h, HEADER_HOST);
    CHECK_HEADER(h, HEADER_HOST, "ahost");
    CHECK_HEADER(h, HEADER_CONNECTION_KEY, "keep-alive");
    CHECK_HEADER(h, HEADER_PROXYCONNECTION, "keep-alive");
    const char* x = Message_get_header_value(m1, HEADER_CONTENT_LENGTH);
    UT_EQUAL_PTR(x, NULL);
    UT_EQUAL_PTR(Message_get_body(m1), NULL);

    test_output_t* rref2 = (test_output_t*) List_remove_first(results);
    UT_EQUAL_INT(0, List_size(results));
    UT_EQUAL_PTR(rref2->message, NULL);
    UT_NOT_EQUAL_INT(rref2->rc, HPE_OK);

    return 0;
}

static ListRef make_request_tests_A ()
{
    ListRef tl = List_new (NULL);
    List_add_back (tl, test_case_ROK001());
    List_add_back (tl, test_case_ROK002());
    List_add_back (tl, test_case_ROK003());
    List_add_back (tl, test_case_ROK004());
    List_add_back (tl, test_case_ROK005());
    List_add_back (tl, test_case_ROK006());
    List_add_back (tl, test_case_ROK007());
    List_add_back (tl, test_case_ROK008());
    List_add_back (tl, test_case_ROK009());
    List_add_back (tl, test_case_perr01());
    List_add_back (tl, test_case_ioerr01());
    return tl;
}

static ListRef make_request_tests_B ()
{
    ListRef tl = List_new (NULL);
    List_add_back (tl, test_case_perr01());
    return tl;
}
int test_requests()
{
    return run_list(make_request_tests_A());
}