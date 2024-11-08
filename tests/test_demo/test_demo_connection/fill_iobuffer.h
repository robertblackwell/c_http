//
// Created by robert on 11/8/24.
//

#ifndef C_HTTP_FILL_IOBUFFER_H
#define C_HTTP_FILL_IOBUFFER_H
#include <http_in_c/common/iobuffer.h>
#include <http_in_c/common/buffer_chain.h>
#include <http_in_c/demo_protocol/demo_message.h>
IOBufferRef fill_iobuffer(char* line, size_t max_len, size_t requested_len);

DemoMessageRef fill_demo_message(char* line, size_t max_len, size_t rqeuested_length);

#endif //C_HTTP_FILL_IOBUFFER_H
