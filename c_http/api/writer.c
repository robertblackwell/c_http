#define _GNU_SOURCE
#include <c_http/api/writer.h>
#include <c_http/dsl/alloc.h>
#include <c_http/test_helpers/message_private.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <c_http/details/ll_parser_types.h>

struct Writer_s {
    WRITER_DECLARE_TAG;
    int m_sock;
};

void Writer_init(WriterRef this, int sock)
{
    WRITER_SET_TAG(this)
    this->m_sock = sock;
}
WriterRef Writer_new(int socket)
{
    WriterRef mwref = eg_alloc(sizeof(Writer));
    if(mwref == NULL)
        return NULL;
    Writer_init(mwref, socket);
    return mwref;
}
void Writer_destroy(WriterRef this)
{
    WRITER_CHECK_TAG(this)
}
void Writer_dispose(WriterRef* this_ptr)
{
    WriterRef this = *(this_ptr);
    WRITER_CHECK_TAG(this)
    Writer_destroy(this);
    eg_free(*this_ptr);
    *this_ptr = NULL;
}

void Writer_write(WriterRef this, MessageRef msg_ref)
{
    IOBufferRef serialized = Message_serialize(msg_ref);
    Writer_write_chunk(this, IOBuffer_data(serialized), IOBuffer_data_len(serialized));
}
/**
 *
 * Initiates the writing of a http response by sending status and headers.
 *
 * \param this    WriterRef contains the socket/fd for writing
 * \param status  HttpStatus enum value
 * \param headers HdrListRef - the deaders to be written
 * \result (TODO) success, EOF-closed by other end, IO error
 */
void Writer_start(WriterRef this, HttpStatus status, HdrListRef headers)
{
    WRITER_CHECK_TAG(this)
    char* first_line = NULL;
    CbufferRef cb_output_ref = NULL;
    CbufferRef serialized_headers = NULL;
    void* return_value = NULL;

    const char* reason_str = http_status_str(status);
    int len = asprintf(&first_line, "HTTP/1.1 %d %s\r\n", status, reason_str);
    if(first_line == NULL) goto failed;

    if((cb_output_ref = Cbuffer_new()) == NULL) goto failed;

    serialized_headers = HdrList_serialize(headers);
    if(serialized_headers == NULL) goto failed;

    Cbuffer_append(cb_output_ref, (void*)first_line, len);
    Cbuffer_append(cb_output_ref, Cbuffer_data(serialized_headers), Cbuffer_size(serialized_headers));
    Cbuffer_append_cstr(cb_output_ref, "\r\n");
    Writer_write_chunk(this, Cbuffer_data(cb_output_ref), Cbuffer_size(cb_output_ref));

    return_value = (void*)1;
    failed:
        if(first_line != NULL) free(first_line);
        if(serialized_headers != NULL) Cbuffer_dispose(&serialized_headers);
        if(cb_output_ref != NULL) Cbuffer_dispose(&cb_output_ref);
    return;
}
/**
 *
 * Write a buffer of given length. TODO Must handle partial writes
 *
 * \param this    WrtRef (holds the socket/fd to which the write should be made
 * \param buffer  void* Address of start of buffer
 * \param len     int length of data in buffer
 * \return        (TODO) return success, EOF-socket closed by other end, IO error
 */
void Writer_write_chunk(WriterRef this, void* buffer, int len)
{
    WRITER_CHECK_TAG(this)
    char* c = (char*)buffer;
    void* tmp_buffer = buffer;
    int tmp_len = len;
    int my_errno;
    while(1) {
        int res = (int)write(this->m_sock, tmp_buffer, tmp_len);
        if(res == tmp_len) {
            return; // success
        } else if(res > 0) {
            // partial write
            tmp_buffer = tmp_buffer + res;
            tmp_len = tmp_len - res;
        } else if(res == 0) {
            // other end closed socket
            return; //eof
        } else if(res == -1) {
            my_errno = errno;
            return; //io_error
        }
    }
}
/**
 * TODO - intended this function to write the entire body in one go and ensure a content-length header was included
 *
 * \param this
 */
void Writer_end(WriterRef this)
{
    WRITER_CHECK_TAG(this)
}
int Writer_sock_fd(WriterRef this)
{
    WRITER_CHECK_TAG(this)
    return this->m_sock;
}
